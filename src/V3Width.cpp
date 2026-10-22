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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Width.h"

#ضمن "V3Assert.h"
#ضمن "V3Ast.h"
#ضمن "V3Begin.h"
#ضمن "V3Const.h"
#ضمن "V3ConstPool.h"
#ضمن "V3Error.h"
#ضمن "V3Global.h"
#ضمن "V3LinkLValue.h"
#ضمن "V3MemberMap.h"
#ضمن "V3Number.h"
#ضمن "V3Randomize.h"
#ضمن "V3String.h"
#ضمن "V3Task.h"
#ضمن "V3UniqueNames.h"
#ضمن "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#ضمن "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ثابت Ast_گرێ*, AstClass*> m_cache;

    AstClass* findCached(Ast_گرێ* nodep) {
        إذا (!nodep) أرجع nullptr;
        ثابت auto it = m_cache.find(nodep);
        إذا (it != m_cache.end()) أرجع it->second;

        AstClass* classp = nullptr;
        إذا (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } وإلا إذا (AstClass* ثابت parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } وإلا إذا (AstClassPackage* ثابت packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } وإلا {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        أرجع classp;
    }

عام:
    AstClass* find(Ast_گرێ* nodep) {
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) أرجع classp;
        إذا (AstClassPackage* ثابت packagep = VN_CAST(nodep, ClassPackage)) {
            أرجع packagep->classp();
        }
        أرجع findCached(nodep);
    }
};

//######################################################################

تعداد Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, ثابت Stage& rhs) {
    أرجع str << ("-PFB"[static_cast<صحيح>(rhs)]);
}

تعداد Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, ثابت Determ& rhs) {
    ساكن constexpr ثابت حرف* ثابت s_det[] = {"SELF", "CNTX", "ASSN"};
    أرجع str << s_det[rhs];
}

تعداد StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, پەیام) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               پەیام)

//######################################################################
// Width state, as a visitor of each AstNode

صنف WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_گرێ_DType* ثابت m_dtypep;  // Parent's data type to resolve to
    ثابت Stage m_stage;  // If true, report errors
    ثابت StreamUse m_streamUse;  // Current expression may be a stream concat

عام:
    WidthVP(Ast_گرێ_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        إذا (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { أرجع هذا; }
    منطقي selfDtm() ثابت { أرجع m_dtypep == nullptr; }
    StreamUse streamUse() ثابت { أرجع m_streamUse; }
    Ast_گرێ_DType* dtypep() ثابت {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        أرجع m_dtypep;
    }
    Ast_گرێ_DType* dtypeNullp() ثابت { أرجع m_dtypep; }
    Ast_گرێ_DType* dtypeNullSkipRefp() ثابت {
        Ast_گرێ_DType* dtp = dtypeNullp();
        إذا (dtp) dtp = dtp->skipRefp();
        أرجع dtp;
    }
    Ast_گرێ_DType* dtypeOverridep(Ast_گرێ_DType* ثابت defaultp) ثابت {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        إذا (!m_dtypep) أرجع defaultp;
        إذا (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            أرجع defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        أرجع m_dtypep;
    }
    صحيح width() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        أرجع m_dtypep->width();
    }
    صحيح widthMin() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        أرجع m_dtypep->widthMin();
    }
    منطقي prelim() ثابت { أرجع m_stage & PRELIM; }
    منطقي final() ثابت { أرجع m_stage & FINAL; }
    فراغ dump(std::ostream& str) ثابت {
        إذا (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } وإلا {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, ثابت WidthVP* vup) {
    إذا (vup) vup->dump(str);
    أرجع str;
}

//######################################################################

صنف WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    فراغ clearWidthRecurse(Ast_گرێ* nodep) {
        لكل (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(خطأ);
            إذا (Ast_گرێ* ثابت refp = nodep->op1p()) clearWidthRecurse(refp);
            إذا (Ast_گرێ* ثابت refp = nodep->op2p()) clearWidthRecurse(refp);
            إذا (Ast_گرێ* ثابت refp = nodep->op3p()) clearWidthRecurse(refp);
            إذا (Ast_گرێ* ثابت refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

عام:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    virtual ~WidthClearVisitor() = افتراضي;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_گرێ_iterate_instead_of_Ast_گرێ_accept

//######################################################################

صنف WidthVisitor final : عام VNVisitor {
    // TYPES
    using PatVecMap = std::map<صحيح, AstPatMember*>;
    using DTypeMap = std::map<ثابت std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ثابت AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    منطقي m_underFork = خطأ;  // Visiting under a fork
    منطقي m_underSExpr = خطأ;  // Visiting under a sequence expression
    منطقي m_underPackedArray = خطأ;  // Visiting under a AstPackArrayDType
    منطقي m_underMemberSel = خطأ;  // Viting under a MemberSel
    Ast_گرێ* m_seqUnsupp = nullptr;  // Property has unsupported node
    منطقي m_hasSExpr = خطأ;  // Property has a sequence expression
    ثابت AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    ثابت AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_گرێ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_گرێ_Module* m_modep = nullptr;  // Current module
    ثابت AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_گرێ_Procedure* m_procedurep = nullptr;  // Current final/always
    ثابت AstWith* m_withp = nullptr;  // Current 'with' statement
    ثابت AstFunc* m_funcp = nullptr;  // Current function
    ثابت AstAttrOf* m_attrp = nullptr;  // Current attribute
    ثابت Ast_گرێ_دەربڕین* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    ثابت منطقي m_paramsOnly;  // Computing parameter value; limit operation
    ثابت منطقي m_doGenerate;  // Do errors later inside generate statement
    منطقي m_streamConcat = خطأ;  // True if visiting arguments of stream concatenation
    منطقي m_inParamOverride = خطأ;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ثابت Ast_گرێ_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<ثابت Ast_گرێ_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<ثابت AstVar*> m_curModVars;  // Variables declared in current module

    ساكن constexpr صحيح ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    تعداد ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    تعداد صنف ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    ساكن StreamUse streamUseForDeterm(Determ determ) {
        أرجع determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() ثابت { أرجع m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    ساكن فراغ packIfUnpacked(Ast_گرێ_دەربڕین* ثابت nodep) {
        إذا (AstUnpackArrayDType* ثابت unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ثابت صحيح elements_ژمارە = unpackDTypep->arrayUnpackedElements();
            ثابت صحيح unpackMinBits = elements_ژمارە * unpackDTypep->subDTypep()->widthMin();
            ثابت صحيح unpackBits = elements_ژمارە * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(جديد AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    ساكن Ast_گرێ_DType* fromDTypep(Ast_گرێ* fromp) {
        إذا (Ast_گرێ_DType* ثابت dtypep = VN_CAST(fromp, گرێ_DType))
            أرجع dtypep->skipRefOrNullp();
        أرجع fromp ? fromp->dtypep() : nullptr;
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
    فراغ visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    فراغ visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    فراغ visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    فراغ visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    فراغ visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    فراغ visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    فراغ visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    فراغ visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    فراغ visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    فراغ visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    فراغ visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    فراغ visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    فراغ visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    فراغ visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    فراغ visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    فراغ visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    فراغ visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    فراغ visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    فراغ visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    فراغ visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    فراغ visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    فراغ visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    فراغ visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    فراغ visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    فراغ visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    فراغ visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    فراغ visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    فراغ visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    فراغ visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    فراغ visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    فراغ visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    فراغ visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    فراغ visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    فراغ visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    فراغ visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    فراغ visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    فراغ visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    فراغ visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    فراغ visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    فراغ visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    فراغ visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    فراغ visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    فراغ visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    فراغ visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    فراغ visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    فراغ visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    فراغ visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    فراغ visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, true, خطأ); }
    فراغ visit(AstSub* nodep) override { visit_add_sub_replace(nodep, true, خطأ); }
    فراغ visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, true, true); }
    فراغ visit(AstMul* nodep) override { visit_add_sub_replace(nodep, true, خطأ); }
    // These can't promote to real
    فراغ visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, خطأ, true); }
    فراغ visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, خطأ, true); }
    فراغ visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, خطأ, خطأ); }
    فراغ visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, خطأ, true); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    فراغ visit(AstNegate* nodep) override { visit_negate_not(nodep, true); }
    // Unary never real
    فراغ visit(AstNot* nodep) override { visit_negate_not(nodep, خطأ); }

    // Real: inputs and output real
    فراغ visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    فراغ visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    فراغ visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    فراغ visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    فراغ visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    فراغ visit(Ast_گرێ_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    فراغ visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    فراغ visit(Ast_گرێ_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    فراغ visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    فراغ visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    فراغ visit(AstShiftL* nodep) override { visit_shift(nodep); }
    فراغ visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    فراغ visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    فراغ visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    فراغ visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    فراغ visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    فراغ visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    فراغ visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    فراغ visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    فراغ visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    فراغ visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    فراغ visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    فراغ visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    فراغ visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            إذا (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } وإلا {
                nodep->dtypeSetInteger();
            }
        }
    }
    فراغ visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    فراغ visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    فراغ visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    فراغ visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    فراغ visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    فراغ visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            جديد AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
    }
    فراغ visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    ساكن منطقي isOverridePattern(ثابت Ast_گرێ* nodep) {
        إذا (ثابت AstPattern* ثابت patternp = VN_CAST(nodep, Pattern)) {
            أرجع !patternp->childDTypep();
        }
        ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond);
        أرجع condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    فراغ visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_دەربڕین(nodep);
        إذا (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ثابت صحيح errors = V3Error::error_ژماردن();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = خطأ;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const);
            ثابت منطقي known = constp && !constp->ژمارە().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            إذا (!known && V3Error::error_ژماردن() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_گرێ_دەربڕین* ثابت keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            أرجع;
        }
        إذا (m_vup->prelim()) {  // First stage evaluation
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
            Ast_گرێ_DType* ثابت thenDTypep = nodep->thenp()->dtypep();
            Ast_گرێ_DType* ثابت elseDTypep = nodep->elsep()->dtypep();
            Ast_گرێ_DType* ثابت thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_گرێ_DType* ثابت elseDTypeSkippedp = elseDTypep->skipRefp();
            إذا (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } وإلا إذا (nodep->thenp()->is_Class_Handle_بەها()
                       || nodep->elsep()->is_Class_Handle_بەها()) {
                Ast_گرێ_DType* commonClassTypep = nullptr;
                إذا (nodep->thenp()->is_Class_Handle_بەها() && nodep->elsep()->is_Class_Handle_بەها()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_گرێ::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                إذا (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } وإلا {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } وإلا إذا (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                إذا (Ast_گرێ::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } وإلا {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    إذا (!Ast_گرێ::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } وإلا إذا (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } وإلا إذا (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } وإلا {
                ثابت منطقي isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                منطقي handled = خطأ;
                ثابت منطقي isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_گرێ_DType* ثابت thenEnumDTypep = thenDTypep->skipRefToEnump();
                إذا (VN_IS(thenDTypeSkippedp, گرێ_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_درەخت(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = true;
                } وإلا إذا (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_درەخت(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    إذا (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = true;
                    } وإلا {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(جديد File_هێڵ{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, true);
                    }
                }
                إذا (!handled) {
                    ثابت صحيح width = std::max(thenDTypep->width(), elseDTypep->width());
                    ثابت صحيح widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    ثابت VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ثابت منطقي isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    إذا ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } وإلا إذا ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } وإلا {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        إذا (m_vup->final()) {
            Ast_گرێ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرێ_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    فراغ visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_گرێ_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        إذا (Ast_گرێ_دەربڕین* ثابت newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* ثابت newp = جديد AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            أرجع;
        }

        // Concat handling
        إذا (m_vup->prelim()) {
            إذا (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            إذا (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } وإلا {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                إذا (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_گرێ_دەربڕین::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            إذا (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->lhsp(), Replicate)) {
                إذا (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            إذا (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->rhsp(), Replicate)) {
                إذا (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع;
                }
            }
        }
        إذا (m_vup->final()) {
            إذا (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_گرێ_دەربڕین* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_گرێ_دەربڕین* rhsp = nodep->rhsp()->unlinkFrBack();
                إذا (!lhsp->isString()) lhsp = جديد AstCvtPackString{lhsp->fileline(), lhsp};
                إذا (!rhsp->isString()) rhsp = جديد AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_گرێ* ثابت newp = جديد AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                أرجع;
            }
            إذا (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گرێ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    فراغ visit(AstConcatN* nodep) override {
        إذا (nodep->didWidth()) أرجع;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        إذا (m_vup->final()) {
            nodep->didWidth(true);
            إذا (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گرێ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    فراغ visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    فراغ visit(AstDelay* nodep) override {
        إذا (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            إذا (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            إذا (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                أرجع;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            أرجع;
        }
        إذا (Ast_گرێ_دەربڕین* ثابت fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            أرجع;
        }
        إذا (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                أرجع;
            } وإلا إذا (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } وإلا {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        إذا (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    فراغ visit(AstFork* nodep) override {
        إذا (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        إذا (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ثابت newp = جديد AstBegin{nodep->fileline(), nodep->name(), nullptr, خطأ};
            nodep->replaceWith(newp);
            إذا (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            إذا (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            إذا (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا إذا (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = true;
            iterateChildren(nodep);
        } وإلا إذا (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
        } وإلا {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    فراغ visitWaitOrDisableFork(Ast_گرێ* nodep) {
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
                أرجع;
            } وإلا إذا (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    فراغ visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    فراغ visit(AstToLowerN* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    فراغ visit(AstToUpperN* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    فراغ visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
            إذا (constp) {
                إذا (constp->ژمارە().isFourState()
                    || (constp->dtypep()->isSigned() && constp->ژمارە().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } وإلا {
                    times = constp->toSInt();
                }
            }

            Ast_گرێ_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
            إذا (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                إذا (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                إذا (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع;
                } وإلا {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* ثابت newp = جديد AstPattern{
                        nodep->fileline(),
                        جديد AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    أرجع;
                }
            }
            إذا (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            إذا (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } وإلا {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            إذا ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_گرێ* ثابت newp
                    = جديد AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                أرجع;
            } وإلا {
                إذا (!constp) nodep->v3error("Replication value isn't a constant.");
                إذا (times == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    times = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    (nodep->srcp()->width() * times), (nodep->srcp()->widthMin() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        إذا (m_vup->final()) {
            إذا (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گرێ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    فراغ visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        إذا (m_vup->final()) {
            إذا (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گرێ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    فراغ visit(Ast_گرێ_Dist_Biop* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    فراغ visit(Ast_گرێ_Dist_Triop* nodep) override {
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    منطقي streamImplicitUseAllowed(ثابت Ast_گرێ_Stream* nodep) ثابت {
        إذا (m_streamConcat) أرجع true;
        إذا (m_vup && m_vup->streamUse() != STREAM_USE_NONE) أرجع true;

        ثابت Ast_گرێ* ثابت backp = nodep->backp();
        إذا (!backp) أرجع true;  // Error elsewhere

        أرجع VN_IS(backp, گرێ_Assign);
    }
    فراغ visit(Ast_گرێ_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            m_streamConcat = true;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = خطأ;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const)) {
                إذا (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } وإلا {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ثابت Ast_گرێ_DType* ثابت lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            إذا (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, گرێ_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, گرێ_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } وإلا إذا (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } وإلا {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_گرێ_دەربڕین::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        إذا (m_vup->final()) {
            إذا (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            إذا (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گرێ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    فراغ visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), true,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), true,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        إذا (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ثابت صحيح width = nodep->elementsConst();
            إذا (nodep->fromBracket() && nodep->چەپ_Const() > nodep->ڕاست_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } وإلا إذا (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ثابت منطقي inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ثابت منطقي inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ثابت منطقي inTypeTable = !m_modep;
            ثابت Ast_گرێ* basep = nodep->backp();
            طالما (VN_IS(basep, Range)) basep = basep->backp();
            إذا (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->چەپ_Const() << ":" << nodep->ڕاست_Const()
                                            << "]");
            }
        }
    }

    فراغ visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        إذا (nodep->didWidth()) أرجع;
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), خطأ);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            صحيح width = nodep->widthConst();
            إذا (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                أرجع;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ثابت منطقي isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ثابت منطقي inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            إذا (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                پاڵنان_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(جديد AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            إذا (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                إذا (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_گرێ_DType* ثابت subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    خطأ /*noerror*/);
                } وإلا {
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
            صحيح frommsb = nodep->fromp()->width() - 1;
            صحيح fromlsb = 0;
            ثابت صحيح elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            إذا (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } وإلا {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ثابت صحيح selwidth = V3_ژمار::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_گرێ_DType* ثابت selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            إذا (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                إذا (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            إذا (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                إذا (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } وإلا إذا (!inParameterizedTemplate) {
                    إذا (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } وإلا {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                إذا (!isWriteSelect) {
                    // Extend it
                    ثابت صحيح extendTo = nodep->msbConst() + 1;
                    Ast_گرێ_DType* ثابت subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    خطأ /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            إذا (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                خطأ /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    فراغ visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            إذا (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->bitp(), Const)) {
                    ثابت صحيح bit = constp->toSInt();
                    Ast_گرێ_دەربڕین* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    إذا (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ثابت AstUnpackArrayDType* ثابت arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            //
            صحيح frommsb;
            صحيح fromlsb;
            ثابت Ast_گرێ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                إذا (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                إذا (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } وإلا {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ثابت صحيح selwidth = V3_ژمار::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_گرێ_DType* ثابت selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            إذا (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                إذا (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            إذا (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                إذا (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    إذا (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                خطأ /*NOWARN*/);
            }
        }
    }

    فراغ visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            ثابت Ast_گرێ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstAssocArrayDType* ثابت adtypep = VN_CAST(fromDtp, AssocArrayDType);
            إذا (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->کلیل_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    فراغ visit(AstAlias* nodep) override {
        إذا (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        ثابت auto check_If_دەربڕین_Ok = [هذا](ثابت Ast_گرێ_دەربڕین* ثابت exprp) {
            إذا (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                أرجع خطأ;
            }

            ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef);
            إذا (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                أرجع خطأ;
            }
            AstVar* ثابت varp = varRefp->varp();
            إذا (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                أرجع خطأ;
            }
            إذا (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                أرجع خطأ;
            } وإلا {
                m_aliasedVars.زیادکردن(varp);
            }
            أرجع true;
        };

        check_If_دەربڕین_Ok(nodep->itemsp());
        Ast_گرێ_DType* ثابت firstItemDtypep = nodep->itemsp()->dtypep();
        لكل (Ast_گرێ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_دەربڕین_Ok(VN_AS(itemp, گرێ_دەربڕین));
            إذا (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    فراغ visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            ثابت Ast_گرێ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstWildcardArrayDType* ثابت adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            إذا (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ثابت AstBasicDType* ثابت basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            إذا (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    فراغ visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ثابت Ast_گرێ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            إذا (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_گرێ_DType* ثابت newDtp
                = جديد AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          جديد AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            إذا (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ثابت صحيح subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ثابت VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                إذا ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(جديد AstConsArray{
                        nodep->fileline(), newDtp,
                        جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع;
                } وإلا إذا ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            إذا (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ثابت fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ثابت newp = جديد AstConsArray{nodep->fileline(), newDtp};
                ثابت VNumRange range = nodep->declRange();
                لكل (صحيح n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_درەخت(خطأ));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            }
        }
    }

    فراغ visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_گرێ* ثابت selp = V3Width::widthSelNoIterEdit(nodep);
        إذا (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    فراغ visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_گرێ* ثابت selp = V3Width::widthSelNoIterEdit(nodep);
        إذا (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    فراغ visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_گرێ* ثابت selp = V3Width::widthSelNoIterEdit(nodep);
        إذا (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    فراغ visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_گرێ* ثابت selp = V3Width::widthSelNoIterEdit(nodep);
        إذا (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    فراغ visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        إذا (nodep->didWidthAndSet()) أرجع;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        إذا (nodep->didWidthAndSet()) أرجع;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        إذا (nodep->didWidthAndSet()) أرجع;
        إذا (m_vup && m_vup->prelim()) {
            إذا (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } وإلا إذا (nodep->ژمارە().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } وإلا إذا (nodep->ژمارە().isString()) {
                nodep->dtypeSetString();
            } وإلا إذا (nodep->ژمارە().isDouble()) {
                nodep->dtypeChgWidth(nodep->ژمارە().width(), nodep->ژمارە().sized()
                                                               ? nodep->ژمارە().width()
                                                               : nodep->ژمارە().widthToFit());
            } وإلا {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->ژمارە().width(),
                    nodep->ژمارە().sized() ? nodep->ژمارە().width() : nodep->ژمارە().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->ژمارە().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    فراغ visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        إذا (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    فراغ visit(AstFell* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstFalling* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstFuture* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    فراغ visit(AstPast* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeFrom(nodep->exprp());
            إذا (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ true);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ثابت AstConst* ثابت constp = VN_CAST(nodep->ticksp(), Const);
                إذا (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_درەخت();
                } وإلا إذا (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_درەخت();
                } وإلا إذا (V3AssertCommon::check_Sva_بەها_Limit(nodep, constp, "$past tick value")) {
                    إذا (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    فراغ visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
            إذا (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } وإلا إذا (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } وإلا إذا (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            إذا (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
                إذا (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } وإلا إذا (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } وإلا إذا (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_گرێ_دەربڕین* propp = nodep->propp();
            إذا (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            إذا (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ثابت منطقي loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ثابت منطقي hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            إذا (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_گرێ* ثابت boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                أرجع;
            }
            إذا (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                أرجع;
            }
            ثابت AstConst* ثابت loConstp = VN_CAST(nodep->loBoundp(), Const);
            ثابت AstConst* ثابت hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            إذا (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            إذا (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            إذا (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } وإلا إذا (loConstp) {
                V3AssertCommon::check_Sva_بەها_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            إذا (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_بەها_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            إذا (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            منطقي hasPropertyOp = propp->isMultiCycleSva();
            إذا (!hasPropertyOp) {
                propp->foreach([&](ثابت Ast_گرێ* np) {
                    إذا (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = true;
                    }
                });
            }
            إذا (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstRising* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstRose* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    فراغ visit(AstSampled* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    فراغ visit(AstSetuphold* nodep) override {
        File_هێڵ* ثابت flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        إذا (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        إذا (nodep->deldatap()) {
            إذا (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } وإلا {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        إذا (!newp) {
            VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        nodep->replaceWith(جديد AstAlways{newp});
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_هێڵ* ثابت flp, Ast_گرێ_دەربڕین* ثابت evp,
                                         Ast_گرێ_دەربڕین* ثابت delp) {
        Ast_گرێ_دەربڕین* ثابت lhsp = delp->clone_درەخت_Pure(خطأ);
        Ast_گرێ_دەربڕین* ثابت rhsp = evp->clone_درەخت_Pure(خطأ);
        UASSERT_OBJ(VN_IS(lhsp, گرێ_Var_Ref) || VN_IS(lhsp, گرێ_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        إذا (Ast_گرێ_Var_Ref* varRefp = VN_CAST(lhsp, گرێ_Var_Ref)) {
            إذا (varRefp->varp()->direction() == VDirection::تێخستن) {
                VL_DO_DANGLING(پاڵنان_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(پاڵنان_Deletep(rhsp), rhsp);
                أرجع nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        إذا (Ast_گرێ_Pre_Sel* selp = VN_CAST(lhsp, گرێ_Pre_Sel)) {
            إذا (Ast_گرێ_Var_Ref* varRefp = VN_CAST(selp->fromp(), گرێ_Var_Ref)) {
                إذا (varRefp->varp()->direction() == VDirection::تێخستن) {
                    VL_DO_DANGLING(پاڵنان_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(پاڵنان_Deletep(rhsp), rhsp);
                    أرجع nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        أرجع جديد AstAssignW{flp, lhsp, rhsp};
    }

    فراغ visit(AstStable* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstSteady* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeSetBit();
        }
    }

    فراغ visit(Ast_Stmt_دەربڕین* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    فراغ visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    فراغ visit(AstUntil* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            إذا (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            إذا (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    فراغ visit(AstRand* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            إذا (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } وإلا {
                nodep->dtypeSetInt();  // Says the spec
            }
            إذا (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    فراغ visit(AstSEventually* nodep) override {
        إذا (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::Widthed_بەها{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
            أرجع;
        }
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstSGotoRep* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            إذا (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } وإلا {
                width_Check_Rep_ژماردن(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstSNonConsRep* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            إذا (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } وإلا {
                width_Check_Rep_ژماردن(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    فراغ width_Check_Rep_ژماردن(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
        إذا (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } وإلا إذا (constp->dtypep()->isSigned() && constp->ژمارە().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } وإلا إذا (constp->ژمارە().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } وإلا إذا (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    فراغ widthCheckGotoRepRange(T_Rep* nodep, ثابت حرف* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
        ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
        إذا (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            أرجع;
        }
        إذا (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            أرجع;
        }
        إذا (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            أرجع;
        }
        إذا (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    فراغ visit(AstSThroughout* nodep) override {
        m_hasSExpr = true;
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = خطأ;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = true;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = true;
        m_hasSExpr = true;
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            إذا (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = true;
        m_hasSExpr = true;
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            لكل (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstURandomRange* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_گرێ_DType* ثابت expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    فراغ visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        إذا (VN_IS(nodep->backp(), IsUnbounded)) أرجع;  // Ok, leave
        إذا (VN_IS(nodep->backp(), BracketArrayDType)) أرجع;  // Ok, leave
        إذا (VN_IS(nodep->backp(), InsideRange)) أرجع;  // Ok, leave
        إذا (ثابت auto* ثابت varp = VN_CAST(nodep->backp(), Var)) {
            إذا (varp->isParam()) أرجع;  // Ok, leave
        }
        Ast_گرێ* backp = nodep->backp();
        إذا (VN_IS(backp, Sub)) backp = backp->backp();
        إذا (ثابت auto* ثابت selp = VN_CAST(backp, SelExtract)) {
            إذا (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) أرجع;
        }
        إذا (ثابت auto* ثابت selp = VN_CAST(backp, SelBit)) {
            إذا (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) أرجع;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    فراغ visit(AstInferredDisable* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) nodep->dtypeSetBit();
    }
    فراغ visit(AstIsUnbounded* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    فراغ visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        إذا (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        إذا (m_vup->final()) {
            Ast_گرێ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            إذا (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    فراغ visit(AstCLog2* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    فراغ widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_گرێ_دەربڕین* ثابت valuep = nodep->valuep();
        ثابت std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            إذا (!constp->ژمارە().isNegative()) أرجع;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } وإلا إذا (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } وإلا {
            أرجع;
        }
        // Continue with the default weight
        valuep->replaceWith(جديد AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(پاڵنان_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    فراغ widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    فراغ visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ثابت cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        إذا (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            أرجع;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        إذا (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            أرجع;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        إذا (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            إذا (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } وإلا {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } وإلا إذا (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_گرێ_دەربڕین* ثابت valuep = nodep->valuep();
            إذا (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(جديد AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(پاڵنان_Deletep(valuep), valuep);
            }
            أرجع;
        } وإلا إذا (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            أرجع;
        } وإلا إذا (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ثابت AstConst* ثابت constp = VN_CAST(nodep->valuep(), Const);
            إذا (!constp || !constp->ژمارە().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstCoverOption* nodep) override {
        إذا (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            أرجع;
        }
        إذا (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            أرجع;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    فراغ visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    فراغ visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    فراغ widthCovergroupRanges(Ast_گرێ* rangesp, صحيح fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ثابت auto fill = [&](Ast_گرێ* nodep) {
            إذا (!fillWidth) أرجع;
            Ast_گرێ_دەربڕین* exprp = VN_AS(nodep, گرێ_دەربڕین);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        لكل (Ast_گرێ *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            إذا (AstInsideRange* ثابت rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } وإلا إذا (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } وإلا {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    فراغ visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    فراغ visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        إذا (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    فراغ visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        إذا (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        إذا (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    فراغ visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        إذا (Ast_گرێ_دەربڕین* ثابت countp = nodep->countp()) {
            إذا (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } وإلا {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    فراغ widthCoverFilter(Ast_گرێ_دەربڕین* filterp, ثابت حرف* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), گرێ_دەربڕین);
        File_هێڵ* ثابت fl = filterp->fileline();
        ثابت Ast_گرێ_DType* ثابت resultp = filterp->dtypep()->skipRefp();
        إذا (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                جديد AstNeqD{fl, filterp, جديد AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } وإلا إذا (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(جديد AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(پاڵنان_Deletep(filterp), filterp);
        } وإلا {
            fixWidthReduce(filterp);
        }
    }
    فراغ visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    فراغ visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    فراغ visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            إذا (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                أرجع;
            }
            checkCvtUS(nodep->lhsp(), خطأ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            إذا (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->ژمارە().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        إذا (m_vup->final()) {
            Ast_گرێ_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرێ_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_گرێ* newp = nullptr;  // No change
            إذا (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = جديد AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } وإلا إذا (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = جديد AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } وإلا إذا (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = جديد AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            إذا (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            }
        }
    }
    فراغ visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(Ast_ژماردن_Bits* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحيح widthMin = V3_ژمار::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    فراغ visit(Ast_ژماردن_Ones* nodep) override {
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحيح widthMin = V3_ژمار::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    فراغ visit(AstCvtPackString* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        إذا (Ast_گرێ_DType* ثابت dt = nodep->lhsp()->dtypep()) {
            إذا (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    فراغ visit(AstCvtPackedToArray* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    فراغ visit(AstCvtArrayToArray* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    فراغ visit(AstCvtArrayToPacked* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    فراغ visit(AstCvtUnpackToQueue* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    فراغ visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        إذا (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    فراغ visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ثابت مزدوج timePrescale = constp->ژمارە().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ثابت مزدوج time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
    }
    فراغ visit(AstEventControl* nodep) override {
        إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
            أرجع;
        }
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                أرجع;
            } وإلا إذا (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } وإلا {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        إذا (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    ساكن string typenameExtends(ثابت AstClass* classp) {
        // An interface class may extend several, so those are not named
        إذا (!classp || classp->isInterfaceClass()) أرجع "";
        لكل (ثابت AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            إذا (extendsp->isImplements()) استمر;
            ثابت AstClass* ثابت basep = extendsp->classOrNullp();
            أرجع basep ? " extends class " + basep->dtypeName(true) : "";
        }
        أرجع "";
    }
    فراغ visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        إذا (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        بدل (nodep->attrType()) {
        حالة VAttrType::DIM_DIMENSIONS:
        حالة VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_گرێ_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ثابت std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(true);
            ثابت صحيح val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(جديد AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            اكسر;
        }
        حالة VAttrType::DIM_BITS_OR_ژمار: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            إذا (VN_IS(nodep->fromp(), گرێ_دەربڕین)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            } وإلا {
                Ast_گرێ* newp = جديد AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            أرجع;
        }
        حالة VAttrType::DIM_BITS:
        حالة VAttrType::DIM_بەرز:
        حالة VAttrType::DIM_INCREMENT:
        حالة VAttrType::DIM_چەپ:
        حالة VAttrType::DIM_نزم:
        حالة VAttrType::DIM_ڕاست:
        حالة VAttrType::DIM_SIZE: {
            Ast_گرێ_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            إذا (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ثابت Ast_گرێ_Module* ثابت ownModp = v3Global.rootp()->containingModule(dtypep);
                إذا (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    أرجع;
                }
            }

            إذا (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                بدل (nodep->attrType()) {
                حالة VAttrType::DIM_SIZE: {
                    Ast_گرێ_دەربڕین* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), گرێ_دەربڕین);
                    Ast_گرێ* ثابت newp
                        = جديد AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(true);
                    newp->protect(خطأ);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_چەپ:
                حالة VAttrType::DIM_نزم: {
                    Ast_گرێ* ثابت newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_ڕاست:
                حالة VAttrType::DIM_بەرز: {
                    Ast_گرێ_دەربڕین* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), گرێ_دەربڕین);
                    Ast_گرێ_دەربڕین* ثابت sizep
                        = جديد AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(true);
                    sizep->protect(خطأ);
                    Ast_گرێ* ثابت newp
                        = جديد AstSub{nodep->fileline(), sizep,
                                     جديد AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_INCREMENT: {
                    Ast_گرێ_دەربڕین* ثابت newp
                        = جديد AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_BITS: {
                    إذا (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } وإلا {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    اكسر;
                }
                افتراضي: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } وإلا {
                ثابت std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(true);
                ثابت uint32_t msbdim = dimpair.first + dimpair.second;
                إذا (!nodep->dimp() || msbdim < 1) {
                    إذا (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_گرێ_دەربڕین* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), گرێ_دەربڕین);
                        Ast_گرێ* ثابت newp = جديد AstShiftL{
                            nodep->fileline(), جديد AstLenN{nodep->fileline(), fromp},
                            جديد AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    } وإلا {
                        Ast_گرێ_DType* ثابت baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        ثابت صحيح dim = 1;
                        AstConst* ثابت newp = dimension_بەها(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    }
                } وإلا إذا (VN_IS(nodep->dimp(), Const)) {
                    ثابت صحيح dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* ثابت newp
                        = dimension_بەها(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                } وإلا {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ثابت tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_گرێ_دەربڕین* ثابت dimp = nodep->dimp()->unlinkFrBack();
                    Ast_گرێ_دەربڕین* ثابت newp = جديد AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                }
            }
            اكسر;
        }
        حالة VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            اكسر;
        حالة VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_گرێ_DType* ثابت protoDtp = nodep->fromp()->dtypep();
            Ast_گرێ_DType* ثابت declDtp = [&]() {
                إذا (m_ftaskp->fvarp()) أرجع m_ftaskp->fvarp()->dtypep();
                Ast_گرێ_DType* ثابت voidp = جديد AstVoidDType{m_ftaskp->fileline()};
                پاڵنان_Deletep(voidp);  // Note voidp used past here
                أرجع voidp;
            }();
            إذا (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            اكسر;
        }
        حالة VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_گرێ_DType* ثابت dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(true);
            إذا (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_گرێ* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
            اكسر;
        }
        حالة VAttrType::TYPEID:
            إذا (Ast_گرێ_DType* dtypep = VN_CAST(nodep->fromp(), گرێ_DType)) {
                nodep->dtypep(dtypep);
            } وإلا {
                nodep->dtypep(VN_AS(nodep->fromp(), گرێ_دەربڕین)->dtypep());
            }
            اكسر;
        حالة VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            اكسر;
        افتراضي: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            اكسر;
        }
        }
    }
    فراغ visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    فراغ visit(AstCellArrayRef* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(true);
    }

    فراغ visit(AstCellRef* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;

        إذا (Ast_گرێ_دەربڕین* ثابت cellExprp = VN_CAST(nodep->cellp(), گرێ_دەربڕین)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        إذا (Ast_گرێ_دەربڕین* ثابت exprp = VN_CAST(nodep->exprp(), گرێ_دەربڕین)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } وإلا {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(true);
    }

    // DTYPES
    فراغ visit(Ast_گرێ_Array_DType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed

        إذا (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ثابت basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            إذا (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* ثابت newp = جديد AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        إذا (VN_IS(nodep, PackArrayDType)) m_underPackedArray = true;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        إذا (AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            إذا (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(true);
        } وإلا {
            ثابت Ast_گرێ_DType* ثابت elemDTypep = nodep->subDTypep()->skipRefp();
            إذا (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ثابت صحيح width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            إذا (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ثابت Ast_گرێ_DType* baseDTypep = nodep->subDTypep();
                طالما (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                ثابت AstBasicDType* ثابت basicp = nodep->basicp();
                إذا (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(true));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ visit(AstAssocArrayDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->کلیل_DTypep(iterateEditMoveDTypep(nodep, nodep->کلیل_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_گرێ_DType* ثابت childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_گرێ* ثابت elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_گرێ* newp;
        إذا (VN_IS(elementsp, Unbounded)) {
            newp = جديد AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_درەخت(), elementsp);
        } وإلا إذا (Ast_گرێ_DType* ثابت keyp = VN_CAST(elementsp, گرێ_DType)) {
            newp = جديد AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } وإلا {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_هێڵ* ثابت elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, true);
            // Must be expression that is constant, but we'll determine that later
            newp = جديد AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                جديد AstRange{nodep->fileline(), جديد AstConst(elementsp->fileline(), 0),
                             جديد AstSub{elementsNewFl, VN_AS(elementsp, گرێ_دەربڕین),
                                        جديد AstConst(elementsNewFl, 1)},
                             true}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    فراغ visit(AstConstraintRefDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    فراغ visit(AstDynArrayDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ visit(AstQueueDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        إذا (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_درەخت();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ visit(AstVoidDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ visit(AstUnsizedArrayDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        إذا (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ visit(AstWildcardArrayDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ visit(AstBasicDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        إذا (nodep->generic()) أرجع;  // Already perfect
        إذا (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } وإلا إذا (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } وإلا إذا (nodep->implicit()) {
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
    فراغ visit(AstConstDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ visit(AstRefDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(true);
        إذا (nodep->typeofp()) {  // type(typeofp_expression)
            إذا (Ast_گرێ_DType* typeofDtp = VN_CAST(nodep->typeofp(), گرێ_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } وإلا {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_گرێ* ثابت typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_درەخت(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        إذا (nodep->subDTypep()) {
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
        إذا (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ثابت منطقي inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            إذا (inTemplateModule) {
                nodep->doingWidth(خطأ);
                أرجع;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(خطأ);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    فراغ widthDTypeNameParams(ثابت AstTypedef* nodep) {
        ثابت VDTypeNameScopes* ثابت scopesp = VDTypeNameScopes::currentp();
        إذا (!scopesp) أرجع;  // Computing parameters, as by V3Param, so naming as found
        Ast_گرێ* scopep = nullptr;
        لكل (ثابت Ast_گرێ* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            إذا (ثابت std::vector<Ast_گرێ*>* ثابت paramsp = scopesp->paramsp(scopep)) {
                لكل (Ast_گرێ* ثابت paramp : *paramsp) {
                    إذا (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    فراغ visit(AstTypedef* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        إذا (auto* ثابت refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ثابت منطقي declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        إذا (declares) {
            Ast_گرێ_DType* ثابت dtypep = nodep->dtypep();
            إذا (Ast_گرێ_UOr_Struct_DType* ثابت sdtypep = VN_CAST(dtypep, گرێ_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } وإلا إذا (AstEnumDType* ثابت edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    فراغ visit(AstParamTypeDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        إذا (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    فراغ visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        إذا (Ast_گرێ_DType* ثابت dtp = VN_CAST(nodep->lhsp(), گرێ_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } وإلا {
            إذا (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(جديد AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
    }
    فراغ visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_گرێ_DType* ثابت toDtp = nodep->لووتکە()->dtypep()->skipRefToEnump();
        Ast_گرێ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_هێڵ* ثابت fl = nodep->fileline();
        ثابت auto castable = Ast_گرێ::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_گرێ* newp;
        إذا (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            أرجع;
        } وإلا إذا (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ثابت enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_گرێ_دەربڕین* ثابت testp = enumTestValid(nodep->fromp(), enumDtp);
            File_هێڵ* ثابت fl_novalue = جديد File_هێڵ{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, true);
            newp = جديد AstCond{
                fl, testp,
                جديد Ast_دەربڕین_Stmt{fl,
                                جديد AstAssign{fl_novalue, nodep->لووتکە()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                جديد AstConst{fl, AstConst::Signed32{}, 1}},
                جديد AstConst{fl, AstConst::Signed32{}, 0}};
        } وإلا إذا ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = جديد Ast_دەربڕین_Stmt{
                fl,
                جديد AstAssign{fl, nodep->لووتکە()->unlinkFrBack(),
                              جديد AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                جديد AstConst{fl, AstConst::Signed32{}, 1}};
        } وإلا إذا (castable == VCastable::INCOMPATIBLE) {
            newp = جديد AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } وإلا {
            newp = جديد AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    فراغ visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        إذا (AstConst* ثابت constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_گرێ* ثابت newp
                = جديد AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } وإلا إذا (Ast_گرێ_DType* ثابت refp = VN_CAST(nodep->dtp(), گرێ_DType)) {
            refp->unlinkFrBack();
            Ast_گرێ* ثابت newp = جديد AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        }
    }
    فراغ visit(AstCast* nodep) override {
        إذا (nodep->didWidth()) أرجع;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        إذا (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            إذا (AstSigned* ثابت fromp = VN_CAST(nodep->fromp(), Signed)) {
                إذا (VN_IS(fromp->lhsp(), گرێ_Stream)) {
                    Ast_گرێ* ثابت lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_درەخت(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_گرێ_DType* ثابت toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_گرێ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            ثابت auto castable = Ast_گرێ::computeCastable(toDtp, fromDtp, nodep->fromp());
            منطقي bad = خطأ;
            إذا (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = true;
            } وإلا إذا (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } وإلا إذا (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = true;
            } وإلا إذا (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = true;
            } وإلا {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_گرێ_دەربڕین* newp = nullptr;
            إذا (bad) {
            } وإلا إذا (AstBasicDType* ثابت basicp = toDtp->basicp()) {
                إذا (!basicp->isString() && fromDtp->isString()) {
                    newp = جديد AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } وإلا إذا (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_گرێ_DType* ثابت origDTypep = nodep->dtypep();
                    إذا (!VN_IS(fromDtp, StreamDType)) {
                        ثابت صحيح width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } وإلا {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 خطأ);
                }
                إذا (newp) {
                } وإلا إذا (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    إذا (nodep->fromp()->isSigned()) {
                        newp = جديد AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } وإلا {
                        newp = جديد AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } وإلا إذا (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = جديد AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } وإلا إذا (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = جديد AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } وإلا إذا (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = جديد AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } وإلا {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } وإلا إذا (VN_IS(toDtp, QueueDType)) {
                إذا (VN_IS(fromDtp, BasicDType)) {
                    newp = جديد AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } وإلا إذا (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    صحيح srcElementBits = 1;
                    صحيح dstElementBits = 1;
                    إذا (Ast_گرێ_DType* ثابت elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ثابت AstQueueDType* ثابت dstQueueDtp = VN_AS(toDtp, QueueDType);
                    إذا (Ast_گرێ_DType* ثابت elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = جديد AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  خطأ,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } وإلا إذا (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } وإلا {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            إذا (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        إذا (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, خطأ, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_گرێ_دەربڕین* ثابت underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(true);
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(true);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        }
    }
    فراغ visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            صحيح width = nodep->rhsp()->toSInt();
            إذا (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        إذا (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_گرێ* ثابت underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    فراغ visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    فراغ castSized(Ast_گرێ* nodep, Ast_گرێ* underp, صحيح width) {
        ثابت AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        إذا (!underDtp) underDtp = underp->dtypep()->basicp();
        إذا (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ثابت صحيح calcWidth = std::max(width, underDtp->width());
            Ast_گرێ_DType* ثابت calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         خطأ, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_گرێ_DType* ثابت outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, گرێ_دەربڕین), outDtp, EXTEND_EXP,
                            خطأ);
            VL_DANGLING(underp);
        }
    }
    فراغ visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(Ast_Constraint_دەربڕین* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    فراغ visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        إذا (nodep->didWidth()) أرجع;
        إذا (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            پاڵنان_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(true);
            أرجع;
        }
        nodep->doingWidth(true);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        إذا (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.پاڵنان_back(nodep);
        إذا (nodep->attrsp()) {
            nodep->attrsp()->foreach([هذا, nodep](AstAttrOf* attrp) {
                إذا (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    إذا (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), گرێ_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(پاڵنان_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        إذا (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_گرێ_DType* dtp = nodep->dtypep();
            Ast_گرێ_DType* np = nullptr;
            طالما (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                إذا (ثابت AstDynArrayDType* ثابت unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    إذا (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } وإلا {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ثابت newp
                        = جديد AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    إذا (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } وإلا {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } وإلا {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        إذا (AstWildcardArrayDType* ثابت wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        إذا (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(true);
        // Parameters if implicit untyped inherit from what they are assigned to
        ثابت AstBasicDType* ثابت bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        منطقي didchk = خطأ;
        ثابت منطقي implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        إذا (implicitParam) {
            إذا (nodep->valuep()) {
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
                إذا (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } وإلا إذا (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } وإلا {
                    صحيح width = 0;
                    Ast_گرێ_DType* ثابت بەها_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(بەها_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ثابت AstBasicDType* ثابت بەها_Bdtypep = بەها_DTypep->basicp();
                    منطقي issigned = خطأ;
                    إذا (bdtypep->isNosign()) {
                        إذا (بەها_Bdtypep && بەها_Bdtypep->isSigned()) issigned = true;
                    } وإلا {
                        issigned = bdtypep->isSigned();
                    }
                    إذا (بەها_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        إذا (ثابت auto* ثابت constp = VN_CAST(nodep->valuep(), Const)) {
                            إذا (constp->ژمارە().isString()) {
                                width = constp->ژمارە().toString().length() * 8;
                            }
                        }
                        إذا (width < 8) width = 8;
                    } وإلا إذا (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } وإلا {
                        إذا (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ثابت منطقي is_بەها_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    إذا (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_بەها_Fourstate));
                    } وإلا {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_بەها_Fourstate);
                    }
                    didchk = true;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } وإلا {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } وإلا إذا (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        إذا (nodep->isNet()) {
            Ast_گرێ_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            إذا (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        إذا (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(true);
        nodep->doingWidth(خطأ);
    }
    فراغ visit(Ast_گرێ_Var_Ref* nodep) override {
        إذا (nodep->didWidth()) أرجع;
        إذا (!nodep->varp()) {
            إذا (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, خطأ,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                أرجع;
            } وإلا {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        إذا (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        إذا (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            أرجع;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        إذا (VN_IS(nodep->backp(), گرێ_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        إذا (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(true);
        إذا (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } وإلا إذا (nodep->access().isWriteOrRW() && nodep->varp()->is_تێخستن()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ثابت منطقي hierRef = !m_curModVars.ژماردن(nodep->varp());
            ثابت منطقي netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            ثابت منطقي contAssign = VN_IS(nodep->abovep(), AssignW);
            ثابت منطقي pinConn = VN_IS(nodep->abovep(), Pin);
            إذا (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } وإلا إذا (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        إذا (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(true);
    }

    فراغ visit(AstEnumDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_گرێ_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_گرێ_DType* ثابت badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        إذا (badDtp) {
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
        V3_ژمار ژمارە(nodep, nodep->width(), 0);
        ثابت V3_ژمار one{nodep, nodep->width(), 1};
        منطقي wrapAround = خطأ;
        std::map<ثابت V3_ژمار, AstEnumItem*> inits;
        لكل (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            إذا (itemp->valuep()) {
                إذا (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_درەخت("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                إذا (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_درەخت();
                    استمر;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = خطأ;  // Explicit value resets wrap-around tracking
            }
            إذا (!itemp->valuep()) {
                إذا (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                إذا (ژمارە.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(جديد AstConst{itemp->fileline(), ژمارە});
            }

            ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
            إذا (constp->ژمارە().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            ژمارە.opAssign(constp->ژمارە());
            // Look for duplicates
            ثابت auto pair = inits.emplace(ژمارە, itemp);
            إذا (!pair.second) {  // IEEE says illegal
                ثابت Ast_گرێ* ثابت otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ثابت منطقي prevNeg = constp->ژمارە().isNegative();
            ژمارە.opAdd(one, constp->ژمارە());
            إذا (basicp->isSigned()) {
                wrapAround = !prevNeg && ژمارە.isNegative();
            } وإلا {
                wrapAround = ژمارە.isEqZero();
            }
        }
    }
    فراغ visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_گرێ_DType* ثابت vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        إذا (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            منطقي warnOn = true;
            Ast_گرێ_دەربڕین* valuep = nodep->valuep();
            إذا (ثابت AstAdd* ثابت anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                إذا (anodep->fileline()->equal_First_هێڵ_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                إذا (static_cast<صحيح>(constp->ژمارە().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = خطأ;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(جديد AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    فراغ visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        إذا (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_گرێ* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            لكل (; enump; enump = enump->backp()) {
                إذا (VN_IS(enump, EnumDType)) اكسر;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    فراغ visit(Ast_گرێ_Cons* nodep) override {}  // Widthed when created
    فراغ visit(Ast_گرێ_Cons_Item* nodep) override {}  // Widthed when created
    فراغ visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_گرێ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_گرێ_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        إذا (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } وإلا إذا (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } وإلا {
            // Take width as maximum across all items
            صحيح width = nodep->exprp()->width();
            صحيح mwidth = nodep->exprp()->widthMin();
            منطقي isFourstate = nodep->exprp()->dtypep()->isFourstate();
            لكل (ثابت Ast_گرێ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        لكل (Ast_گرێ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            إذا (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } وإلا {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        إذا (m_constraintp) {
            منطقي canLower = true;
            لكل (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                إذا (VN_IS(ditemp->rangep(), InsideRange)) استمر;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ثابت Ast_گرێ_DType* ثابت itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                إذا (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = خطأ;
                    اكسر;
                }
            }
            إذا (canLower) أرجع;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_گرێ_دەربڕین* newp = nullptr;
        لكل (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_گرێ_دەربڕین* inewp
                = insideItem(nodep, nodep->exprp()->clone_درەخت_Pure(خطأ), itemp->rangep());
            إذا (!inewp) استمر;
            Ast_گرێ_دەربڕین* ثابت cmpp
                = جديد AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            جديد AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, true);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, true);
            inewp = جديد AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? جديد AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        إذا (!newp) newp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
    }

    فراغ visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_گرێ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_گرێ_DType* expDTypep = nullptr;

        إذا (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } وإلا إذا (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } وإلا {
            // Take width as maximum across all items
            صحيح width = nodep->exprp()->width();
            صحيح mwidth = nodep->exprp()->widthMin();
            منطقي isFourstate = nodep->exprp()->dtypep()->isFourstate();
            لكل (ثابت Ast_گرێ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            ثابت VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        لكل (Ast_گرێ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            إذا (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } وإلا إذا (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_گرێ_دەربڕین* exprp;
        Ast_دەربڕین_Stmt* دەربڕین_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        إذا (!m_constraintp && !nodep->exprp()->isPure()) {
            File_هێڵ* ثابت fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_گرێ_DType* ثابت دەربڕین_Dtp = nodep->exprp()->dtypep();
            ثابت صحيح w = دەربڕین_Dtp->width();
            Ast_گرێ_DType* ثابت tempDTypep
                = دەربڕین_Dtp->widthSized() ? دەربڕین_Dtp
                                        : nodep->findBitOrLogicDType(w, w, دەربڕین_Dtp->numeric(),
                                                                     دەربڕین_Dtp->isFourstate());
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = جديد AstVarRef{fl, varp, VAccess::READ};
            دەربڕین_Stmtp = جديد Ast_دەربڕین_Stmt{fl,
                                        جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            إذا (m_ftaskp) {
                varp->funcLocal(true);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } وإلا {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(دەربڕین_Stmtp);
        } وإلا {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_گرێ_دەربڕین* newp = nullptr;
        لكل (Ast_گرێ_دەربڕین *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), گرێ_دەربڕین);  // Will be unlinking
            Ast_گرێ_دەربڕین* inewp;
            إذا (دەربڕین_Stmtp) {
                inewp = insideItem(nodep, دەربڕین_Stmtp, itemp);
                دەربڕین_Stmtp = nullptr;
            } وإلا {
                inewp = insideItem(nodep, exprp->clone_درەخت_Pure(خطأ), itemp);
            }
            إذا (!inewp) استمر;
            newp = newp ? جديد AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        إذا (دەربڕین_Stmtp) VL_DO_DANGLING(دەربڕین_Stmtp->delete_درەخت(), دەربڕین_Stmtp);
        إذا (!newp) newp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
    }
    Ast_گرێ_دەربڕین* insideItem(Ast_گرێ* nodep, Ast_گرێ_دەربڕین* exprp, Ast_گرێ_دەربڕین* itemp) {
        ثابت Ast_گرێ_DType* ثابت itemDtp = itemp->dtypep()->skipRefp();
        إذا (AstInsideRange* ثابت irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            أرجع irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } وإلا إذا (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_گرێ_دەربڕین* ثابت inewp = جديد AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(true);
            أرجع inewp;
        } وإلا إذا (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_درەخت(), exprp);
            أرجع nullptr;
        }
        أرجع AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    فراغ visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        إذا (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } وإلا {
            إذا (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            إذا (m_vup->final()) {
                Ast_گرێ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ثابت صحيح expWidth = expDTypep->width();
                ثابت منطقي waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                ثابت منطقي waiveRhs = expWidth == 32
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

    فراغ visit(AstIfaceRefDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        إذا (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ visit(Ast_گرێ_UOr_Struct_DType* nodep) override {
        إذا (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(خطأ);
            أرجع;
        }
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(true);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        إذا (ثابت AstUnionDType* ثابت unionp = VN_CAST(nodep, UnionDType)) {
            إذا (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(خطأ);
        // Error checks
        لكل (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گرێ_DType* ثابت dtp = itemp->subDTypep()->skipRefp();
            إذا (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            إذا ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                پاڵنان_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        ثابت منطقي isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ثابت منطقي inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        إذا (VN_IS(nodep, UnionDType) || nodep->packed()) {
            صحيح lsb = 0;
            صحيح width = 0;
            منطقي first = true;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            لكل (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            لكل (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                إذا (itemp->isFourstate()) nodep->isFourstate(true);
                itemp->lsb(lsb);
                إذا (VN_IS(nodep, UnionDType)) {
                    ثابت صحيح itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    إذا (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } وإلا {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = خطأ;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } وإلا {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(خطأ);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    فراغ visit(AstThisRef* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        إذا (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    فراغ visit(AstClassRefDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    فراغ visit(AstClassOrPackageRef* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        userIterateChildren(nodep, nullptr);
    }
    فراغ visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
    }
    فراغ visit(AstClassExtends* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        إذا (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    فراغ visit(AstMemberDType* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        إذا (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    فراغ visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        إذا (nodep->didWidth()) أرجع;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = true;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_گرێ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        ثابت AstMemberSel* ثابت fromSel = VN_CAST(nodep->fromp(), MemberSel);
        إذا (AstClocking* ثابت clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            إذا (AstVar* ثابت varp = memberSelClocking(nodep, clockingp)) {
                إذا (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_درەخت(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                إذا (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                إذا (AstIfaceRefDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(true);  // Must not visit again: will confuse scopes
                أرجع;
            }
        } وإلا إذا (Ast_گرێ_UOr_Struct_DType* ثابت adtypep = VN_CAST(fromDtp, گرێ_UOr_Struct_DType)) {
            إذا (memberSelStruct(nodep, adtypep)) أرجع;
        } وإلا إذا (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            إذا (memberSelClass(nodep, adtypep)) أرجع;
        } وإلا إذا (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            إذا (Ast_گرێ* foundp = memberSelIface(nodep, adtypep)) {
                إذا (AstClocking* ثابت clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                إذا (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    إذا (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* ثابت ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ثابت AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
                    إذا (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ثابت newp = جديد AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(true);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                        أرجع;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(true);
                    أرجع;
                }
                إذا (AstModport* ثابت modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ثابت newDtypep = جديد AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_گرێ_دەربڕین* ثابت fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع;
                }
                إذا (AstCell* ثابت cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    إذا (VN_IS(cellp->modp(), Iface)) {
                        ثابت string viftopName = cellp->name() + "__Viftop";
                        Ast_گرێ_Module* ثابت parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        لكل (Ast_گرێ* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            إذا (AstVar* ثابت vp = VN_CAST(itemp, Var)) {
                                إذا (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    اكسر;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        إذا (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_گرێ_DType* subDtypep = viftopVarp->dtypep();
                        إذا (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            إذا (AstIfaceRefDType* ثابت subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ثابت newDtypep = جديد AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(true);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(true);
                        أرجع;
                    }
                }
                إذا (Ast_گرێ_FTask* ثابت ftaskp = VN_CAST(foundp, گرێ_FTask)) {
                    AstMethodCall* ثابت newp = جديد AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    أرجع;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } وإلا إذا (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_گرێ* ثابت newp = جديد AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            أرجع;
        } وإلا {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
    }
    منطقي memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        إذا (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* ثابت newp = جديد AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            visit(newp);
            أرجع true;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        لكل (AstClass* classp = first_classp; classp;) {
            إذا (Ast_گرێ* ثابت foundp = m_memberMap.findMember(classp, nodep->name())) {
                إذا (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    إذا (!varp->didWidth()) userIterate(varp, nullptr);
                    إذا (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ثابت varRefp
                            = جديد AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                        أرجع true;
                    }
                    إذا (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(true);
                    إذا (nodep->fromp()->same_درەخت(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    أرجع true;
                }
                إذا (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(جديد AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع true;
                }
                إذا (AstEnumItemRef* ثابت adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_درەخت(خطأ));
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع true;
                }
                إذا (Ast_گرێ_FTask* ftaskp = VN_CAST(foundp, گرێ_FTask)) {
                    AstMethodCall* newp = جديد AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع true;
                }
                إذا (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    أرجع true;
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
        لكل (AstClass* classp = first_classp; classp;) {
            لكل (Ast_گرێ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.پاڵنان_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        ثابت string suggest = speller.best_Candidate_پەیام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        أرجع خطأ;  // Caller handles error
    }
    منطقي modportAllowsItem(Ast_گرێ* nodep, ثابت AstIfaceRefDType* adtypep, ثابت Ast_گرێ* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ثابت AstModport* ثابت modportp = adtypep->modportp();
        إذا (!modportp) أرجع true;
        إذا (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) {
            إذا (varp->isParam()) أرجع true;
        } وإلا إذا (!VN_IS(itemp, گرێ_FTask) && !VN_IS(itemp, Clocking)) {
            أرجع true;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        إذا (pair.second) {
            لكل (ثابت Ast_گرێ* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.زیادکردن(mpItemp->name());
            }
        }
        إذا (pair.first->second.ژماردن(itemp->name())) أرجع true;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        أرجع خطأ;
    }
    Ast_گرێ* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_گرێ_Module* ثابت ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        لكل (Ast_گرێ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            إذا (itemp->name() == nodep->name()) {
                إذا (!modportAllowsItem(nodep, adtypep, itemp)) {
                    أرجع nullptr;  // Caller handles error
                }
                أرجع itemp;
            }
            إذا (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.پاڵنان_Candidate(itemp->prettyName());
            }
        }
        ثابت string suggest = speller.best_Candidate_پەیام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        أرجع nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        لكل (Ast_گرێ* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                إذا (citemp->varp()->name() == nodep->name()) أرجع citemp->varp();
                speller.پاڵنان_Candidate(citemp->varp()->prettyName());
            }
        }
        ثابت string suggest = speller.best_Candidate_پەیام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        أرجع nullptr;  // Caller handles error
    }
    منطقي memberSelStruct(AstMemberSel* nodep, Ast_گرێ_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        إذا (AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            إذا (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } وإلا إذا (adtypep->packed()) {
                AstSel* ثابت newp = جديد AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(true);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } وإلا {
                AstStructSel* ثابت newp = جديد AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(true);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            أرجع true;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        أرجع خطأ;
    }

    فراغ visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    فراغ visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        إذا (nodep->didWidth()) أرجع;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_گرێ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* ثابت basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        إذا (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } وإلا إذا (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } وإلا إذا (AstEnumDType* ثابت adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } وإلا إذا (AstAssocArrayDType* ثابت adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } وإلا إذا (AstWildcardArrayDType* ثابت adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } وإلا إذا (AstDynArrayDType* ثابت adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } وإلا إذا (AstQueueDType* ثابت adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } وإلا إذا (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } وإلا إذا (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } وإلا إذا (AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } وإلا إذا (AstConstraintRefDType* ثابت adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } وإلا إذا (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } وإلا إذا (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_گرێ_FTask_Ref* nodep, منطقي required, منطقي arbReturn,
                              Ast_گرێ_DType* returnDtp, Ast_گرێ_DType* indexDtp,
                              Ast_گرێ_DType* بەها_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        إذا (AstWith* ثابت withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->بەها_Arg_Refp()->dtypep(بەها_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            أرجع withp;
        }
        إذا (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        أرجع nullptr;
    }
    فراغ methodOkArguments(Ast_گرێ_FTask_Ref* nodep, صحيح minArg, صحيح maxArg) {
        صحيح narg = 0;
        إذا (AstWith* ثابت withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(پاڵنان_Deletep(withp->unlinkFrBack()), withp);
        }
        لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ثابت منطقي ok = (narg >= minArg) && (narg <= maxArg);
        إذا (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            لكل (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    جديد AstArg{nodep->fileline(), "",
                               جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            لكل (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                طالما (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_درەخت(), argp);
            }
        }
    }

    Ast_گرێ_دەربڕین* methodArg(AstMethodCall* nodep, صحيح arg) {
        AstArg* argp = nodep->argsp();
        لكل (صحيح narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        أرجع argp->exprp();
    }

    فراغ methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        إذا (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            إذا (nodep->name() == "num") {
                صحيح کاڵاکان = 0;
                لكل (Ast_گرێ* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++کاڵاکان;
                newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{}, کاڵاکان);
            } وإلا إذا (nodep->name() == "first") {
                ثابت AstEnumItem* itemp = adtypep->itemsp();
                إذا (!itemp) {
                    newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } وإلا {
                    newp = VN_AS(itemp->valuep()->clone_درەخت(خطأ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } وإلا إذا (nodep->name() == "last") {
                ثابت AstEnumItem* itemp = adtypep->itemsp();
                طالما (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                إذا (!itemp) {
                    newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } وإلا {
                    newp = VN_AS(itemp->valuep()->clone_درەخت(خطأ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        } وإلا إذا (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            إذا (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } وإلا إذا (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } وإلا إذا (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }

            إذا (nodep->name() != "name" && nodep->argsp()) {
                Ast_گرێ_دەربڕین* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, true);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                إذا (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_گرێ_دەربڕین* ثابت newp = جديد AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ثابت uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                إذا (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* ثابت newp = جديد AstMethodCall{
                        nodep->fileline(),
                        جديد AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
                    أرجع;
                } وإلا إذا (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    پاڵنان_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* ثابت clonep = nodep->clone_درەخت(خطأ);
                    VN_AS(stepp, Const)->ژمارە().setLong(1);
                    AstConst* ثابت constp = جديد AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* ثابت argp = جديد AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* ثابت newp
                        = جديد AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
                    أرجع;
                }
            }
            Ast_گرێ_دەربڕین* ثابت newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    فراغ methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        إذا (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } وإلا إذا (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } وإلا إذا (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_گرێ_دەربڕین* ثابت index_exprp = method_Call_Wildcard_Index_دەربڕین(nodep, adtypep);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } وإلا إذا (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            إذا (!nodep->argsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } وإلا {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_گرێ_دەربڕین* ثابت index_exprp = method_Call_Wildcard_Index_دەربڕین(nodep, adtypep);
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } وإلا إذا (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = methodWithClause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp
                = methodWithClause(nodep, true, خطأ, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "map") {
            AstWith* ثابت withp
                = methodWithClause(nodep, true, خطأ, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        }
    }
    فراغ methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        إذا (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        إذا (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } وإلا إذا (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->کلیل_DTypep(), BOTH);
            Ast_گرێ_دەربڕین* ثابت index_exprp = method_Call_Assoc_Index_دەربڕین(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->کلیل_DTypep(), BOTH);
            Ast_گرێ_دەربڕین* ثابت index_exprp = method_Call_Assoc_Index_دەربڕین(nodep, adtypep);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } وإلا إذا (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            إذا (!nodep->argsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } وإلا {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->کلیل_DTypep(),
                                  BOTH);
                Ast_گرێ_دەربڕین* ثابت index_exprp = method_Call_Assoc_Index_دەربڕین(nodep, adtypep);
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } وإلا إذا (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp = methodWithClause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                                    adtypep->کلیل_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = methodWithClause(
                nodep, خطأ, true, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            إذا (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->کلیل_DTypep()));
            } وإلا {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp = methodWithClause(nodep, true, خطأ, nodep->findBitDType(),
                                                    adtypep->کلیل_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp = methodWithClause(nodep, true, خطأ, nodep->findBitDType(),
                                                    adtypep->کلیل_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->کلیل_DTypep()));
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "map") {
            AstWith* ثابت withp = methodWithClause(nodep, true, خطأ, adtypep->subDTypep(),
                                                    adtypep->کلیل_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        }
    }
    Ast_گرێ_دەربڕین* method_Call_Assoc_Index_دەربڕین(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_گرێ* ثابت index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->کلیل_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        أرجع nodep->argsp()->exprp();
    }
    Ast_گرێ_دەربڕین* method_Call_Wildcard_Index_دەربڕین(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_گرێ* ثابت index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        أرجع nodep->argsp()->exprp();
    }
    فراغ methodCallLValueRecurse(AstMethodCall* nodep, Ast_گرێ* childp, ثابت VAccess& access) {
        إذا (AstCMethodHard* ثابت ichildp = VN_CAST(childp, CMethodHard)) {
            منطقي methAt = خطأ;
            منطقي methAtBack = خطأ;
            منطقي methAtWrite = خطأ;
            بدل (ichildp->method()) {
            حالة VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = true;
                اكسر;
            حالة VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = true;
                اكسر;
            حالة VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            حالة VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            حالة VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = true;
                اكسر;
            افتراضي: اكسر;
            }
            إذا (methAt || methAtBack || methAtWrite) {
                ثابت Ast_گرێ_DType* ثابت fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                إذا (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    إذا (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } وإلا إذا (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                أرجع;
            }
        }
        إذا (Ast_گرێ_Var_Ref* ثابت varrefp = VN_CAST(childp, گرێ_Var_Ref)) {
            varrefp->access(access);
        } وإلا إذا (ثابت AstMemberSel* ثابت ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } وإلا إذا (ثابت AstStructSel* ثابت ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } وإلا إذا (ثابت Ast_گرێ_Sel* ثابت ichildp = VN_CAST(childp, گرێ_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } وإلا إذا (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } وإلا {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_گرێ_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        إذا (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            إذا (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, خطأ, true, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = methodWithClause(
                nodep, خطأ, true, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            إذا (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } وإلا {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ثابت withp
                = methodWithClause(nodep, true, خطأ, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp
                = methodWithClause(nodep, true, خطأ, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ثابت withp
                = methodWithClause(nodep, true, خطأ, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        أرجع newp;
    }
    فراغ methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        إذا (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } وإلا إذا (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = methodWithClause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا ((newp = methodCallArray(nodep, adtypep))) {
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        }
    }
    فراغ methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        إذا (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } وإلا إذا (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            إذا (!nodep->argsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } وإلا {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_گرێ_دەربڕین* ثابت index_exprp = method_Call_Queue_Index_دەربڕین(nodep);
                إذا (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_دەرهێنان_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } وإلا {
                    newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } وإلا إذا (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_گرێ_دەربڕین* ثابت index_exprp = method_Call_Queue_Index_دەربڕین(nodep);
            AstArg* ثابت argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            إذا (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_پاڵنان_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } وإلا {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_زیادکردن, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } وإلا إذا (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ثابت argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ثابت withp
                = methodWithClause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وإلا إذا ((newp = methodCallArray(nodep, adtypep))) {
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        }
    }
    Ast_گرێ_دەربڕین* method_Call_Queue_Index_دەربڕین(AstMethodCall* nodep) {
        Ast_گرێ* ثابت index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        أرجع nodep->argsp()->exprp();
    }
    فراغ methodCallWarnTiming(Ast_گرێ_FTask_Ref* ثابت nodep, ثابت std::string& className) {
        إذا (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } وإلا إذا (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    فراغ methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ثابت ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        إذا (Ast_گرێ_FTask* ثابت ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), گرێ_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            إذا (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                أرجع;
            }
            إذا (adtypep->isVirtual()) {
                لكل (Ast_گرێ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    إذا (AstVar* ثابت mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            إذا (ftaskp->isStatic()) {
                AstArg* ثابت argsp = nodep->argsp();
                إذا (argsp) argsp->unlinkFrBackWithNext();
                Ast_گرێ_FTask_Ref* newp = nullptr;
                إذا (VN_IS(ftaskp, Task)) {
                    newp = جديد AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } وإلا {
                    newp = جديد AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
            } وإلا {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                إذا (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            أرجع;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    فراغ handleRandomizeArgs(Ast_گرێ_FTask_Ref* ثابت nodep, AstClass* ثابت classp) {
        منطقي hasNonNullArgs = خطأ;
        AstConst* nullp = nullptr;
        لكل (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_گرێ_دەربڕین* exprp = argp->exprp();
            إذا (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                إذا (constp->ژمارە().isNull()) {
                    nullp = constp;
                    استمر;
                }
            }
            hasNonNullArgs = true;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            إذا (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_گرێ_دەربڕین* fromp = methodCallp->fromp();
                طالما (AstMemberSel* ثابت memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ثابت varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            إذا (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_درەخت(), argp);
                استمر;
            }
            طالما (exprp) {
                إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } وإلا {
                    إذا (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } وإلا {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_درەخت(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                إذا (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            إذا (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } وإلا إذا (classp->existsMember([&](ثابت AstClass*, ثابت AstVar* memberVarp) {
                           أرجع memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } وإلا {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_درەخت(), argp);
            }
        }
        إذا (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    فراغ methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        إذا (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, خطأ, خطأ, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                إذا (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } وإلا إذا (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        لكل (AstClass* classp = first_classp; classp;) {
            إذا (nodep->fileline()->timingOn()) {
                إذا (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ثابت packagep = getItemPackage(classp);
                    // Check if it's std
                    إذا (packagep && packagep->name() == "std") {
                        إذا (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } وإلا إذا (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } وإلا إذا (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            إذا (Ast_گرێ_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), گرێ_FTask)) {
                userIterate(ftaskp, nullptr);
                إذا (ftaskp->isStatic()) {
                    AstArg* ثابت argsp = nodep->argsp();
                    إذا (argsp) argsp->unlinkFrBackWithNext();
                    Ast_گرێ_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    إذا (!m_vup) {  // Called as task
                        newp = جديد AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } وإلا {
                        newp = جديد AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
                } وإلا {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    إذا (VN_IS(ftaskp, Task)) {
                        إذا (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_دەربڕین)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    إذا (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                أرجع;
            } وإلا إذا (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(true);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                أرجع;
            } وإلا إذا (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_گرێ_دەربڕین* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_گرێ_دەربڕین* ثابت exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(true);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                أرجع;
            } وإلا إذا (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(true);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            لكل (AstClass* classp = first_classp; classp;) {
                لكل (Ast_گرێ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    إذا (VN_IS(itemp, گرێ_FTask)) speller.پاڵنان_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            ثابت string suggest = speller.best_Candidate_پەیام(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    فراغ methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        إذا (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            إذا (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } وإلا {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(true);
        } وإلا {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        }
    }
    فراغ methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        إذا (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } وإلا {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(true);
    }
    فراغ methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        تعداد : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        إذا (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } وإلا إذا (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } وإلا إذا (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } وإلا إذا (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } وإلا إذا (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        إذا (methodId) {
            AstWith* ثابت withp
                = methodWithClause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            إذا (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                إذا (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(خطأ);
                newp->didWidth(true);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
                أرجع;
            } وإلا {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_هێڵ* ثابت fl = nodep->fileline();
                Ast_گرێ_دەربڕین* newp = nullptr;
                لكل (صحيح i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_گرێ_دەربڕین* ثابت arrayRef = nodep->fromp()->clone_درەخت_Pure(خطأ);
                    Ast_گرێ_دەربڕین* ثابت selector = جديد AstArraySel{fl, arrayRef, i};
                    إذا (!newp) {
                        newp = selector;
                    } وإلا {
                        بدل (methodId) {
                        حالة ARRAY_OR: newp = جديد AstOr{fl, newp, selector}; اكسر;
                        حالة ARRAY_AND: newp = جديد AstAnd{fl, newp, selector}; اكسر;
                        حالة ARRAY_XOR: newp = جديد AstXor{fl, newp, selector}; اكسر;
                        حالة ARRAY_SUM: newp = جديد AstAdd{fl, newp, selector}; اكسر;
                        حالة ARRAY_PRODUCT: newp = جديد AstMul{fl, newp, selector}; اكسر;
                        افتراضي: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
            }
        } وإلا إذا (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(خطأ);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    فراغ methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        إذا (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    فراغ methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        إذا (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_گرێ* ثابت newp = جديد AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        } وإلا إذا (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } وإلا إذا (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } وإلا إذا (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } وإلا إذا (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } وإلا إذا (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } وإلا إذا (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_گرێ* ثابت newp
                = جديد AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا إذا (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_گرێ* ثابت newp
                = جديد AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا إذا (nodep->name() == "compare" || nodep->name() == "icompare") {
            ثابت منطقي ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ثابت argp = nodep->argsp();
            Ast_گرێ_دەربڕین* ثابت lhs = nodep->fromp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت rhs = argp->exprp()->unlinkFrBack();
            Ast_گرێ* ثابت newp = جديد AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا إذا (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_گرێ_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_گرێ_دەربڕین* ثابت rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* ثابت varrefp
                = جديد AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_گرێ* ثابت newp = جديد AstAssign{
                nodep->fileline(), fromp, جديد AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            پاڵنان_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } وإلا إذا (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            Ast_گرێ_دەربڕین* ثابت lhsp = nodep->fromp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا إذا (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_گرێ_دەربڕین* ثابت lhsp = nodep->fromp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت thsp = arg1p->exprp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا إذا (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            إذا (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } وإلا إذا (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } وإلا إذا (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } وإلا إذا (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } وإلا إذا (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } وإلا {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_گرێ* ثابت newp
                = جديد AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_گرێ_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        إذا (AstQueueDType* ثابت queuep = m_queueDTypeIndexed[indexDTypep]) {
            أرجع queuep;
        } وإلا {
            AstQueueDType* ثابت newp
                = جديد AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            أرجع newp;
        }
    }

    فراغ visit(AstNew* nodep) override {
        إذا (nodep->didWidth()) أرجع;
        AstClass* classp = nullptr;
        منطقي assign = خطأ;
        إذا (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = true;
            Ast_گرێ* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            إذا (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                إذا (AstClass* ثابت clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ثابت adtypep
                        = جديد AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } وإلا {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            إذا (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, گرێ_DType)
                           ? ("data type "s + VN_AS(warnp, گرێ_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                أرجع;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            إذا (Ast_گرێ_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } وإلا {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } وإلا {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        إذا (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    فراغ visit(AstNewCopy* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        AstClassRefDType* ثابت refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        إذا (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            أرجع;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        إذا (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    فراغ visit(AstNewDynamic* nodep) override {
        إذا (nodep->didWidthAndSet()) أرجع;
        AstDynArrayDType* ثابت adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        إذا (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            أرجع;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        إذا (!VN_IS(nodep->backp(), گرێ_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            أرجع;
        }
        nodep->dtypep(adtypep);
        إذا (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        إذا (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ثابت consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            إذا (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            }
        }
    }

    فراغ visit(Ast_Tagged_دەربڕین* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    فراغ visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    فراغ visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    فراغ visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    فراغ visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    فراغ visit(AstPattern* nodep) override {
        إذا (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = خطأ;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                إذا (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), گرێ_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                لكل (Ast_گرێ *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            أرجع;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = خطأ;
        إذا (nodep->didWidthAndSet()) أرجع;
        UINFO(9, "PATTERN " << nodep);
        إذا (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        إذا (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_گرێ_DType* dtypep = nodep->dtypep();
        إذا (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            إذا (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                أرجع;
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            أرجع;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            إذا (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                أرجع;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ثابت صحيح times = visitPatMemberRep(patp);
                لكل (صحيح i = 1; i < times; i++) {
                    AstPatMember* ثابت newp = patp->clone_درەخت(خطأ);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                إذا (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    طالما (Ast_گرێ_دەربڕین* ثابت movep = VN_AS(patp->lhssp()->nextp(), گرێ_دەربڕین)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_گرێ* newkeyp = nullptr;
                        إذا (patp->keyp()) newkeyp = patp->keyp()->clone_درەخت(true);
                        AstPatMember* ثابت newp
                            = جديد AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ثابت nextp = VN_AS(patp->nextp(), PatMember);
                إذا (patp->isDefault()) {
                    إذا (defaultp) {
                        VL_DO_DANGLING(پاڵنان_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            طالما (ثابت AstConstDType* ثابت vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            إذا (auto* ثابت vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } وإلا إذا (auto* ثابت vdtypep = VN_CAST(dtypep, گرێ_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ثابت consp = جديد AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ثابت consp = جديد AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } وإلا {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            إذا (defaultp) VL_DO_DANGLING(پاڵنان_Deletep(defaultp), defaultp);
        }
    }
    فراغ patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<ثابت AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ثابت AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            طالما (patp) {
                افعل {
                    إذا (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        إذا (ثابت AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            إذا (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                اكسر;
                            } وإلا {
                                ثابت std::pair<PatMap::iterator, منطقي> ret
                                    = patmap.emplace(memp, patp);
                                إذا (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } وإلا إذا (Ast_گرێ_DType* nodedtypep = VN_CAST(patp->keyp(), گرێ_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            ثابت string dtype = nodedtypep->dtypep()->prettyDTypeName(true);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } وإلا {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } وإلا إذا (memp) {
                        // constant expr
                        ثابت std::pair<PatMap::iterator, منطقي> ret = patmap.emplace(memp, patp);
                        إذا (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } وإلا {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } طالما (خطأ);

                // Next
                إذا (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_گرێ_دەربڕین* newp = nullptr;
        إذا (vdtypep->packed()) {
            لكل (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت auto it = patmap.find(memp);
                إذا (it == patmap.end()) {  // default or default_type assignment
                    إذا (AstStructDType* ثابت memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } وإلا {
                        AstPatMember* ثابت patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = بەها_Concat_pattern_Struct(patp, newp, memp, nodep);
                        إذا (patp) VL_DO_DANGLING(پاڵنان_Deletep(patp), patp);
                    }
                } وإلا {  // member assignment
                    newp = بەها_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } وإلا {  // Unpacked
            AstConsStruct* ثابت consp = جديد AstConsStruct{nodep->fileline(), vdtypep};
            لكل (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                إذا (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    پاڵنان_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    پاڵنان_Deletep(patp);  // patp used below
                } وإلا {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_گرێ_دەربڕین* ثابت valuep = pattern_Member_بەها_Iterate(patp);
                consp->addItemsp(جديد AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        إذا (newp) {
            nodep->replaceWith(newp);
        } وإلا {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_گرێ_دەربڕین* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_گرێ_دەربڕین* newp,
                                                 AstPattern* nodep, ثابت DTypeMap& dtypemap) {
        لكل (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            إذا (AstStructDType* ثابت memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } وإلا {
                AstPatMember* ثابت patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = بەها_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                إذا (patp) VL_DO_DANGLING(پاڵنان_Deletep(patp), patp);
            }
        }
        أرجع newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ثابت DTypeMap& dtypemap) {
        ثابت string memp_DType = memp->virtRefDTypep()->prettyDTypeName(true);
        ثابت auto it = dtypemap.find(memp_DType);
        إذا (it != dtypemap.end()) {
            // default_value for data_type
            أرجع it->second->clone_درەخت(خطأ);
        }
        إذا (defaultp) {
            // default_value for any unmatched member yet
            أرجع defaultp->clone_درەخت(خطأ);
        }
        إذا (memp->valuep()) {
            أرجع جديد AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_درەخت(خطأ), گرێ_دەربڕین),
                                    جديد AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        أرجع nullptr;
    }

    Ast_گرێ_دەربڕین* بەها_Concat_pattern_Struct(AstPatMember* patp, Ast_گرێ_دەربڕین* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        إذا (patp) {
            patp->dtypep(memp);
            Ast_گرێ_دەربڕین* ثابت valuep = pattern_Member_بەها_Iterate(patp);
            إذا (!newp) {
                newp = valuep;
            } وإلا {
                AstConcat* ثابت concatp = جديد AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, true);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, true);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        أرجع newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_گرێ_DType* elemDTypep,
                                       ثابت DTypeMap& dtypemap = DTypeMap{}) {
        Ast_گرێ_دەربڕین* ثابت valuep = defaultp->lhssp()->clone_درەخت(خطأ);
        Ast_گرێ_DType* ثابت elemDTypeSkipRefp = elemDTypep->skipRefp();
        ثابت AstStructDType* ثابت structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ثابت منطقي unpackedStruct = structp && !structp->packed();

        إذا (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(پاڵنان_Deletep(valuep), valuep);
            أرجع defaultp->clone_درەخت(خطأ);
        }
        إذا (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(پاڵنان_Deletep(valuep), valuep);
            أرجع defaultp->clone_درەخت(خطأ);
        }
        إذا (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        منطقي wholeElement = unpackedStruct;
        إذا (Ast_گرێ_DType* ثابت بەها_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !بەها_DTypep->skipRefp()->isIntegralOrPacked()
                                          : بەها_DTypep->similarDType(elemDTypeSkipRefp);
        }
        إذا (wholeElement) {
            VL_DO_DANGLING(پاڵنان_Deletep(valuep), valuep);
            أرجع defaultp->clone_درەخت(خطأ);
        }

        AstPatMember* ثابت nestedDefaultp
            = جديد AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(true);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        لكل (ثابت auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_درەخت(خطأ));
        AstPattern* ثابت recursivePatternp = جديد AstPattern{defaultp->fileline(), nestedDefaultp};
        أرجع جديد AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_گرێ_دەربڕین* patternDefaultValuep(AstPatMember* defaultp, Ast_گرێ_DType* elemDTypep) {
        إذا (!defaultp) أرجع nullptr;
        AstPatMember* ثابت elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_گرێ_دەربڕین* ثابت valuep = pattern_Member_بەها_Iterate(elemDefaultp);
        VL_DO_DANGLING(پاڵنان_Deletep(elemDefaultp), elemDefaultp);
        أرجع valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    فراغ patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        ثابت VNumRange range = arrayDtp->declRange();
        ثابت صحيح elements = range.elements();
        AstConsArray* ثابت newp = جديد AstConsArray{nodep->fileline(), arrayDtp};
        صحيح entn = 0;  // Number of elements given so far, from the left
        منطقي error = خطأ;  // Reported an error, so the value is not complete
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_گرێ_دەربڕین* ثابت valuep = pattern_Member_بەها_Iterate(patp);
            ثابت uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            ثابت AstUnpackArrayDType* ثابت spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            إذا (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_بەهاکان(index, valuep);
                استمر;
            }
            ثابت VCastable castable = Ast_گرێ::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            إذا (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = true;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        إذا (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = true;
        }
        إذا (error) {
            VL_DO_DANGLING(پاڵنان_Deletep(newp), newp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } وإلا {
            nodep->replaceWith(newp);
        }
    }
    فراغ patternArray(AstPattern* nodep, Ast_گرێ_Array_DType* arrayDtp, AstPatMember* defaultp) {
        إذا (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            أرجع;
        }
        ثابت VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.چەپ() << " to " << range.ڕاست());
        Ast_گرێ* newp = nullptr;
        منطقي missed = خطأ;  // Some element missed, so not a complete value
        لكل (صحيح entn = 0, ent = range.چەپ(); entn < range.elements();
             ++entn, ent += range.چەپ_To_ڕاست_Inc()) {
            Ast_گرێ_دەربڕین* valuep = nullptr;
            ثابت auto it = patmap.find(ent);
            إذا (it == patmap.end()) {
                إذا (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } وإلا {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = true;
                }
            } وإلا {
                AstPatMember* ثابت patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_بەها_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ثابت AstUnpackArrayDType* ثابت memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                إذا (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(پاڵنان_Deletep(valuep), valuep);
                    valuep = جديد AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            إذا (valuep) {
                إذا (VN_IS(arrayDtp, UnpackArrayDType)) {
                    إذا (!newp) newp = جديد AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } وإلا {  // Packed. Convert to concat for now.
                    إذا (!newp) {
                        newp = valuep;
                    } وإلا {
                        AstConcat* ثابت concatp
                            = جديد AstConcat{valuep->fileline(), VN_AS(newp, گرێ_دەربڕین), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        إذا (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        إذا (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(پاڵنان_Deletep(newp), newp);
            newp = جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        إذا (newp) {
            nodep->replaceWith(newp);
        } وإلا {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        إذا (VN_IS(patp->keyp(), گرێ_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            أرجع nullptr;
        }
        إذا (patp->کلیل_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->کلیل_Exprp());  // May change
        AstConst* ثابت constp = VN_CAST(patp->کلیل_Exprp(), Const);
        إذا (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        أرجع constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_گرێ_DType* کلیل_DTypep) {
        إذا (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            أرجع nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        إذا (کلیل_DTypep->skipRefp()->isIntegralOrPacked() && patp->کلیل_Exprp()) {
            userIterate(patp->کلیل_Exprp(), WidthVP{کلیل_DTypep, PRELIM}.p());
            ثابت صحيح width = std::max(patp->کلیل_Exprp()->width(), کلیل_DTypep->width());
            Ast_گرێ_DType* ثابت contextDTypep
                = کلیل_DTypep->findBitOrLogicDType(width, width, کلیل_DTypep->numeric(), true);
            iterateCheck(patp, "key", patp->کلیل_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, خطأ);
        }
        AstConst* ثابت constp = patternConstKeyp(patp);
        إذا (!constp) أرجع nullptr;
        إذا (constp->ژمارە().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            أرجع nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, کلیل_DTypep, EXTEND_EXP, خطأ);
        Ast_گرێ_دەربڕین* ثابت convp = patp->کلیل_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_گرێ_دەربڕین* ثابت keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        أرجع VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_گرێ_دەربڕین* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_گرێ_DType* arrayDtp) {
        Ast_گرێ_DType* ثابت کلیل_DTypep = VN_AS(arrayDtp, AssocArrayDType)->کلیل_DTypep();
        إذا (!کلیل_DTypep->skipRefp()->isAggregateType() || !patp->کلیل_Exprp()) {
            أرجع patternScalarKeyp(patp, کلیل_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->کلیل_Exprp(), کلیل_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->کلیل_Exprp());
        Ast_گرێ_دەربڕین* ثابت foldedp = patp->کلیل_Exprp();
        إذا (!foldedp->is_Constant_درەخت()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            أرجع nullptr;
        }
        إذا (foldedp->exists([](ثابت AstConst* constp) { أرجع constp->ژمارە().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            أرجع nullptr;
        }
        أرجع foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_گرێ_DType* arrayDtp) {
        أرجع patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_گرێ_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        أرجع patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    فراغ sort_Cons_کاڵاکان(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        طالما (T_Item* ثابت itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.پاڵنان_back(itemp);
        }
        ثابت auto کلیل_Of = [](ثابت T_Item* itemp) -> ثابت V3_ژمار& {
            ثابت Ast_گرێ* ثابت keyp = itemp->keyp();
            أرجع VN_AS(keyp, Const)->ژمارە();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](ثابت T_Item* ap, ثابت T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            أرجع کلیل_Of(ap).isLtXZ(کلیل_Of(bp));
        });
        ثابت T_Item* firstp = nullptr;  // First item with the current key
        لكل (T_Item* ثابت itemp : itemps) {
            إذا (firstp && کلیل_Of(firstp).isCaseEq(کلیل_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } وإلا {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    فراغ patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_گرێ_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ثابت auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_گرێ_دەربڕین* ثابت valuep = pattern_Member_بەها_Iterate(patp);
            إذا (keyp) {
                consp->addItemsp(جديد T_Item{patp->fileline(), keyp, valuep});
            } وإلا {
                VL_DO_DANGLING(پاڵنان_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ثابت AstAssocArrayDType* ثابت assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        إذا (!assocDtp || !assocDtp->کلیل_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_کاڵاکان<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_گرێ_دەربڕین* newConsDynArrayOrQueue(File_هێڵ* fileline, ثابت Ast_گرێ_DType* dtypep) {
        إذا (VN_IS(dtypep, DynArrayDType)) أرجع جديد AstConsDynArray{fileline};
        إذا (VN_IS(dtypep, QueueDType)) أرجع جديد AstConsQueue{fileline};
        أرجع nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    فراغ addConsItem(T_Cons* consp, Ast_گرێ_دەربڕین* valuep) {
        Ast_گرێ_DType* ثابت arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ثابت منطقي isElement
            = Ast_گرێ::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ثابت AstQueueDType* ثابت subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        إذا (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* ثابت subp = Ast_گرێ::cast<T_Cons>(valuep);
            إذا (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                إذا (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(پاڵنان_Deletep(subp), subp);
                أرجع;
            }
            إذا (AstConsQueue* ثابت queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_کاڵاکان(consp, queuep);
                أرجع;
            }
            إذا (AstConsDynArray* ثابت dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_کاڵاکان(consp, dynp);
                أرجع;
            }
        }
        T_Item* ثابت itemp = جديد T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        إذا (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } وإلا {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    فراغ addConsItem(Ast_گرێ_دەربڕین* consp, Ast_گرێ_دەربڕین* valuep) {
        إذا (AstConsDynArray* ثابت dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } وإلا {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    فراغ add_Cons_کاڵاکان(Ast_گرێ_دەربڕین* consp, T_Sub* subp) {
        طالما (auto* ثابت itemp = subp->itemsp()) {
            Ast_گرێ_دەربڕین* ثابت valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(پاڵنان_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(پاڵنان_Deletep(subp), subp);
    }
    فراغ patternDynArrayOrQueue(AstPattern* nodep, Ast_گرێ_DType* arrayp, AstPatMember* defaultp) {
        Ast_گرێ_دەربڕین* ثابت newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ثابت AstPatMember* ثابت firstp = VN_AS(nodep->itemsp(), PatMember);
        ثابت منطقي keyed = defaultp || (firstp && firstp->keyp());
        إذا (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            أرجع;
        }
        ثابت حرف* ثابت kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        إذا (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            إذا (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_بەها_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    فراغ patternBasic(AstPattern* nodep, Ast_گرێ_DType* vdtypep, AstPatMember* defaultp) {
        ثابت AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ثابت VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_گرێ_دەربڕین* newp = nullptr;
        لكل (ثابت صحيح ent : range.seqHiToLo()) {
            Ast_گرێ_دەربڕین* valuep = nullptr;
            ثابت auto it = patmap.find(ent);
            إذا (it == patmap.end()) {
                إذا (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } وإلا {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } وإلا {
                AstPatMember* ثابت patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_بەها_Iterate(patp);
            }
            إذا (valuep) {  // Packed. Convert to concat for now.
                إذا (!newp) {
                    newp = valuep;
                } وإلا {
                    AstConcat* ثابت concatp = جديد AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        إذا (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        إذا (newp) {
            nodep->replaceWith(newp);
        } وإلا {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_گرێ_دەربڕین* pattern_Member_بەها_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_گرێ_دەربڕین* valuep = patp->lhssp()->unlinkFrBack();
        إذا (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            إذا (AstConst* ثابت newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(پاڵنان_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        أرجع valuep;
    }

    ساكن فراغ checkEventAssignment(ثابت Ast_گرێ_Assign* ثابت asgnp) {
        string unsupEvtAsgn;
        إذا (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        إذا (VN_IS(asgnp->rhsp(), CReset)) أرجع;
        إذا (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        إذا (!unsupEvtAsgn.empty()) {
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

    ساكن منطقي usesDynamicScheduler(Ast_گرێ* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        طالما (true) {
            AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef);
            إذا (vrefp) أرجع usesDynamicScheduler(vrefp);
            إذا (VN_IS(nodep, MemberSel)) {
                أرجع true;
            } وإلا إذا (Ast_گرێ_Sel* selp = VN_CAST(nodep, گرێ_Sel)) {
                nodep = selp->fromp();
            } وإلا {
                أرجع خطأ;
            }
        }
    }

    ساكن منطقي usesDynamicScheduler(AstVarRef* vrefp) {
        أرجع VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    فراغ visit(AstPatMember* nodep) override {
        Ast_گرێ_DType* ثابت vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    صحيح visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        إذا (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ثابت AstConst* ثابت constp = VN_CAST(nodep->repp(), Const);
            إذا (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } وإلا {
                times = constp->toUInt();
            }
            إذا (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_درەخت();  // Done with replicate before cloning
        }
        أرجع times;
    }

    فراغ visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_گرێ* propp = nodep->propp();
            طالما (AstLogNot* ثابت notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ثابت untilp = VN_CAST(propp, Until);
            إذا (untilp && VN_IS(nodep->backp(), گرێ_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(جديد AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(پاڵنان_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            إذا (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    منطقي firstNewStatementOkRecurse(Ast_گرێ* nodep) {
        إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            إذا (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) أرجع true;
        }
        إذا (AstAssign* ثابت aitemp = VN_CAST(nodep, Assign)) {
            إذا (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) أرجع true;
        }
        إذا (AstInitialStaticStmt* ثابت aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            لكل (Ast_گرێ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (!firstNewStatementOkRecurse(stmtp)) أرجع خطأ;
            }
            أرجع true;
        }
        إذا (AstInitialAutomaticStmt* ثابت aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            لكل (Ast_گرێ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (!firstNewStatementOkRecurse(stmtp)) أرجع خطأ;
            }
            أرجع true;
        }
        أرجع خطأ;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    فراغ handleCaseType(Ast_گرێ* casep, Ast_گرێ_دەربڕین* exprp, CaseItem* itemsp) {
        AstAttrOf* ثابت exprap = VN_CAST(exprp, AttrOf);
        إذا (!exprap) أرجع;
        إذا (exprap->attrType() != VAttrType::TYPEID) أرجع;

        ثابت Ast_گرێ_DType* ثابت دەربڕین_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << دەربڕین_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_هێڵ* ثابت newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, true);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, true);  // Side effect of transform
        casep->fileline(newfl);
        لكل (CaseItem* itemp = itemsp; itemp; itemp = Ast_گرێ::as<CaseItem>(itemp->nextp())) {
            إذا (itemp->isDefault()) استمر;
            منطقي hit = خطأ;
            لكل (Ast_گرێ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ثابت AstAttrOf* ثابت condAttrp = VN_CAST(condp, AttrOf);
                إذا (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } وإلا {
                    Ast_گرێ_DType* ثابت condDtp = condAttrp->dtypep();
                    إذا (Ast_گرێ::computeCastable(دەربڕین_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = true;
                        اكسر;
                    }
                }
            }
            پاڵنان_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(جديد AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(جديد AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(پاڵنان_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    فراغ handleCase(Ast_گرێ* casep, Ast_گرێ_دەربڕین* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_گرێ_DType* subDTypep = exprp->dtypep();
        لكل (CaseItem* itemp = itemsp; itemp; itemp = Ast_گرێ::as<CaseItem>(itemp->nextp())) {
            لكل (Ast_گرێ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                إذا (condp->dtypep() == subDTypep) استمر;

                إذا (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } وإلا إذا (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } وإلا {
                    ثابت صحيح width = std::max(subDTypep->width(), condp->width());
                    ثابت صحيح mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    ثابت منطقي issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        لكل (CaseItem* itemp = itemsp; itemp; itemp = Ast_گرێ::as<CaseItem>(itemp->nextp())) {
            لكل (Ast_گرێ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    فراغ visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        لكل (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            لكل (Ast_گرێ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    فراغ visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    فراغ visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    فراغ visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        لكل (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            لكل (Ast_گرێ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_گرێ_DType* ثابت itemDTypep = nodep->findUInt32DType();
        لكل (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            لكل (Ast_گرێ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    فراغ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    فراغ visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    فراغ visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    فراغ visit(Ast_گرێ_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    فراغ visit(Ast_دەربڕین_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    فراغ visit(Ast_گرێ_Foreach* nodep) override {
        إذا (nodep->didWidth()) أرجع;
        nodep->didWidth(true);
        ثابت AstForeachHeader* ثابت headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_گرێ_دەربڕین* ثابت fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_گرێ_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        لكل (Ast_گرێ *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            ثابت منطقي empty = VN_IS(argsp, Empty);
            AstVar* ثابت varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            إذا (varp) varp->usedLoopIdx(true);
            إذا (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
                أرجع;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            إذا (VN_IS(fromDtp, گرێ_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } وإلا إذا (AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                إذا (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
                    أرجع;
                }
            } وإلا إذا (ثابت AstAssocArrayDType* ثابت adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->کلیل_DTypep());
            } وإلا {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
                أرجع;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_گرێ* ثابت bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        إذا (AstForeach* ثابت loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            أرجع;
        }
    }

    فراغ visit(Ast_گرێ_Assign* nodep) override {
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
            إذا (ثابت AstIfaceRefDType* ثابت irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                إذا (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
                    أرجع;
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
            Ast_گرێ_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            إذا (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ثابت Ast_گرێ_DType* ثابت lhsDtp = lhsDTypep->skipRefp();
                ثابت Ast_گرێ_DType* ثابت rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ثابت منطقي lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ثابت منطقي rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                إذا (lhsIsUnpackArray && rhsIsUnpackArray) {
                    إذا (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_گرێ_Var_Ref, Ast_گرێ_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), گرێ_Var_Ref), VN_CAST(nodep->rhsp(), گرێ_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                إذا (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), گرێ_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        إذا (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        إذا (Ast_گرێ* ثابت controlp = nodep->timingControlp()) {
            إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_درەخت(), controlp);
            } وإلا إذا (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } وإلا {
                إذا (nodep->fileline()->timingOn()) {
                    إذا (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } وإلا {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_درەخت(), controlp);
            }
        }
        إذا (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ثابت Ast_گرێ_DType* ثابت lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            إذا (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
                أرجع;
            }
            AstMethodCall* ثابت newp
                = جديد AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            أرجع;
        }
        إذا (ثابت AstNewDynamic* ثابت dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            إذا (!dynp->rhsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } وإلا {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(true);
            newp->protect(خطأ);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            أرجع;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        إذا (AstCMethodHard* ثابت slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            إذا (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } وإلا إذا (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } وإلا إذا (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } وإلا {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            إذا (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_گرێ_دەربڕین* ثابت fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_گرێ_دەربڕین* ثابت lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_گرێ_دەربڕین* ثابت msbp = slicep->pinsp()->unlinkFrBack();
                Ast_گرێ_دەربڕین* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(true);
                newp->protect(خطأ);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                أرجع;
            }
        }

        إذا (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    فراغ visit(AstRSRule* nodep) override {
        إذا (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    فراغ visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    فراغ visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    ساكن منطقي isFormatNonNumericArg(ثابت Ast_گرێ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        أرجع dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, گرێ_UOr_Struct_DType)
                   && !VN_AS(dtypep, گرێ_UOr_Struct_DType)->packed());
    }

    فراغ checkFormatNumericArg(Ast_گرێ* argp, حرف ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        إذا (std::tolower(ch) == 'd') أرجع;
        // In contrast, %x of a class reference, causes errors on several simulators.
        إذا (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    فراغ visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    فراغ visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    فراغ visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        إذا (m_vup && !m_vup->prelim()) أرجع;  // Can be called as statement or function
        إذا (nodep->didWidthAndSet()) أرجع;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        إذا (!nodep->دەربڕین_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_گرێ_دەربڕین* oldExprsp = nodep->exprsp();
        إذا (nodep->دەربڕین_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), گرێ_دەربڕین);
        }
        إذا (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        طالما (Ast_گرێ_دەربڕین* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), گرێ_دەربڕین);
            إذا (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            إذا (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                استمر;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_گرێ_DType* ثابت dtypep = argp->dtypep()->skipRefp();
            إذا (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } وإلا إذا (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } وإلا إذا (isFormatNonNumericArg(dtypep)) {
                إذا (AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    إذا (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(true);
                    }
                } وإلا {
                    // Class handles inside containers print type names, not their members.
                    Ast_گرێ_DType* گرێ_Dtypep = dtypep;
                    طالما (گرێ_Dtypep) {
                        گرێ_Dtypep = گرێ_Dtypep->skipRefp();
                        إذا (Ast_گرێ_UOr_Struct_DType* ثابت uOrStructDTypep
                            = VN_CAST(گرێ_Dtypep, گرێ_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(true);
                        }
                        گرێ_Dtypep = گرێ_Dtypep->subDTypep();
                    }
                }
                Ast_گرێ_دەربڕین* ثابت newp = جديد AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } وإلا إذا (nodep->دەربڕین_Format_2()) {
                إذا (AstEnumDType* ثابت enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    استمر;
                }
            }
            إذا (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            إذا (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } وإلا {
                nodep->addExprsp(جديد AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    فراغ visit(AstCReset* nodep) override {
        assert_At_دەربڕین(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    فراغ visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    فراغ visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    فراغ visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        إذا (!m_paramsOnly) {
            nodep->foreach([هذا](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    جديد AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            إذا (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            بدل (nodep->displayType()) {
            حالة VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); اكسر;
            حالة VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); اكسر;
            حالة VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); اكسر;
            حالة VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); اكسر;
            افتراضي: UASSERT_OBJ(خطأ, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
        }
    }
    فراغ visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    فراغ visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    فراغ visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    فراغ visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    فراغ visit(AstFError* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    فراغ visit(AstFEof* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    فراغ visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    فراغ visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    فراغ visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    فراغ visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    فراغ visit(AstFGetC* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    فراغ visit(AstFGetS* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    فراغ visit(AstFUngetC* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    فراغ visit(AstFRead* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            إذا (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            إذا (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    فراغ visit(AstFScanF* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    فراغ visit(AstSScanF* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    فراغ visit(Ast_کەڵەکە_Trace_F* nodep) override { nodep->dtypeSetString(); }
    فراغ visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstSystemF* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(Ast_گرێ_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        ثابت Ast_گرێ_DType* subp = nullptr;
        إذا (ثابت AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            إذا (!adtypep->کلیل_DTypep()->skipRefp()->basicp()
                || !adtypep->کلیل_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } وإلا إذا (ثابت AstDynArrayDType* ثابت adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } وإلا {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        إذا (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstTestPlusArgs* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    فراغ visit(Ast_بەها_Plus_Args* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    فراغ visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        إذا (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        إذا (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        إذا (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    فراغ visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    فراغ visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    فراغ visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    فراغ visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    فراغ visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    منطقي pinInstArrayElement(AstPin* nodep, ثابت AstRange* rangep, صحيح idx) {
        Ast_گرێ_DType* ثابت modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_گرێ_DType* ثابت conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        إذا (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) أرجع true;
        // Types don't match, but all dimensions done, still need the usual checks
        إذا (!rangep) أرجع خطأ;

        ثابت AstRange* ثابت subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        صحيح subElems = 1;
        لكل (ثابت AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ثابت صحيح elements = rangep->elementsConst();
        ثابت صحيح from_چەپ = idx / subElems;

        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ثابت uint32_t conDims = conDTypep->dimensions(خطأ).second;
        ثابت uint32_t modDims = modDTypep->dimensions(خطأ).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        إذا (arrp && conDims != modDims) {
            إذا (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                أرجع true;
            }
            // ArraySel index is counted from lo
            ثابت صحيح bit = arrp->declRange().ascending() ? from_چەپ : elements - 1 - from_چەپ;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_گرێ_دەربڕین* ثابت fromp = VN_AS(nodep->exprp(), گرێ_دەربڕین)->unlinkFrBack();
            إذا (Ast_گرێ_دەربڕین* ثابت elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(پاڵنان_Deletep(fromp), fromp);
            } وإلا {
                AstArraySel* ثابت selp = جديد AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(true);
                nodep->exprp(selp);
            }
            أرجع pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        إذا (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) أرجع خطأ;

        // Packed connection
        ثابت صحيح modwidth = modDTypep->width();
        ثابت صحيح conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        إذا (conwidth == modwidth) أرجع خطأ;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        إذا (conwidth == modwidth * elements * subElems) {
            إذا (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->چەپ_Const() << ":"
                                                     << rangep->ڕاست_Const() << "]");
            }
            ثابت صحيح selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_گرێ_دەربڕین* ثابت fromp = VN_AS(nodep->exprp(), گرێ_دەربڕین)->unlinkFrBack();
            AstSel* ثابت selp = جديد AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_چەپ), selwidth};
            selp->didWidth(true);
            nodep->exprp(selp);
            أرجع pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        أرجع true;
    }

    فراغ visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        إذا (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ثابت concatp = VN_CAST(nodep->exprp(), Replicate);
            ثابت AstConst* ثابت countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            ثابت Ast_گرێ_DType* ثابت dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            إذا (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* ثابت patternp = جديد AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(پاڵنان_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } وإلا إذا (!m_paramsOnly) {
            إذا (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            إذا (!nodep->exprp()) {  // No-connect
                إذا (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                أرجع;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ثابت VDirection pinDirection = nodep->modVarp()->direction();
            إذا (VN_IS(nodep->exprp(), گرێ_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_گرێ_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_گرێ_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            إذا (ثابت AstIfaceRefDType* ثابت modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ثابت AstIfaceRefDType* ثابت conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                إذا (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_گرێ_DType* subDTypep = modDTypep;
            ثابت صحيح modwidth = modDTypep->width();
            صحيح conwidth = conDTypep->width();
            إذا (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } وإلا {
                إذا (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    إذا (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        إذا (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        أرجع;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                إذا (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } وإلا إذا (nodep->modVarp()->isTristate()) {
                    إذا (modwidth != conwidth) {
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
                } وإلا إذا (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                إذا ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ثابت Ast_گرێ_Array_DType* ثابت دەربڕین_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ثابت Ast_گرێ_Array_DType* ثابت modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                إذا (دەربڕین_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } وإلا إذا ((دەربڕین_Arrayp && !modArrayp) || (!دەربڕین_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (دەربڕین_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } وإلا {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_گرێ_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), گرێ_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            إذا (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    فراغ visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        إذا (!m_paramsOnly) {
            إذا (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_پەیام(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            إذا (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            إذا (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                پاڵنان_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    فراغ visit(AstGatePin* nodep) override {
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ثابت Ast_گرێ_DType* ثابت pinDTypep = nodep->exprp()->dtypep();
            ثابت صحيح ژمارە_Insts = nodep->rangep()->elementsConst();
            ثابت صحيح modwidth = ژمارە_Insts;
            ثابت صحيح pinwidth = pinDTypep->width();
            إذا (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_گرێ_DType* ثابت subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_گرێ* ثابت newp
                    = جديد AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(ژمارە_Insts)};
                nodep->replaceWith(newp);
            } وإلا {
                إذا (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
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
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        }
    }
    فراغ visit(Ast_گرێ_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        إذا (nodep->didWidth()) أرجع;
        إذا (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(true);
            nodep->didWidth(true);
            أرجع;
        }
        إذا (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } وإلا إذا (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        إذا (nodep->classMethod() && nodep->name() == "new") {
            إذا (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            إذا (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_گرێ* firstp = nullptr;
            لكل (Ast_گرێ* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                إذا (Ast_Stmt_دەربڕین* ثابت sep = VN_CAST(itemp, Stmt_دەربڕین)) {
                    إذا (AstNew* ثابت newp = VN_CAST(sep->exprp(), New)) {
                        إذا (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            اكسر;
                        }
                    }
                    استمر;
                }
                إذا (firstNewStatementOkRecurse(itemp)) استمر;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(true);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        إذا (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        إذا (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } وإلا إذا (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } وإلا إذا (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        إذا (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(true);
        nodep->doingWidth(خطأ);
        إذا (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        إذا (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](ثابت AstVarRef* varrefp) {
                   أرجع varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        لكل (Ast_گرێ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } وإلا إذا (AstAttrOf* ثابت attrp = VN_CAST(stmtp, AttrOf)) {
                إذا (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                إذا (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        إذا (protop) {
            لكل (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                إذا (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    اكسر;
                } وإلا {
                    AstVar* ثابت portp = ports[i];
                    AstAttrOf* ثابت rprotop = protos[i];
                    Ast_گرێ_DType* ثابت declDtp = portp->dtypep();
                    Ast_گرێ_DType* ثابت protoDtp = rprotop->fromp()->dtypep();
                    إذا (portp->name() != rprotop->name()) {
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
                    } وإلا إذا (!similarDTypeRecurse(protoDtp, declDtp)) {
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
    فراغ visit(AstConstraint* nodep) override {
        إذا (nodep->didWidth()) أرجع;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(true);
    }
    فراغ visit(AstProperty* nodep) override {
        إذا (nodep->didWidth()) أرجع;
        إذا (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(true);
            nodep->didWidth(true);
            أرجع;
        }
        nodep->doingWidth(true);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        لكل (Ast_گرێ* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            إذا (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } وإلا إذا (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } وإلا إذا (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } وإلا {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(true);
        nodep->doingWidth(خطأ);
    }
    فراغ visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    فراغ visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        إذا (nodep->didWidth()) أرجع;
        nodep->dtypeSetBit();
        لكل (Ast_گرێ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } وإلا إذا (VN_IS(stmtp, گرێ_دەربڕین)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } وإلا {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(true);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_گرێ* pkgItemp) {
        طالما (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        أرجع VN_CAST(pkgItemp->backp(), Package);
    }
    فراغ visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_گرێ_FTask_Ref*>(nodep));
        إذا (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            إذا (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_دەربڕین))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } وإلا {
            nodep->dtypeFrom(nodep->taskp());
        }
        إذا (nodep->fileline()->timingOn()) {
            Ast_گرێ_Module* ثابت classp = nodep->classOrPackagep();
            إذا (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ثابت packagep = getItemPackage(classp);
                إذا (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    ساكن منطقي areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ثابت std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ثابت std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        إذا (dims0.size() != dims1.size()) أرجع خطأ;
        لكل (size_t i = 0; i < dims0.size(); ++i) {
            إذا (dims0[i]->elementsConst() != dims1[i]->elementsConst()) أرجع خطأ;
        }
        أرجع true;
    }
    // Makes sure that port and pin have same size and same datatype
    فراغ checkUnpackedArrayArgs(AstVar* portp, Ast_گرێ* pinp) {
        إذا (AstUnpackArrayDType* ثابت portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            إذا (AstUnpackArrayDType* ثابت pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                إذا (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                ثابت AstBasicDType* ثابت portBasicp = portDtypep->basicp();
                ثابت AstBasicDType* ثابت pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ثابت منطقي sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                إذا (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } وإلا {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    فراغ processFTaskRefArgs(Ast_گرێ_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        إذا (nodep->didWidth()) أرجع;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        افعل {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ثابت V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            إذا (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            لكل (ثابت auto& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.first;
                AstArg* ثابت argp = tconnect.second;
                Ast_گرێ_دەربڕین* pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                إذا (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_گرێ_دەربڕین* argsp = nullptr;
                    طالما (AstArg* ثابت nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_درەخت();  // Remove the call's Arg wrapper
                    }
                    string format;
                    إذا (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->ژمارە().toString();
                    } وإلا {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(پاڵنان_Deletep(argp), argp);
                    AstSFormatF* ثابت newp
                        = جديد AstSFormatF{nodep->fileline(), format, خطأ, argsp};
                    إذا (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(جديد AstScopeName{newp->fileline(), true});
                    }
                    handle.relink(جديد AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    اذهب reloop;  // so exit early; next loop will correct it
                }  //
                وإلا إذا (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_گرێ_دەربڕین* ثابت newp = جديد AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                إذا (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } طالما (خطأ);
        // Stage 2
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            لكل (ثابت auto& tconnect : tconnects) {
                AstVar* ثابت portp = tconnect.first;
                ثابت AstArg* ثابت argp = tconnect.second;
                Ast_گرێ_دەربڕین* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                // Change data types based on above accept completion
                إذا (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                إذا (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            لكل (ثابت auto& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.first;
                ثابت AstArg* ثابت argp = tconnect.second;
                Ast_گرێ_دەربڕین* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        إذا (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            لكل (ثابت auto& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.first;
                ثابت AstArg* ثابت argp = tconnect.second;
                Ast_گرێ_دەربڕین* pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                Ast_گرێ_DType* ثابت portDTypep = portp->dtypep()->skipRefToEnump();
                إذا (VN_IS(pinp, EmptyQueue)) {
                    Ast_گرێ_دەربڕین* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    إذا (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = جديد AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ثابت Ast_گرێ_DType* ثابت pinDTypep = pinp->dtypep()->skipRefToEnump();
                ثابت AstIfaceRefDType* ثابت portIfacep
                    = VN_CAST(portDTypep->elemDTypep(true), IfaceRefDType);
                ثابت VCastable ifaceCastable
                    = portIfacep ? Ast_گرێ::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ true)
                                 : VCastable{VCastable::UNSUPPORTED};
                ثابت منطقي matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                ثابت منطقي pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->ژمارە().isNull();
                إذا ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    إذا (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } وإلا {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } وإلا إذا (portIfacep && portIfacep->isVirtual() && portp->is_تێخستن() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } وإلا إذا (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_گرێ_دەربڕین* ثابت newp = جديد AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                إذا (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    استمر;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                إذا (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } وإلا إذا (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } وإلا {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    فراغ handleStdRandomizeArgs(Ast_گرێ_FTask_Ref* ثابت nodep) {
        AstConst* nullp = nullptr;
        لكل (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_گرێ_دەربڕین* ثابت exprp = argp->exprp();
            إذا (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                إذا (constp->ژمارە().isNull()) {
                    nullp = constp;
                    استمر;
                }
            }
            إذا (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                استمر;
            }
            إذا (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                استمر;
            }
            إذا (ثابت AstCMethodHard* ثابت methodp = VN_CAST(exprp, CMethodHard)) {
                إذا (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    استمر;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        إذا (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    فراغ visit(Ast_گرێ_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        إذا (nodep->didWidth()) أرجع;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        إذا (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(true);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            أرجع;  // Handled in V3Randomize
        } وإلا إذا (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ثابت classp = VN_CAST(nodep->classOrPackagep(), Class);
            إذا (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(true);
                Ast_گرێ_DType* ثابت adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, خطأ, خطأ, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                إذا (withp) nodep->withp(withp);
                nodep->didWidth(true);
                أرجع;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            إذا (nodep->name() == "randomize") {
                AstClassRefDType* ثابت adtypep
                    = جديد AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, خطأ, خطأ, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } وإلا إذا (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } وإلا إذا (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCExpr* ثابت newp
                    = جديد AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                أرجع;
            } وإلا إذا (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_گرێ_دەربڕین* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_گرێ_دەربڕین* ثابت exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCExpr* ثابت newp
                    = جديد AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                أرجع;
            } وإلا {
                UASSERT_OBJ(خطأ, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        إذا (nodep->didWidth()) {
            إذا (withp) nodep->withp(withp);
            أرجع;
        }
        إذا ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            منطقي allow = خطأ;
            إذا (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                إذا (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } وإلا إذا (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                إذا (!allow) {
                    ثابت AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    إذا (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    إذا (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            إذا (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ثابت AstClass* ثابت cgClassp = m_containingClassFinder.find(nodep);
                إذا (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    إذا (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            إذا (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        إذا (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), خطأ});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        إذا (withp) nodep->withp(withp);
        nodep->didWidth(true);
        // See steps that follow in visit(AstFuncRef*)
    }
    فراغ visit(Ast_گرێ_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    فراغ visit(AstSenItem* nodep) override {
        إذا (nodep->isComboStar()) أرجع;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        إذا (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* ثابت concatOrReplp = VN_CAST(nodep->sensp(), گرێ_Biop);
            auto* ثابت rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            إذا (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* ثابت lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(جديد AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(جديد AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
        } وإلا {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            إذا (nodep->edgeType().anEdge()) {
                Ast_گرێ_DType* ثابت sensDtp = nodep->sensp()->dtypep()->skipRefp();
                إذا (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } وإلا إذا (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    فراغ visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ثابت refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    فراغ visit(AstWait* nodep) override {
        إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
            أرجع;
        }
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                إذا (Ast_گرێ_Var_Ref* ثابت varrefp = VN_CAST(nodep->condp(), گرێ_Var_Ref)) {
                    إذا (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                أرجع;
            } وإلا إذا (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } وإلا {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        إذا (Ast_گرێ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
    }
    فراغ visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_گرێ_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->بەها_Arg_Refp(), nullptr);
        إذا (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } وإلا {
            إذا (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } وإلا {
                إذا (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } وإلا {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            إذا (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } وإلا {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    فراغ visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        إذا (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } وإلا {
            nodep->dtypeFrom(m_withp->بەها_Arg_Refp());
        }
    }
    فراغ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    فراغ visit(Ast_گرێ_دەربڕین* nodep) override {
        إذا (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    فراغ visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        إذا (nodep->didWidthAndSet()) أرجع;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        إذا (Ast_گرێ_DType* ثابت vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } وإلا إذا (!nodep->dtypep()) {
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
        nodep->didWidth(true);
    }
    فراغ visitClass(AstClass* nodep) {
        إذا (nodep->didWidthAndSet()) أرجع;

        // If the class is std::process
        إذا (nodep->name() == "process") {
            AstPackage* ثابت packagep = getItemPackage(nodep);
            إذا (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                إذا (AstVar* ثابت varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_گرێ_DType* ثابت dtypep = varp->getChildDTypep();
                    إذا (!varp->dtypep()) {
                        VL_DO_DANGLING(پاڵنان_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* ثابت newdtypep = جديد AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                إذا (Ast_گرێ_FTask* ثابت ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), گرێ_FTask)) {
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
    فراغ visit(Ast_گرێ_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } وإلا {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([هذا](ثابت AstVar* varp) { m_curModVars.زیادکردن(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    فراغ visit(Ast_گرێ* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    فراغ visit_Or_Lu64(Ast_گرێ_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_گرێ_DType* ثابت subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    فراغ visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            إذا (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    جديد AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
            }
        }
    }
    فراغ visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    فراغ visit_Os32_Lr(Ast_گرێ_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    فراغ visit_Ou64_Lr(Ast_گرێ_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    فراغ visit_log_not(AstLogNot* nodep) {
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
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            إذا (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ثابت newp
                    = جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            }
        }
    }
    فراغ visit_log_and_or(Ast_گرێ_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_گرێ_دەربڕین::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    فراغ visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit_red_and_or(Ast_گرێ_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    فراغ visit_red_unknown(Ast_گرێ_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    منطقي isEquivalentDType(ثابت Ast_گرێ_DType* lhs, ثابت Ast_گرێ_DType* rhs) {
        // a) If two types match, they are equivalent.
        إذا (!lhs || !rhs) أرجع خطأ;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        إذا (lhs == rhs) أرجع true;
        // If both are basic types, check if they are the same type
        إذا (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ثابت auto* lb = VN_CAST(lhs, BasicDType);
            ثابت auto* rb = VN_CAST(rhs, BasicDType);
            إذا (lb->isString() != rb->isString()) أرجع خطأ;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ثابت منطقي lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ثابت منطقي rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        إذا (lhsIsUnpackArray || rhsIsUnpackArray) {
            إذا (lhsIsUnpackArray && rhsIsUnpackArray) {
                ثابت AstUnpackArrayDType* ثابت lhsp = VN_CAST(lhs, UnpackArrayDType);
                ثابت AstUnpackArrayDType* ثابت rhsp = VN_CAST(rhs, UnpackArrayDType);
                ثابت صحيح lsz = lhsp->elementsConst();
                ثابت صحيح rsz = rhsp->elementsConst();
                إذا (lsz >= 0 && rsz >= 0 && lsz != rsz) أرجع خطأ;
                أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            أرجع خطأ;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ثابت منطقي lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ثابت منطقي rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ثابت منطقي lhsIsQueue = VN_IS(lhs, QueueDType);
        ثابت منطقي rhsIsQueue = VN_IS(rhs, QueueDType);
        ثابت منطقي lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ثابت منطقي rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        إذا (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            إذا (ثابت AstDynArrayDType* ثابت lhsp = VN_CAST(lhs, DynArrayDType)) {
                إذا (ثابت AstDynArrayDType* ثابت rhsp = VN_CAST(rhs, DynArrayDType)) {
                    أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            إذا (ثابت AstQueueDType* ثابت lhsp = VN_CAST(lhs, QueueDType)) {
                إذا (ثابت AstQueueDType* ثابت rhsp = VN_CAST(rhs, QueueDType)) {
                    أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            إذا (ثابت AstAssocArrayDType* ثابت lhsp = VN_CAST(lhs, AssocArrayDType)) {
                إذا (ثابت AstAssocArrayDType* ثابت rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->کلیل_DTypep(), rhsp->کلیل_DTypep());
                }
            }

            أرجع خطأ;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        إذا (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            إذا (lhs->width() != rhs->width()) أرجع خطأ;
            إذا (lhs->isFourstate() != rhs->isFourstate()) أرجع خطأ;
            إذا (lhs->isSigned() != rhs->isSigned()) أرجع خطأ;
            أرجع true;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        إذا (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) أرجع خطأ;
        أرجع !(VN_IS(lhs, گرێ_UOr_Struct_DType) || VN_IS(rhs, گرێ_UOr_Struct_DType));
    }

    ساكن منطقي isAggregateType(ثابت Ast_گرێ* nodep) {
        إذا (!nodep) أرجع خطأ;
        ثابت Ast_گرێ_DType* dtypep = nodep->dtypep();
        إذا (!dtypep) أرجع خطأ;
        dtypep = dtypep->skipRefp();
        إذا (!dtypep) أرجع خطأ;
        أرجع dtypep->isAggregateType();
    }

    فراغ visit_cmp_eq_gt(Ast_گرێ_Biop* nodep, ثابت ComparisonType cmpType) {
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
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ثابت منطقي lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            ثابت منطقي rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            إذا (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } وإلا إذا (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } وإلا {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            ثابت منطقي isAggrLhs = isAggregateType(nodep->lhsp());
            ثابت منطقي isAggrRhs = isAggregateType(nodep->rhsp());

            إذا ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                ثابت Ast_گرێ_DType* ثابت lhsDType = nodep->lhsp()->dtypep();
                ثابت Ast_گرێ_DType* ثابت rhsDType = nodep->rhsp()->dtypep();

                إذا (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_گرێ* ثابت newp
                        = جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } وإلا إذا (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                إذا (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_گرێ* ثابت newp
                        = جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                    أرجع;
                }
                إذا (Ast_گرێ_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } وإلا إذا (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                إذا (Ast_گرێ_Biop* ثابت newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } وإلا {
                ثابت منطقي signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                إذا (Ast_گرێ_Biop* ثابت newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ثابت صحيح width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ثابت صحيح ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                منطقي warnOn = true;
                إذا (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    إذا ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = خطأ;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    إذا ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = خطأ;
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
    فراغ visit_cmp_real(Ast_گرێ_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit_cmp_string(Ast_گرێ_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    فراغ visit_cmp_type(Ast_گرێ_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            ثابت AstAttrOf* ثابت lhsap = VN_AS(nodep->lhsp(), AttrOf);
            ثابت AstAttrOf* ثابت rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ثابت Ast_گرێ_DType* ثابت lhsDtp = lhsap->dtypep();
            ثابت Ast_گرێ_DType* ثابت rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ثابت منطقي invert = VN_IS(nodep, NeqT);
            ثابت منطقي identical
                = Ast_گرێ::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ثابت منطقي eq = invert ^ identical;
            Ast_گرێ* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        }
    }

    فراغ visit_negate_not(Ast_گرێ_Uniop* nodep, منطقي real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            إذا (!real_ok) checkCvtUS(nodep->lhsp(), خطأ);
        }
        إذا (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            إذا (Ast_گرێ_Uniop* ثابت newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                أرجع;
            }
        } وإلا {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        إذا (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        إذا (m_vup->final()) {
            Ast_گرێ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_گرێ_DType* ثابت subDTypep = expDTypep;
            // Some warning suppressions
            منطقي lhsWarn = true;
            إذا (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                إذا (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = خطأ;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    فراغ visit_signed_unsigned(Ast_گرێ_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), true);
            ثابت صحيح width = nodep->lhsp()->width();
            Ast_گرێ_DType* ثابت expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_گرێ_DType* ثابت subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    فراغ visit_shift(Ast_گرێ_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ثابت Ast_گرێ_Biop* ثابت newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (فراغ)newp;  // Ununused
    }
    فراغ iterate_shift_prelim(Ast_گرێ_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), خطأ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            إذا (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_گرێ_Biop* iterate_shift_final(Ast_گرێ_Biop* nodep) {
        // Nodep maybe edited
        assert_At_دەربڕین(nodep);
        إذا (m_vup->final()) {
            Ast_گرێ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرێ_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            إذا (VN_IS(nodep, ShiftRS)) {
                إذا (Ast_گرێ_Biop* ثابت newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            منطقي warnOn = true;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            إذا (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), گرێ_Assign)) warnOn = خطأ;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const);
            إذا (shiftp && !shiftp->ژمارە().isFourState() && shiftp->width() <= 32) {
                ثابت int64_t shiftVal = shiftp->ژمارە().toSQuad();
                إذا (VN_IS(nodep, ShiftL)) {
                    إذا (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = خطأ;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            إذا (nodep->rhsp()->width() > 32) {
                إذا (shiftp && shiftp->ژمارە().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_ژمار ژمارە{shiftp, 32, shiftp->ژمارە()};
                    Ast_گرێ* ثابت shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(جديد AstConst{shiftrhsp->fileline(), ژمارە});
                    VL_DO_DANGLING(shiftrhsp->delete_درەخت(), shiftrhsp);
                }
            }
        }
        أرجع nodep;  // May edit
    }

    فراغ visit_boolexpr_and_or(Ast_گرێ_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), خطأ);
            checkCvtUS(nodep->rhsp(), خطأ);
            ثابت صحيح width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            ثابت صحيح mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            ثابت منطقي expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_گرێ_دەربڕین::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        إذا (m_vup->final()) {
            Ast_گرێ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرێ_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    فراغ visit_add_sub_replace(Ast_گرێ_Biop* nodep, منطقي real_ok, ثابت منطقي modOrDiv) {
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
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            إذا (!real_ok) {
                checkCvtUS(nodep->lhsp(), خطأ);
                checkCvtUS(nodep->rhsp(), خطأ);
            }
            إذا (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                إذا (Ast_گرێ_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                أرجع;
            } وإلا إذا (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                أرجع;
            } وإلا {
                ثابت صحيح width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ثابت صحيح mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                ثابت منطقي expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_گرێ_دەربڕین::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        إذا (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_گرێ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرێ_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            إذا (Ast_گرێ_Biop* ثابت newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            منطقي lhsWarn = true;
            منطقي rhsWarn = true;
            إذا (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                إذا (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = خطأ;
                إذا (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = خطأ;
                إذا (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = خطأ;  // do_increment + ...
                إذا (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = خطأ;  // ... + do_increment
            } وإلا إذا (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                إذا (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = خطأ;
                إذا (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = خطأ;
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
    فراغ visit_real_add_sub(Ast_گرێ_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    فراغ visit_real_neg_ceil(Ast_گرێ_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_دەربڕین(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    فراغ visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->دەربڕین_Format_2(), nodep, "Assumes constant format");
        منطقي inPct = خطأ;
        Ast_گرێ_دەربڕین* argp = nodep->exprsp();
        string newFormat;
        لكل (حرف ch : nodep->text()) {
            إذا (!inPct && ch == '%') {
                inPct = true;
                newFormat += ch;
            } وإلا إذا (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } وإلا إذا (!inPct) {  // Normal text
                newFormat += ch;
            } وإلا {
                inPct = خطأ;
                Ast_گرێ_دەربڕین* ثابت nextp = argp ? VN_AS(argp->nextp(), گرێ_دەربڕین) : nullptr;
                AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_گرێ_دەربڕین* subargp = fargp ? fargp->exprp() : argp;
                ثابت Ast_گرێ_DType* ثابت dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                بدل (ch) {
                حالة '?':  // Unspecified by user, guess
                    إذا (dtypep->isDouble()) {
                        ch = 'g';
                    } وإلا إذا (dtypep->isString()) {
                        ch = 's';
                    } وإلا إذا (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } وإلا {
                        ch = 'p';
                    }
                    argp = nextp;
                    اكسر;
                حالة '%': اكسر;  // %% - just output a %
                حالة 'm': اكسر;  // %m - auto insert "name"
                حالة 'l': اكسر;  // %m - auto insert "library"
                حالة 'd':
                    إذا (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        إذا (subargp->isDouble()) spliceCvtS(subargp, true, 64);
                        argp = nextp;
                    }
                    اكسر;
                حالة 'b':  // FALLTHRU
                حالة 'o':  // FALLTHRU
                حالة 'x':
                    إذا (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    اكسر;
                حالة 't':  // Decimal/float time
                    إذا (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    اكسر;
                حالة 'e':  // FALLTHRU
                حالة 'f':  // FALLTHRU
                حالة 'g':
                    إذا (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        إذا (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    اكسر;
                حالة 'p':  // FALLTHRU
                حالة 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    إذا (subargp && !fargp) {
                        إذا (AstEnumDType* ثابت enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    اكسر;
                افتراضي:  // Most operators, just move to next argument
                    argp = nextp;
                    اكسر;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    ساكن AstEnumDType* formatEnumDType(Ast_گرێ_دەربڕین* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        إذا (!enumDtp) {
            إذا (ثابت AstVarRef* ثابت varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        أرجع enumDtp;
    }

    Ast_دەربڕین_Stmt* newEnumCapture(Ast_گرێ_دەربڕین* valuep, AstEnumDType* dtypep) {
        File_هێڵ* ثابت flp = valuep->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(true);
        إذا (m_ftaskp) {
            varp->funcLocal(true);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } وإلا {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_دەربڕین_Stmt* ثابت exprp = جديد Ast_دەربڕین_Stmt{
            flp, جديد AstAssign{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            جديد AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        أرجع exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_گرێ_دەربڕین* valuep, AstEnumDType* dtypep) {
        إذا (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_گرێ_دەربڕین* keyp;
        إذا (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_درەخت(خطأ);
        } وإلا {
            Ast_دەربڕین_Stmt* ثابت capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_درەخت_Pure(خطأ);
        }
        ثابت VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ثابت argp = جديد AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        أرجع argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    منطقي widthBad(Ast_گرێ* nodep, Ast_گرێ_DType* expDTypep) {
        ثابت صحيح expWidth = expDTypep->width();
        صحيح expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        إذا (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) أرجع خطأ;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        إذا (expWidthMin == 0) expWidthMin = expWidth;
        إذا (nodep->dtypep()->width() == expWidth) أرجع خطأ;
        إذا (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) أرجع true;
        إذا (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) أرجع true;
        أرجع خطأ;
    }

    فراغ fixWidthExtend(Ast_گرێ_دەربڕین* nodep, Ast_گرێ_DType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<صحيح>(extendRule) << ") old: " << nodep);
        إذا (extendRule == EXTEND_OFF) أرجع;
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        ثابت صحيح expWidth = expDTypep->width();
        إذا (constp && !constp->ژمارە().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_ژمار ژمارە{nodep, expWidth, constp->ژمارە()};
            ژمارە.isSigned(خطأ);
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstConst{nodep->fileline(), ژمارە};
            constp->replaceWith(newp);
            VL_DO_DANGLING(پاڵنان_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } وإلا إذا (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(true);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } وإلا {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            منطقي doSigned = خطأ;
            بدل (extendRule) {
            حالة EXTEND_ZERO: doSigned = خطأ; اكسر;
            حالة EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); اكسر;
            حالة EXTEND_LHS: doSigned = nodep->isSigned(); اكسر;
            افتراضي: nodep->v3fatalSrc("bad case");
            }
            Ast_گرێ_دەربڕین* ثابت newp
                = (doSigned ? static_cast<Ast_گرێ_دەربڕین*>(جديد AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_گرێ_دەربڕین*>(جديد AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(true);
            linker.relink(newp);
            nodep = newp;
        }
        إذا (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_گرێ_دەربڕین* ثابت newp = spliceCvtD(nodep);
            nodep = newp;
        }
        إذا (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } وإلا {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    فراغ fixWidthReduce(Ast_گرێ_دەربڕین* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ثابت صحيح expWidth = 1;
        ثابت صحيح expSigned = خطأ;
        إذا (nodep->width() == expWidth) أرجع;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (constp) {
            V3_ژمار ژمارە(nodep, expWidth);
            ژمارە.opRedOr(constp->ژمارە());
            ژمارە.isSigned(expSigned);
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstConst{nodep->fileline(), ژمارە};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_درەخت(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } وإلا {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_گرێ_دەربڕین* newp;
            Ast_گرێ_DType* ثابت dtypeSkipp = nodep->dtypep()->skipRefp();
            إذا (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = جديد AstNeq{nodep->fileline(),
                                  جديد AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } وإلا {
                // But, AstRedOr can be faster on wide
                newp = جديد AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    منطقي fixAutoExtend(Ast_گرێ_دەربڕین*& nodepr, صحيح expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        إذا (AstConst* ثابت constp = VN_CAST(nodepr, Const)) {
            إذا (constp->ژمارە().autoExtend() && !constp->ژمارە().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_ژمار ژمارە(constp, expWidth);
                ژمارە.opRepl(constp->ژمارە(), expWidth);  // {width{'1}}
                Ast_گرێ_دەربڕین* ثابت newp = جديد AstConst{constp->fileline(), ژمارە};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_درەخت(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                أرجع true;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            وإلا إذا (!constp->ژمارە().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->ژمارە().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_ژمار ژمارە(constp, expWidth);
                ژمارە.opExtendXZ(constp->ژمارە(), constp->width());
                Ast_گرێ_دەربڕین* ثابت newp = جديد AstConst{constp->fileline(), ژمارە};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_درەخت(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                أرجع true;
            }
        }
        أرجع خطأ;  // No change
    }
    منطقي isBaseClassRecurse(ثابت AstClass* ثابت cls1p, ثابت AstClass* ثابت cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        إذا (cls1p == cls2p) أرجع true;
        لكل (ثابت AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            إذا (isBaseClassRecurse(cls1p, cextp->classp())) أرجع true;
        }
        أرجع خطأ;
    }
    // Lines to end a message on nodep with the data types of its operands
    ساكن string warnDTypes(ثابت Ast_گرێ* nodep, ثابت Ast_گرێ_DType* lhsDtp,
                             ثابت Ast_گرێ_DType* rhsDtp, ثابت string& lhsName = "Left-hand",
                             ثابت string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        إذا (lhsState == rhsState) lhsState = rhsState = "";
        أرجع "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    ساكن منطقي unpackedArraySizesMatch(ثابت Ast_گرێ_DType* lhsDtp, ثابت Ast_گرێ_DType* rhsDtp) {
        طالما (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ثابت AstUnpackArrayDType* ثابت lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ثابت AstUnpackArrayDType* ثابت rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            إذا (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                أرجع خطأ;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        أرجع true;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    فراغ checkUnpackedArrayAssignmentCompatible(ثابت Ast_گرێ* nodep, ثابت T* ثابت lhsRefp,
                                                ثابت N* ثابت rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::بەها || std::is_same<T, Ast_گرێ_Var_Ref>::بەها)
                && (std::is_same<N, AstVar>::بەها || std::is_same<N, Ast_گرێ_Var_Ref>::بەها),
            "Unsupported types provided.");
        إذا (!lhsRefp || !rhsRefp) أرجع;
        std::string lhsName;
        std::string rhsName;
        إذا (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } وإلا {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ثابت Ast_گرێ_DType* ثابت lhsDtp = lhsRefp->dtypep()->skipRefp();
        ثابت Ast_گرێ_DType* ثابت rhsDtp = rhsRefp->dtypep()->skipRefp();
        ثابت منطقي isLhsAggregate = lhsDtp->isAggregateType();
        ثابت منطقي isRhsAggregate = rhsDtp->isAggregateType();
        إذا (!isLhsAggregate && !isRhsAggregate) أرجع;
        إذا (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            أرجع;
        } وإلا إذا (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            أرجع;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(خطأ),
                                      rhsDim = rhsDtp->dimensions(خطأ);
        // Check if unpacked array dimensions are matching
        إذا (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            أرجع;
        }

        ثابت Ast_گرێ_DType* lhsDtpIterp = lhsDtp;
        ثابت Ast_گرێ_DType* rhsDtpIterp = rhsDtp;
        لكل (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            إذا (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                أرجع;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        إذا (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    فراغ checkClassAssign(ثابت Ast_گرێ* nodep, ثابت حرف* side, Ast_گرێ* rhsp,
                          Ast_گرێ_DType* ثابت lhsDTypep, منطقي isOutputArg = خطأ) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ثابت Ast_گرێ_DType* ثابت lhsRawDTypep = lhsDTypep->skipRefp();
        ثابت Ast_گرێ_DType* ثابت rhsRawDTypep = rhsp->dtypep()->skipRefp();
        إذا (ثابت AstClassRefDType* ثابت lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            إذا (ثابت AstClassRefDType* ثابت rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ثابت AstClass* ثابت dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ثابت AstClass* ثابت srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                إذا (isBaseClassRecurse(dstp, srcp)) أرجع;
            } وإلا إذا (rhsp->isNull()) {
                أرجع;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } وإلا إذا (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        إذا (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                جديد AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, گرێ_دەربڕین), lhsDTypep});
        }
    }
    ساكن منطقي similarDTypeRecurse(ثابت Ast_گرێ_DType* ثابت node1p,
                                    ثابت Ast_گرێ_DType* ثابت node2p) {
        أرجع node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    فراغ iterateCheckFileDesc(Ast_گرێ* parentp, Ast_گرێ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_گرێ_DType* ثابت expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckReal(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    فراغ iterateCheckSigned8(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    فراغ iterateCheckSigned32(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    فراغ iterateCheckUInt32(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    فراغ iterateCheckDelay(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        إذا (stage & FINAL) {
            Ast_گرێ_DType* expDTypep;
            إذا (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } وإلا {
                File_هێڵ* ثابت newFl = جديد File_هێڵ{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, true);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, خطأ);
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckString(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    فراغ iterateCheckTyped(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp,
                           Ast_گرێ_DType* expDTypep, Stage stage) {
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        إذا (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckTypedSelfPrelim(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp,
                                     Ast_گرێ_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        إذا (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckIntegralSelf(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, true);
    }
    فراغ iterateCheckSelf(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Determ determ,
                          Stage stage, منطقي integralOnly = خطأ) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, گرێ_دەربڕین) ? checkCvtUS(VN_AS(underp, گرێ_دەربڕین), integralOnly) : underp;
        Ast_گرێ_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckSizedSelf(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Determ determ,
                               Stage stage, منطقي deferUntyped = خطأ) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, گرێ_دەربڕین) ? checkCvtUS(VN_AS(underp, گرێ_دەربڕین), خطأ) : underp;
        Ast_گرێ_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_گرێ_DType* ثابت checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        إذا (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckAssign(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* rhsp, Stage stage,
                            Ast_گرێ_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        إذا (AstConst* constp = VN_CAST(rhsp, Const)) {
            إذا (ثابت AstUnpackArrayDType* ثابت arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                إذا (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    إذا (basicp->width() == 8 && constp->ژمارە().isFromString()) {
                        AstConsArray* newp = جديد AstConsArray{
                            constp->fileline(), lhsDTypep,
                            جديد AstConst{constp->fileline(), AstConst::Widthed_بەها{}, 8, 0}};
                        لكل (ثابت صحيح aindex : arrayp->declRange().seqLoToHi()) {
                            صحيح cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_ژمار selected{constp, 8};
                            selected.opSel(constp->ژمارە(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            إذا (!selected.isFourState()) {
                                إذا (ثابت uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 جديد AstConst{constp->fileline(),
                                                              AstConst::Widthed_بەها{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(پاڵنان_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        ثابت منطقي lhsStream = (VN_IS(parentp, گرێ_Assign)
                                && VN_IS(VN_AS(parentp, گرێ_Assign)->lhsp(), گرێ_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, true, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (فراغ)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_گرێ_دەربڕین* convertUnpackedOrder(Ast_گرێ_دەربڕین* exprp, Ast_گرێ_DType* dtypep) {
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        إذا (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) أرجع exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_گرێ_دەربڕین* ثابت newp = جديد AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        أرجع newp;
    }

    ساكن منطقي isLogicalOperator(ثابت Ast_گرێ* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        أرجع VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    فراغ iterateCheckBool(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Stage stage) {
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
        Ast_گرێ_DType* ثابت underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        إذا (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_گرێ* ثابت newp
                = جديد AstNeqD{parentp->fileline(), VN_AS(underp, گرێ_دەربڕین),
                              جديد AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } وإلا إذا (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_گرێ* ثابت newp
                = جديد AstNeqN{parentp->fileline(), VN_AS(underp, گرێ_دەربڕین),
                              جديد AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } وإلا إذا (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, گرێ_دەربڕین)), underp);  // Changed
        } وإلا إذا (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(جديد AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(پاڵنان_Deletep(underp), underp);
        } وإلا {
            ثابت منطقي bad = widthBad(underp, parentp->findBitDType());
            إذا (bad) {
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
            إذا (Ast_دەربڕین_Stmt* ثابت دەربڕین_Stmtp = VN_CAST(underp, دەربڕین_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(دەربڕین_Stmtp->resultp());
                دەربڕین_Stmtp->dtypeFrom(دەربڕین_Stmtp->resultp());
            } وإلا إذا (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, گرێ_دەربڕین)), underp);  // Changed
            }
        }
    }

    ساكن فراغ checkEnum(Ast_گرێ* nodep, Ast_گرێ* parentp, ثابت Ast_گرێ_DType* targetDTypep,
                          ثابت Ast_گرێ_DType* sourceDTypep, ثابت Ast_گرێ_دەربڕین* sourceExprp) {
        إذا (ثابت AstEnumDType* ثابت targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            ثابت VCastable castable
                = Ast_گرێ::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            إذا (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    Ast_گرێ* iterateCheck(Ast_گرێ* parentp, ثابت حرف* side, Ast_گرێ* underp, Determ determ,
                          Stage stage, Ast_گرێ_DType* expDTypep, ExtendRule extendRule,
                          منطقي warnOn = true, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ثابت StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        إذا (rhsAccess.isReadOnly()) {
            إذا (Ast_گرێ_دەربڕین* ثابت exprp = VN_CAST(underp, گرێ_دەربڕین)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        إذا (VN_IS(underp, گرێ_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } وإلا إذا (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } وإلا إذا (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } وإلا إذا (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_گرێ* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, گرێ_دەربڕین));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } وإلا إذا (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_گرێ* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, گرێ_دەربڕین), true, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } وإلا إذا (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_گرێ* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, گرێ_دەربڕین));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } وإلا {
            ثابت Ast_گرێ_DType* ثابت underDtp = underp->dtypep()->skipRefp();
            إذا (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_گرێ* ثابت newp
                    = جديد AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(پاڵنان_Deletep(underp), underp);
                underp = newp;
            }
            ثابت Ast_گرێ_DType* ثابت expDtp = expDTypep->skipRefp();
            إذا (determ == ASSIGN && VN_IS(expDtp, گرێ_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            إذا (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ثابت AstBasicDType* ثابت expBasicp = expDTypep->basicp();
            ثابت AstBasicDType* ثابت underBasicp = underp->dtypep()->basicp();
            إذا (expBasicp && underBasicp) {
                إذا (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    إذا (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, گرێ_دەربڕین));
                    }
                    إذا (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_گرێ_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                إذا (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } وإلا إذا (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    إذا (underp->isSigned() != subDTypep->isSigned()
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
                } وإلا {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, گرێ_دەربڕین), expDTypep, extendRule,
                                warnOn);
            } وإلا إذا (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } وإلا إذا (ثابت AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ثابت AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                إذا (VN_IS(underp, Const) && VN_AS(underp, Const)->ژمارە().isNull()) {
                    // '= null' is ok
                } وإلا إذا (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } وإلا إذا (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } وإلا إذا (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } وإلا {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } وإلا {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        أرجع underp;
    }

    فراغ
    widthCheckSized(Ast_گرێ* parentp, ثابت حرف* side,
                    Ast_گرێ_دەربڕین* underp,  // Node to be checked or have typecast added in front of
                    Ast_گرێ_DType* expDTypep, ExtendRule extendRule, منطقي warnOn = true) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ثابت AstBasicDType* ثابت expBasicp = expDTypep->basicp();
        ثابت AstBasicDType* ثابت underBasicp = underp->dtypep()->basicp();
        إذا (expDTypep == underp->dtypep()) {
            أرجع;  // Same type must match
        } وإلا إذا (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#إذا VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            أرجع;
        } وإلا {
            ثابت صحيح expWidth = expDTypep->width();
            صحيح expWidthMin = expDTypep->widthMin();
            إذا (expWidthMin == 0) expWidthMin = expWidth;
            ثابت منطقي bad = widthBad(underp, expDTypep);
            إذا ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                أرجع;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            إذا (VN_IS(parentp->backp(), CastSize)) warnOn = خطأ;
            إذا (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = خطأ;
            إذا (VN_IS(underp, Const) && VN_AS(underp, Const)->ژمارە().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = خطأ;
            }
            إذا ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = خطأ;
            }
            إذا (bad && warnOn) {
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
            إذا (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ثابت Ast_گرێ_Assign* assignp = VN_CAST(parentp, گرێ_Assign);
                ثابت AstPin* pinp = VN_CAST(parentp, Pin);
                إذا (assignp && VN_IS(assignp->lhsp(), گرێ_Stream)) {
                } وإلا إذا (pinp && pinp->modVarp()->direction() != VDirection::تێخستن) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } وإلا إذا (assignp && VN_IS(underp, گرێ_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ثابت صحيح shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* ثابت widenedp = جديد AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(true);
                    // Shift left so zeros fill on the right
                    Ast_گرێ_دەربڕین* ثابت shiftedp = جديد AstShiftL{
                        underp->fileline(), widenedp,
                        جديد AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } وإلا {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_گرێ_دەربڕین* checkCvtUS(Ast_گرێ_دەربڕین* nodep, منطقي fatal) {
        إذا (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            إذا (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } وإلا {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, خطأ, 32);
        }
        أرجع nodep;
    }

    Ast_گرێ_دەربڕین* spliceCvtD(Ast_گرێ_دەربڕین* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        إذا (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_گرێ_دەربڕین* newp;
            إذا (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = جديد AstISToRD{nodep->fileline(), nodep};
            } وإلا {
                newp = جديد AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            أرجع newp;
        } وإلا {
            أرجع nodep;
        }
    }
    Ast_گرێ_دەربڕین* spliceCvtS(Ast_گرێ_دەربڕین* nodep, منطقي warnOn, صحيح width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        إذا (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                إذا (constp->isDouble() && V3_ژمار::epsilonIntegral(constp->ژمارە().toDouble())) {
                    warnOn = خطأ;
                }
            }
            إذا (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            أرجع newp;
        } وإلا {
            أرجع nodep;
        }
    }
    Ast_گرێ_دەربڕین* spliceCvtString(Ast_گرێ_دەربڕین* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        إذا (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            أرجع newp;
        } وإلا {
            أرجع nodep;
        }
    }
    Ast_گرێ_Biop* replaceWithUOrSVersion(Ast_گرێ_Biop* nodep, منطقي signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        إذا (signedFlavorNeeded == nodep->signedFlavor()) أرجع nullptr;
        إذا (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        بدل (nodep->type()) {
        حالة VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع nullptr;
        حالة VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع nullptr;
        حالة VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع nullptr;
        حالة VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع nullptr;
        حالة VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع nullptr;
        حالة VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع nullptr;
        حالة VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع nullptr;
        حالة VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع nullptr;
        حالة VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع nullptr;
        افتراضي: اكسر;
        }
        File_هێڵ* ثابت fl = nodep->fileline();
        Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گرێ_دەربڕین* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گرێ_Biop* newp = nullptr;
        بدل (nodep->type()) {
        حالة VNType::Gt: newp = جديد AstGtS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::GtS: newp = جديد AstGt{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gte: newp = جديد AstGteS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::GteS: newp = جديد AstGte{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lt: newp = جديد AstLtS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::LtS: newp = جديد AstLt{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lte: newp = جديد AstLteS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::LteS: newp = جديد AstLte{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Div: newp = جديد AstDivS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::DivS: newp = جديد AstDiv{fl, lhsp, rhsp}; اكسر;
        حالة VNType::ModDiv: newp = جديد AstModDivS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::ModDivS: newp = جديد AstModDiv{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Mul: newp = جديد AstMulS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::MulS: newp = جديد AstMul{fl, lhsp, rhsp}; اكسر;
        حالة VNType::ShiftR: newp = جديد AstShiftRS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::ShiftRS: newp = جديد AstShiftR{fl, lhsp, rhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        أرجع newp;
    }
    Ast_گرێ_Biop* replaceWithDVersion(Ast_گرێ_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        إذا (nodep->doubleFlavor()) أرجع nullptr;
        File_هێڵ* ثابت fl = nodep->fileline();
        Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گرێ_دەربڕین* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گرێ_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        بدل (nodep->type()) {
        حالة VNType::Add: newp = جديد AstAddD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Sub: newp = جديد AstSubD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Pow: newp = جديد AstPowD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Eq:
        حالة VNType::EqCase: newp = جديد AstEqD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Neq:
        حالة VNType::NeqCase: newp = جديد AstNeqD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gt:
        حالة VNType::GtS: newp = جديد AstGtD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gte:
        حالة VNType::GteS: newp = جديد AstGteD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lt:
        حالة VNType::LtS: newp = جديد AstLtD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lte:
        حالة VNType::LteS: newp = جديد AstLteD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Div:
        حالة VNType::DivS: newp = جديد AstDivD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Mul:
        حالة VNType::MulS: newp = جديد AstMulD{fl, lhsp, rhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        أرجع newp;
    }
    Ast_گرێ_Biop* replaceWithNVersion(Ast_گرێ_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        إذا (nodep->stringFlavor()) أرجع nullptr;
        File_هێڵ* ثابت fl = nodep->fileline();
        Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گرێ_دەربڕین* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گرێ_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        بدل (nodep->type()) {
        حالة VNType::Eq:
        حالة VNType::EqCase:
        حالة VNType::EqWild: newp = جديد AstEqN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Neq:
        حالة VNType::NeqCase:
        حالة VNType::NeqWild: newp = جديد AstNeqN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gt:
        حالة VNType::GtS: newp = جديد AstGtN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gte:
        حالة VNType::GteS: newp = جديد AstGteN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lt:
        حالة VNType::LtS: newp = جديد AstLtN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lte:
        حالة VNType::LteS: newp = جديد AstLteN{fl, lhsp, rhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        أرجع newp;
    }
    Ast_گرێ_Uniop* replaceWithDVersion(Ast_گرێ_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        إذا (nodep->doubleFlavor()) أرجع nullptr;
        File_هێڵ* ثابت fl = nodep->fileline();
        Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گرێ_Uniop* newp = nullptr;
        بدل (nodep->type()) {
        حالة VNType::Negate: newp = جديد AstNegateD{fl, lhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
        أرجع newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    فراغ replaceWithSFormat(AstMethodCall* nodep, ثابت string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ثابت argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_گرێ_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_گرێ* ثابت newp = جديد AstAssign{
            nodep->fileline(), fromp,
            جديد AstSFormatF{nodep->fileline(), format, خطأ, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        پاڵنان_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_گرێ_DType* iterateEditMoveDTypep(Ast_گرێ* parentp, Ast_گرێ_DType* dtnodep) {
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
        ثابت منطقي child1 = (parentp->getChildDTypep() == dtnodep);
        ثابت منطقي child2 = (parentp->getChild2DTypep() == dtnodep);
        إذا (child1 || child2) {
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
        إذا (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        أرجع dtnodep;
    }

    AstConst* dimension_بەها(File_هێڵ* fileline, Ast_گرێ_DType* nodep, VAttrType attrType,
                             صحيح dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_گرێ_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        لكل (صحيح i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            إذا (ثابت Ast_گرێ_Array_DType* ثابت adtypep = VN_CAST(dtypep, گرێ_Array_DType)) {
                declRange = adtypep->declRange();
                إذا (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                استمر;
            } وإلا إذا (ثابت Ast_گرێ_UOr_Struct_DType* ثابت adtypep
                       = VN_CAST(dtypep, گرێ_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                اكسر;  // Sub elements don't look like arrays and can't iterate into
            } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                إذا (adtypep->isRanged()) declRange = adtypep->declRange();
                اكسر;
            }
            اكسر;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        صحيح val = 0;
        بدل (attrType) {
        حالة VAttrType::DIM_BITS: {
            صحيح bits = 1;
            طالما (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                إذا (ثابت Ast_گرێ_Array_DType* ثابت adtypep = VN_CAST(dtypep, گرێ_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    استمر;
                } وإلا إذا (ثابت Ast_گرێ_UOr_Struct_DType* ثابت adtypep
                           = VN_CAST(dtypep, گرێ_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    اكسر;
                } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    اكسر;
                }
                اكسر;
            }
            إذا (dim == 0) {
                val = 0;
            } وإلا إذا (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } وإلا {
                val = bits;
            }
            اكسر;  // LCOV_EXCL_LINE
        }
        حالة VAttrType::DIM_بەرز: val = !declRange.ranged() ? 0 : declRange.hi(); اكسر;
        حالة VAttrType::DIM_چەپ: val = !declRange.ranged() ? 0 : declRange.چەپ(); اكسر;
        حالة VAttrType::DIM_نزم: val = !declRange.ranged() ? 0 : declRange.lo(); اكسر;
        حالة VAttrType::DIM_ڕاست: val = !declRange.ranged() ? 0 : declRange.ڕاست(); اكسر;
        حالة VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            اكسر;
        حالة VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); اكسر;
        افتراضي: nodep->v3fatalSrc("Missing DIM ATTR type case"); اكسر;
        }
        إذا (!valp) valp = جديد AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        أرجع valp;
    }
    AstVarRef* dimensionVarRefp(Ast_گرێ_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        إذا (cachedpr) أرجع cachedpr->clone_درەخت(خطأ);

        Ast_گرێ_Array_DType* ثابت vardtypep = جديد AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), جديد AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* ثابت consp = جديد AstConsArray{nodep->fileline(), vardtypep};
        لكل (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_بەها(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_درەخت(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(true);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(true);
        پاڵنان_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        أرجع cachedpr->clone_درەخت(خطأ);
    }
    ساكن uint64_t enum_Max_بەها(ثابت Ast_گرێ* errNodep, ثابت AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        لكل (ثابت AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ثابت AstConst* ثابت vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            إذا (!vconstp->ژمارە().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        إذا (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            أرجع 64;
        }
        أرجع maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ثابت nodep, VAttrType attrType, منطقي assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        إذا (cachedpr) أرجع cachedpr->clone_درەخت(خطأ);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_هێڵ* ثابت flp = nodep->fileline();
        Ast_گرێ_DType* basep;
        إذا (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } وإلا {
            basep = nodep->dtypep();
        }
        Ast_گرێ_DType* vardtypep;
        إذا (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = جديد AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } وإلا {
            vardtypep = جديد AstUnpackArrayDType{flp, basep, جديد AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        إذا (attrType == VAttrType::ENUM_NAME) {
            defaultp = جديد AstConst{flp, AstConst::String{}, ""};
        } وإلا إذا (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = جديد AstConst{flp, V3_ژمار{nodep, nodep->width(), 0}};
        } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
            defaultp = جديد AstConst{flp, AstConst::BitFalse{}};
        } وإلا {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_گرێ_دەربڕین*> بەهاکان;
        {
            uint64_t ordinal = 0;
            AstEnumItem* ثابت firstp = nodep->itemsp();
            ثابت AstEnumItem* prevp = firstp;  // Prev must start with last item
            طالما (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            لكل (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* ثابت nextp = VN_AS(itemp->nextp(), EnumItem);
                ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                إذا (!vconstp->ژمارە().isAnyXZ()) {  // Can 2-state runtime decode
                    ثابت uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    إذا (attrType == VAttrType::ENUM_NAME) {
                        بەهاکان[i] = جديد AstConst{flp, AstConst::String{},
                                                 V3_ژمار::displayedEnumName(itemp)};
                    } وإلا إذا (attrType == VAttrType::ENUM_NEXT) {
                        بەهاکان[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_درەخت(خطأ);  // A const
                    } وإلا إذا (attrType == VAttrType::ENUM_PREV) {
                        بەهاکان[i] = prevp->valuep()->clone_درەخت(خطأ);  // A const
                    } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
                        بەهاکان[i] = جديد AstConst{flp, AstConst::BitTrue{}};
                    } وإلا {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_گرێ_دەربڕین* valuep = nullptr;
        إذا (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ثابت consp = جديد AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            لكل (ثابت auto& itr : بەهاکان) {
                V3_ژمار کلیل{nodep, nodep->width()};
                کلیل.setQuad(itr.first);
                consp->addItemsp(جديد AstConsAssocItem{flp, جديد AstConst{flp, کلیل}, itr.second});
            }
            valuep = consp;
        } وإلا {
            AstConsArray* ثابت consp = جديد AstConsArray{flp, vardtypep, defaultp};
            لكل (uint64_t i = 0; i < (msbdim + 1); ++i) {
                إذا (بەهاکان[i]) consp->valuep(i, بەهاکان[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(پاڵنان_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(true);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(true);
        پاڵنان_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        أرجع cachedpr->clone_درەخت(خطأ);
    }

    Ast_گرێ_دەربڕین* enumSelect(Ast_گرێ_دەربڕین* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        إذا (attrType == VAttrType::ENUM_NAME) {
            إذا (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                Ast_گرێ_دەربڕین* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->ژمارە().displayedEnumName(adtypep)};
                VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
                أرجع newp;
            }
        }
        إذا (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_دەربڕین_Stmt* ثابت capturep = newEnumCapture(nodep, adtypep);
            Ast_گرێ_دەربڕین* ثابت valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            أرجع capturep;
        }
        إذا (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_گرێ_دەربڕین* indexp = جديد AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            لكل (ثابت AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                إذا (constp->ژمارە().isAnyXZ()) استمر;
                indexp = جديد AstCond{nodep->fileline(),
                                     جديد AstEq{nodep->fileline(), nodep->clone_درەخت(خطأ),
                                               itemp->valuep()->clone_درەخت(خطأ)},
                                     جديد AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, خطأ, ordinal);
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
            أرجع newp;
        }
        ثابت uint64_t msbdim = enum_Max_بەها(nodep, adtypep);
        ثابت منطقي assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_گرێ_دەربڕین* newp;
        إذا (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, true, 0);
            newp = جديد AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } وإلا {
            ثابت صحيح selwidth = V3_ژمار::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(adtypep, attrType, خطأ, (1ULL << selwidth) - 1);
            Ast_گرێ_دەربڕین* ثابت boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? جديد AstGt{nodep->fileline(), nodep->clone_درەخت(خطأ),
                                  جديد AstConst{nodep->fileline(),
                                               V3_ژمار{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = جديد AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                جديد AstSel{nodep->fileline(), nodep, 0, selwidth}};
            إذا (boundp) {
                newp = جديد AstCond{nodep->fileline(), boundp,
                                   جديد AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        إذا (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } وإلا {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        أرجع newp;
    }
    Ast_گرێ_دەربڕین* enumTestValid(Ast_گرێ_دەربڕین* valp, AstEnumDType* enumDtp) {
        ثابت uint64_t maxval = enum_Max_بەها(valp, enumDtp);
        ثابت منطقي assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_گرێ_دەربڕین* testp = nullptr;
        File_هێڵ* ثابت fl_novalue = جديد File_هێڵ{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, true);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, true);
        إذا (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, true, 0);
            testp = جديد AstAssocSel{fl_novalue, tabRefp, valp->clone_درەخت_Pure(خطأ)};
        } وإلا {
            ثابت صحيح selwidth = V3_ژمار::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, خطأ, (1ULL << selwidth) - 1);
            File_هێڵ* ثابت fl_nowidth = جديد File_هێڵ{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, true);
            testp = جديد AstCond{
                fl_novalue,
                جديد AstGt{fl_nowidth, valp->clone_درەخت_Pure(خطأ),
                          جديد AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                جديد AstConst{fl_novalue, AstConst::BitFalse{}},
                جديد AstArraySel{fl_novalue, tabRefp,
                                جديد AstSel{fl_novalue, valp->clone_درەخت_Pure(خطأ), 0, selwidth}}};
        }
        أرجع testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, ثابت VNumRange& range) {
        PatVecMap patmap;
        صحيح element = range.چەپ();
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            إذا (patp->keyp()) {
                إذا (ثابت AstConst* ثابت constp = patternConstKeyp(patp)) {
                    إذا (constp->ژمارە().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } وإلا {
                        element = constp->toSInt();
                    }
                }
            }
            ثابت منطقي newEntry = patmap.emplace(element, patp).second;
            إذا (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.چەپ_To_ڕاست_Inc();
        }
        أرجع patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    فراغ patConcatConvertRecurse(AstPattern* patternp, Ast_گرێ_دەربڕین* nodep) {
        إذا (AstConcat* ثابت concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } وإلا {
            AstPatMember* ثابت newp
                = جديد AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(true);
            patternp->addItemsp(newp);
        }
    }

    فراغ makeOpenArrayShell(Ast_گرێ_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_گرێ_FTask* ثابت oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_گرێ_FTask* ثابت newTaskp = oldTaskp->clone_درەخت(خطأ);
        newTaskp->dpiOpenChild(true);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        لكل (ثابت auto& tconnect : tconnects) {
            AstVar* ثابت portp = tconnect.first;
            ثابت AstArg* ثابت argp = tconnect.second;
            ثابت Ast_گرێ* ثابت pinp = argp->exprp();
            إذا (!pinp) استمر;  // Argument error we'll find later
            إذا (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_گرێ_DType* ثابت oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_گرێ* nodep) {
                    إذا (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    منطقي markHasOpenArray(Ast_گرێ_FTask* nodep) {
        منطقي hasOpen = خطأ;
        لكل (Ast_گرێ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(true);
                    hasOpen = true;
                }
            }
        }
        أرجع hasOpen;
    }
    منطقي hasOpenArrayDTypeRecurse(Ast_گرێ_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        إذا (VN_IS(nodep, UnsizedArrayDType)) أرجع true;
        إذا (nodep->subDTypep()) أرجع hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        أرجع خطأ;
    }
    Ast_گرێ_DType* dtypeNotIntAtomOrVecRecurse(Ast_گرێ_DType* nodep, منطقي ranged = خطأ) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        إذا (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            إذا (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                أرجع dtp;  // Packed when already packed
            إذا (dtp->keyword().isIntNumeric()) أرجع nullptr;
            أرجع dtp;
        } وإلا إذا (AstPackArrayDType* ثابت dtp = VN_CAST(nodep, PackArrayDType)) {
            إذا (ranged) أرجع dtp;  // Packed when already packed
            أرجع dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), true);
        }
        أرجع nodep;
    }
    Ast_گرێ_DType* dtypeNot4StateIntegralRecurse(Ast_گرێ_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        إذا (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            إذا (!dtp->keyword().isFourstate()) أرجع dtp;
            أرجع nullptr;
        } وإلا إذا (Ast_گرێ_Array_DType* ثابت dtp = VN_CAST(nodep, گرێ_Array_DType)) {
            أرجع dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } وإلا إذا (Ast_گرێ_UOr_Struct_DType* ثابت dtp = VN_CAST(nodep, گرێ_UOr_Struct_DType)) {
            لكل (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_گرێ_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                إذا (badDtp) أرجع badDtp;
            }
            أرجع nullptr;
        }
        أرجع nodep;
    }
    فراغ checkForceReleaseLhs(Ast_گرێ* nodep, Ast_گرێ* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        إذا (Ast_گرێ* ثابت selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    فراغ assert_At_دەربڕین(Ast_گرێ* nodep) {
        إذا (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    فراغ assertAtStatement(Ast_گرێ* nodep) {
        إذا (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    فراغ checkConstantOrReplace(Ast_گرێ* nodep, منطقي noFourState, ثابت string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (!constp || (noFourState && constp->ژمارە().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(پاڵنان_Deletep(nodep), nodep, constp);
            أرجع;
        }
    }
    Ast_گرێ* گرێ_For_Unsized_Warning(Ast_گرێ* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        إذا (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            أرجع nodep->op1p();
        } وإلا إذا (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            أرجع nodep->op2p();
        }
        أرجع nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_گرێ* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        إذا (VL_LIKELY(!nodep)) أرجع nullptr;
        إذا (auto* ثابت refp = VN_CAST(nodep, RefDType)) {
            إذا (refp->typedefp() == typedefp) أرجع refp;
        }
        إذا (auto* ثابت refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) أرجع refp;
        إذا (auto* ثابت refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) أرجع refp;
        إذا (auto* ثابت refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) أرجع refp;
        إذا (auto* ثابت refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) أرجع refp;
        أرجع nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    ساكن منطقي isIfaceArrayDType(ثابت Ast_گرێ_DType* dtypep) {
        إذا (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        ثابت AstIfaceRefDType* ثابت irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        أرجع irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    ساكن std::string unpackedArraySize(ثابت Ast_گرێ_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        طالما (ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            إذا (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        أرجع size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    ساكن Ast_گرێ_دەربڕین* newIfaceArrayElement(Ast_گرێ_دەربڕین* fromp, صحيح bit) {
        AstConsArray* ثابت consp = VN_CAST(fromp, ConsArray);
        إذا (!consp || !isIfaceArrayDType(consp->dtypep())) أرجع nullptr;
        ثابت AstUnpackArrayDType* ثابت arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (bit < 0 || bit >= arrp->elementsConst()) أرجع nullptr;
        أرجع consp->valuep(bit)->clone_درەخت(خطأ);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_گرێ_دەربڕین* new_Iface_Array_بەها(ثابت Ast_گرێ_Var_Ref* refp, Ast_گرێ_DType* dtypep,
                                    ثابت std::string& name, ثابت std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_هێڵ* ثابت flp = refp->fileline();
        AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        إذا (!arrp) {
            AstVar* ثابت varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_گرێ_Var_Ref* newp;
            إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(refp, VarXRef)) {
                newp = جديد AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } وإلا {
                newp = جديد AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            أرجع newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ثابت consp = جديد AstConsArray{flp, arrp};
        ثابت VNumRange range = arrp->declRange();
        لكل (ثابت صحيح i : range.seq_چەپ_To_ڕاست()) {
            ثابت std::string s = name + "__BRA__" + Ast_گرێ::encode_ژمار(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_بەها(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        أرجع consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    فراغ replaceSplitIfaceArrayRef(Ast_گرێ_Var_Ref* nodep) {
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        إذا (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_بەها(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(پاڵنان_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    فراغ expandIfaceArrayPinDimensions(AstPin* nodep, ثابت Ast_گرێ_DType* dtypep,
                                       Ast_گرێ_دەربڕین* conp, ثابت std::string& suffix,
                                       AstVar*& elemVarpr) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        إذا (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ثابت newp = nodep->clone_درەخت(خطأ);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            إذا (conp) newp->exprp(conp->clone_درەخت(خطأ));
            nodep->addNextHere(newp);
            أرجع;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ثابت VNumRange range = arrp->declRange();
        لكل (صحيح n = range.elements() - 1; n >= 0; --n) {
            ثابت صحيح i = range.چەپ() + n * range.چەپ_To_ڕاست_Inc();
            ثابت std::string s = suffix + "__BRA__" + Ast_گرێ::encode_ژمار(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_گرێ_دەربڕین* ثابت elemConnp = [&]() -> Ast_گرێ_دەربڕین* {
                إذا (!conp) أرجع nullptr;
                ثابت AstUnpackArrayDType* ثابت conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ثابت صحيح elements = conArrp->elementsConst();
                // Counted from lo
                Ast_گرێ_دەربڕین* ثابت resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                أرجع resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    فراغ expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ثابت portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        لكل (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_گرێ_دەربڕین* ثابت exprp = VN_AS(nodep->exprp(), گرێ_دەربڕین);
        إذا (exprp) پاڵنان_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ثابت منطقي connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(پاڵنان_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_گرێ* userIterateSubtreeReturnEdits(Ast_گرێ* nodep, WidthVP* vup) {
        إذا (!nodep) أرجع nullptr;
        Ast_گرێ* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        أرجع ret;
    }
    فراغ userIterate(Ast_گرێ* nodep, WidthVP* vup) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    فراغ userIterateAndNext(Ast_گرێ* nodep, WidthVP* vup) {
        إذا (!nodep) أرجع;
        إذا (nodep->didWidth()) أرجع;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    فراغ userIterateChildren(Ast_گرێ* nodep, WidthVP* vup) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    فراغ userIterateChildrenBackwardsConst(Ast_گرێ* nodep, WidthVP* vup) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

عام:
    // CONSTRUCTORS
    WidthVisitor(منطقي paramsOnly,  // [in] TRUE if we are considering parameters only.
                 منطقي doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_گرێ* mainAcceptEdit(Ast_گرێ* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        لكل (AstVar* ثابت varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(پاڵنان_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        أرجع nodep;
    }
    ساكن منطقي lowerAsFixedAggregate(ثابت Ast_گرێ_DType* ثابت dtypep) {
        أرجع dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    فراغ streamAssignLower(Ast_گرێ_Assign* nodep) {
        Ast_گرێ_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        إذا (Ast_گرێ_Stream* ثابت streamp = VN_CAST(nodep->rhsp(), گرێ_Stream)) {
            Ast_گرێ_DType* ثابت lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ثابت صحيح lwidth = lhsDTypeSkippedRefp->widthStream();
            ثابت صحيح rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            إذا (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            إذا (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                ثابت صحيح queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            إذا (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    جديد AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        إذا (Ast_گرێ_Stream* ثابت streamp = VN_CAST(nodep->lhsp(), گرێ_Stream)) {
            ثابت Ast_گرێ_DType* ثابت rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_گرێ_DType* ثابت lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            ثابت صحيح lwidth = lhsStreamDTypep->widthStream();
            ثابت صحيح rwidth = rhsDTypep->widthStream();
            إذا (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            إذا (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_گرێ_دەربڕین* ثابت streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_گرێ_دەربڕین* ثابت dstp = streamp->lhsp()->unlinkFrBack();
                Ast_گرێ_دەربڕین* srcp = nodep->rhsp()->unlinkFrBack();
                إذا (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_گرێ_دەربڕین::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } وإلا {
                    إذا (srcp->width() > lwidth) {
                        srcp = جديد AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(پاڵنان_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(جديد AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } وإلا إذا (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_گرێ_دەربڕین* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(جديد AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = افتراضي;
};

//######################################################################
// Width class functions

فراغ V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ثابت WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ثابت VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{خطأ, خطأ};
        (فراغ)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (فراغ)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درەخت("width", 0, dump_درەخت_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_گرێ* V3Width::widthParamsEdit(Ast_گرێ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{true, خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    أرجع nodep;
}

فراغ V3Width::streamAssignLowerEdit(Ast_گرێ_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{خطأ, خطأ};
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
Ast_گرێ* V3Width::widthGenerateParamsEdit(
    Ast_گرێ* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{true, true};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    أرجع nodep;
}

Ast_گرێ* V3Width::selectNonConstantRecurse(Ast_گرێ* nodep, منطقي inSel) {
    // If node has a non-constant select, return that select
    Ast_گرێ* resultp = nullptr;
    إذا (Ast_گرێ_Sel* ثابت anodep = VN_CAST(nodep, گرێ_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        إذا (resultp) أرجع resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), true);
    } وإلا إذا (AstSel* ثابت anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        إذا (resultp) أرجع resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), true);
    } وإلا إذا (Ast_گرێ_Var_Ref* ثابت anodep = VN_CAST(nodep, گرێ_Var_Ref)) {
        إذا (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) أرجع anodep;
    } وإلا {
        إذا (Ast_گرێ* ثابت refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        إذا (resultp) أرجع resultp;
        إذا (Ast_گرێ* ثابت refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        إذا (resultp) أرجع resultp;
        إذا (Ast_گرێ* ثابت refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        إذا (resultp) أرجع resultp;
        إذا (Ast_گرێ* ثابت refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    أرجع resultp;
}
