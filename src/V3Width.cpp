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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Width.h"

#شامل "V3Assert.h"
#شامل "V3Ast.h"
#شامل "V3Begin.h"
#شامل "V3Const.h"
#شامل "V3ConstPool.h"
#شامل "V3Error.h"
#شامل "V3Global.h"
#شامل "V3LinkLValue.h"
#شامل "V3MemberMap.h"
#شامل "V3Number.h"
#شامل "V3Randomize.h"
#شامل "V3String.h"
#شامل "V3Task.h"
#شامل "V3UniqueNames.h"
#شامل "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#شامل "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

رده ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ثابت Ast_گره*, AstClass*> m_cache;

    AstClass* findCached(Ast_گره* nodep) {
        اگر (!nodep) بازگشت nullptr;
        ثابت auto it = m_cache.find(nodep);
        اگر (it != m_cache.end()) بازگشت it->second;

        AstClass* classp = nullptr;
        اگر (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } وگرنه اگر (AstClass* ثابت parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } وگرنه اگر (AstClassPackage* ثابت packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } وگرنه {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        بازگشت classp;
    }

عمومی:
    AstClass* find(Ast_گره* nodep) {
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) بازگشت classp;
        اگر (AstClassPackage* ثابت packagep = VN_CAST(nodep, ClassPackage)) {
            بازگشت packagep->classp();
        }
        بازگشت findCached(nodep);
    }
};

//######################################################################

شمارش Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, ثابت Stage& rhs) {
    بازگشت str << ("-PFB"[static_cast<صحیح>(rhs)]);
}

شمارش Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, ثابت Determ& rhs) {
    ایستا constexpr ثابت نویسه* ثابت s_det[] = {"SELF", "CNTX", "ASSN"};
    بازگشت str << s_det[rhs];
}

شمارش StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, پیام) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               پیام)

//######################################################################
// Width state, as a visitor of each AstNode

رده WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_گره_DType* ثابت m_dtypep;  // Parent's data type to resolve to
    ثابت Stage m_stage;  // If true, report errors
    ثابت StreamUse m_streamUse;  // Current expression may be a stream concat

عمومی:
    WidthVP(Ast_گره_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        اگر (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { بازگشت این; }
    منطقی selfDtm() ثابت { بازگشت m_dtypep == nullptr; }
    StreamUse streamUse() ثابت { بازگشت m_streamUse; }
    Ast_گره_DType* dtypep() ثابت {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        بازگشت m_dtypep;
    }
    Ast_گره_DType* dtypeNullp() ثابت { بازگشت m_dtypep; }
    Ast_گره_DType* dtypeNullSkipRefp() ثابت {
        Ast_گره_DType* dtp = dtypeNullp();
        اگر (dtp) dtp = dtp->skipRefp();
        بازگشت dtp;
    }
    Ast_گره_DType* dtypeOverridep(Ast_گره_DType* ثابت defaultp) ثابت {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        اگر (!m_dtypep) بازگشت defaultp;
        اگر (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            بازگشت defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        بازگشت m_dtypep;
    }
    صحیح width() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        بازگشت m_dtypep->width();
    }
    صحیح widthMin() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        بازگشت m_dtypep->widthMin();
    }
    منطقی prelim() ثابت { بازگشت m_stage & PRELIM; }
    منطقی final() ثابت { بازگشت m_stage & FINAL; }
    تهی dump(std::ostream& str) ثابت {
        اگر (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } وگرنه {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, ثابت WidthVP* vup) {
    اگر (vup) vup->dump(str);
    بازگشت str;
}

//######################################################################

رده WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    تهی clearWidthRecurse(Ast_گره* nodep) {
        برای (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(نادرست);
            اگر (Ast_گره* ثابت refp = nodep->op1p()) clearWidthRecurse(refp);
            اگر (Ast_گره* ثابت refp = nodep->op2p()) clearWidthRecurse(refp);
            اگر (Ast_گره* ثابت refp = nodep->op3p()) clearWidthRecurse(refp);
            اگر (Ast_گره* ثابت refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

عمومی:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    مجازی ~WidthClearVisitor() = پیشفرض;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_گره_iterate_instead_of_Ast_گره_accept

//######################################################################

رده WidthVisitor final : عمومی VNVisitor {
    // TYPES
    using PatVecMap = std::map<صحیح, AstPatMember*>;
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
    منطقی m_underFork = نادرست;  // Visiting under a fork
    منطقی m_underSExpr = نادرست;  // Visiting under a sequence expression
    منطقی m_underPackedArray = نادرست;  // Visiting under a AstPackArrayDType
    منطقی m_underMemberSel = نادرست;  // Viting under a MemberSel
    Ast_گره* m_seqUnsupp = nullptr;  // Property has unsupported node
    منطقی m_hasSExpr = نادرست;  // Property has a sequence expression
    ثابت AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    ثابت AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_گره_Module* m_modep = nullptr;  // Current module
    ثابت AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_گره_Procedure* m_procedurep = nullptr;  // Current final/always
    ثابت AstWith* m_withp = nullptr;  // Current 'with' statement
    ثابت AstFunc* m_funcp = nullptr;  // Current function
    ثابت AstAttrOf* m_attrp = nullptr;  // Current attribute
    ثابت Ast_گره_عبارت* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    ثابت منطقی m_paramsOnly;  // Computing parameter value; limit operation
    ثابت منطقی m_doGenerate;  // Do errors later inside generate statement
    منطقی m_streamConcat = نادرست;  // True if visiting arguments of stream concatenation
    منطقی m_inParamOverride = نادرست;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ثابت Ast_گره_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<ثابت Ast_گره_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<ثابت AstVar*> m_curModVars;  // Variables declared in current module

    ایستا constexpr صحیح ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    شمارش ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    شمارش رده ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    ایستا StreamUse streamUseForDeterm(Determ determ) {
        بازگشت determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() ثابت { بازگشت m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    ایستا تهی packIfUnpacked(Ast_گره_عبارت* ثابت nodep) {
        اگر (AstUnpackArrayDType* ثابت unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ثابت صحیح elements_رقم = unpackDTypep->arrayUnpackedElements();
            ثابت صحیح unpackMinBits = elements_رقم * unpackDTypep->subDTypep()->widthMin();
            ثابت صحیح unpackBits = elements_رقم * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(جدید AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    ایستا Ast_گره_DType* fromDTypep(Ast_گره* fromp) {
        اگر (Ast_گره_DType* ثابت dtypep = VN_CAST(fromp, گره_DType))
            بازگشت dtypep->skipRefOrNullp();
        بازگشت fromp ? fromp->dtypep() : nullptr;
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
    تهی visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    تهی visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    تهی visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    تهی visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    تهی visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    تهی visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    تهی visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    تهی visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    تهی visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    تهی visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    تهی visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    تهی visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    تهی visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    تهی visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    تهی visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    تهی visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    تهی visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    تهی visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    تهی visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    تهی visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    تهی visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    تهی visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    تهی visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    تهی visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    تهی visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    تهی visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    تهی visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    تهی visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    تهی visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    تهی visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    تهی visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    تهی visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    تهی visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    تهی visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    تهی visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    تهی visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    تهی visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    تهی visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    تهی visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    تهی visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    تهی visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    تهی visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    تهی visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    تهی visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    تهی visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    تهی visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    تهی visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    تهی visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, درست, نادرست); }
    تهی visit(AstSub* nodep) override { visit_add_sub_replace(nodep, درست, نادرست); }
    تهی visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, درست, درست); }
    تهی visit(AstMul* nodep) override { visit_add_sub_replace(nodep, درست, نادرست); }
    // These can't promote to real
    تهی visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, نادرست, درست); }
    تهی visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, نادرست, درست); }
    تهی visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, نادرست, نادرست); }
    تهی visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, نادرست, درست); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    تهی visit(AstNegate* nodep) override { visit_negate_not(nodep, درست); }
    // Unary never real
    تهی visit(AstNot* nodep) override { visit_negate_not(nodep, نادرست); }

    // Real: inputs and output real
    تهی visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    تهی visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    تهی visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    تهی visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    تهی visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    تهی visit(Ast_گره_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    تهی visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    تهی visit(Ast_گره_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    تهی visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    تهی visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    تهی visit(AstShiftL* nodep) override { visit_shift(nodep); }
    تهی visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    تهی visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    تهی visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    تهی visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    تهی visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    تهی visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    تهی visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    تهی visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        اگر (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    تهی visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        اگر (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    تهی visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        اگر (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    تهی visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        اگر (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    تهی visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    تهی visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            اگر (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } وگرنه {
                nodep->dtypeSetInteger();
            }
        }
    }
    تهی visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    تهی visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    تهی visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    تهی visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    تهی visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    تهی visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            جدید AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    ایستا منطقی isOverridePattern(ثابت Ast_گره* nodep) {
        اگر (ثابت AstPattern* ثابت patternp = VN_CAST(nodep, Pattern)) {
            بازگشت !patternp->childDTypep();
        }
        ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond);
        بازگشت condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    تهی visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_عبارت(nodep);
        اگر (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ثابت صحیح errors = V3Error::error_شمار();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = نادرست;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const);
            ثابت منطقی known = constp && !constp->رقم().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            اگر (!known && V3Error::error_شمار() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_گره_عبارت* ثابت keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            بازگشت;
        }
        اگر (m_vup->prelim()) {  // First stage evaluation
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
            Ast_گره_DType* ثابت thenDTypep = nodep->thenp()->dtypep();
            Ast_گره_DType* ثابت elseDTypep = nodep->elsep()->dtypep();
            Ast_گره_DType* ثابت thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_گره_DType* ثابت elseDTypeSkippedp = elseDTypep->skipRefp();
            اگر (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } وگرنه اگر (nodep->thenp()->is_Class_Handle_مقدار()
                       || nodep->elsep()->is_Class_Handle_مقدار()) {
                Ast_گره_DType* commonClassTypep = nullptr;
                اگر (nodep->thenp()->is_Class_Handle_مقدار() && nodep->elsep()->is_Class_Handle_مقدار()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_گره::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                اگر (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } وگرنه {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } وگرنه اگر (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                اگر (Ast_گره::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } وگرنه {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    اگر (!Ast_گره::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } وگرنه اگر (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } وگرنه اگر (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } وگرنه {
                ثابت منطقی isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                منطقی handled = نادرست;
                ثابت منطقی isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_گره_DType* ثابت thenEnumDTypep = thenDTypep->skipRefToEnump();
                اگر (VN_IS(thenDTypeSkippedp, گره_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_درخت(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = درست;
                } وگرنه اگر (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_درخت(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    اگر (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = درست;
                    } وگرنه {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(جدید File_سطر{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, درست);
                    }
                }
                اگر (!handled) {
                    ثابت صحیح width = std::max(thenDTypep->width(), elseDTypep->width());
                    ثابت صحیح widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    ثابت VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ثابت منطقی isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    اگر ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } وگرنه اگر ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } وگرنه {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        اگر (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    تهی visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        اگر (Ast_گره_عبارت* ثابت newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        اگر (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* ثابت newp = جدید AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            بازگشت;
        }

        // Concat handling
        اگر (m_vup->prelim()) {
            اگر (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            اگر (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } وگرنه {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                اگر (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            اگر (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->lhsp(), Replicate)) {
                اگر (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
            }
            اگر (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->rhsp(), Replicate)) {
                اگر (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
            }
        }
        اگر (m_vup->final()) {
            اگر (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_گره_عبارت* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_گره_عبارت* rhsp = nodep->rhsp()->unlinkFrBack();
                اگر (!lhsp->isString()) lhsp = جدید AstCvtPackString{lhsp->fileline(), lhsp};
                اگر (!rhsp->isString()) rhsp = جدید AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_گره* ثابت newp = جدید AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            }
            اگر (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    تهی visit(AstConcatN* nodep) override {
        اگر (nodep->didWidth()) بازگشت;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        اگر (m_vup->final()) {
            nodep->didWidth(درست);
            اگر (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    تهی visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    تهی visit(AstDelay* nodep) override {
        اگر (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            اگر (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            اگر (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                بازگشت;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            بازگشت;
        }
        اگر (Ast_گره_عبارت* ثابت fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            بازگشت;
        }
        اگر (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        اگر (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        اگر (nodep->fileline()->timingOn()) {
            اگر (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                بازگشت;
            } وگرنه اگر (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } وگرنه {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        اگر (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    تهی visit(AstFork* nodep) override {
        اگر (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        اگر (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ثابت newp = جدید AstBegin{nodep->fileline(), nodep->name(), nullptr, نادرست};
            nodep->replaceWith(newp);
            اگر (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            اگر (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            اگر (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه اگر (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = درست;
            iterateChildren(nodep);
        } وگرنه اگر (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        } وگرنه {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    تهی visitWaitOrDisableFork(Ast_گره* nodep) {
        اگر (nodep->fileline()->timingOn()) {
            اگر (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
                بازگشت;
            } وگرنه اگر (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    تهی visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    تهی visit(AstToLowerN* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    تهی visit(AstToUpperN* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    تهی visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
            اگر (constp) {
                اگر (constp->رقم().isFourState()
                    || (constp->dtypep()->isSigned() && constp->رقم().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } وگرنه {
                    times = constp->toSInt();
                }
            }

            Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
            اگر (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                اگر (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                اگر (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                } وگرنه {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* ثابت newp = جدید AstPattern{
                        nodep->fileline(),
                        جدید AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    بازگشت;
                }
            }
            اگر (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            اگر (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } وگرنه {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            اگر ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_گره* ثابت newp
                    = جدید AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            } وگرنه {
                اگر (!constp) nodep->v3error("Replication value isn't a constant.");
                اگر (times == 0
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
        اگر (m_vup->final()) {
            اگر (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    تهی visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        اگر (m_vup->final()) {
            اگر (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    تهی visit(Ast_گره_Dist_Biop* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    تهی visit(Ast_گره_Dist_Triop* nodep) override {
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    منطقی streamImplicitUseAllowed(ثابت Ast_گره_Stream* nodep) ثابت {
        اگر (m_streamConcat) بازگشت درست;
        اگر (m_vup && m_vup->streamUse() != STREAM_USE_NONE) بازگشت درست;

        ثابت Ast_گره* ثابت backp = nodep->backp();
        اگر (!backp) بازگشت درست;  // Error elsewhere

        بازگشت VN_IS(backp, گره_Assign);
    }
    تهی visit(Ast_گره_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            m_streamConcat = درست;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = نادرست;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const)) {
                اگر (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } وگرنه {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ثابت Ast_گره_DType* ثابت lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            اگر (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, گره_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, گره_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } وگرنه اگر (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } وگرنه {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        اگر (m_vup->final()) {
            اگر (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            اگر (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    تهی visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), درست,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), درست,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        اگر (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ثابت صحیح width = nodep->elementsConst();
            اگر (nodep->fromBracket() && nodep->چپ_Const() > nodep->راست_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } وگرنه اگر (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ثابت منطقی inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ثابت منطقی inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ثابت منطقی inTypeTable = !m_modep;
            ثابت Ast_گره* basep = nodep->backp();
            وقتی (VN_IS(basep, Range)) basep = basep->backp();
            اگر (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->چپ_Const() << ":" << nodep->راست_Const()
                                            << "]");
            }
        }
    }

    تهی visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        اگر (nodep->didWidth()) بازگشت;
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), نادرست);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            صحیح width = nodep->widthConst();
            اگر (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                بازگشت;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ثابت منطقی isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ثابت منطقی inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            اگر (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                افزودن_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(جدید AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            اگر (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                اگر (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_گره_DType* ثابت subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    نادرست /*noerror*/);
                } وگرنه {
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
            صحیح frommsb = nodep->fromp()->width() - 1;
            صحیح fromlsb = 0;
            ثابت صحیح elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            اگر (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } وگرنه {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ثابت صحیح selwidth = V3_عدد::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_گره_DType* ثابت selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            اگر (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                اگر (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            اگر (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                اگر (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } وگرنه اگر (!inParameterizedTemplate) {
                    اگر (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } وگرنه {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                اگر (!isWriteSelect) {
                    // Extend it
                    ثابت صحیح extendTo = nodep->msbConst() + 1;
                    Ast_گره_DType* ثابت subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    نادرست /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            اگر (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                نادرست /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    تهی visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            اگر (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->bitp(), Const)) {
                    ثابت صحیح bit = constp->toSInt();
                    Ast_گره_عبارت* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    اگر (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ثابت AstUnpackArrayDType* ثابت arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
            }
            //
            صحیح frommsb;
            صحیح fromlsb;
            ثابت Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                اگر (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                اگر (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } وگرنه {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ثابت صحیح selwidth = V3_عدد::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_گره_DType* ثابت selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            اگر (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                اگر (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            اگر (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                اگر (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    اگر (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                نادرست /*NOWARN*/);
            }
        }
    }

    تهی visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            ثابت Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstAssocArrayDType* ثابت adtypep = VN_CAST(fromDtp, AssocArrayDType);
            اگر (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->کلید_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    تهی visit(AstAlias* nodep) override {
        اگر (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        ثابت auto check_If_عبارت_Ok = [این](ثابت Ast_گره_عبارت* ثابت exprp) {
            اگر (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                بازگشت نادرست;
            }

            ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef);
            اگر (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                بازگشت نادرست;
            }
            AstVar* ثابت varp = varRefp->varp();
            اگر (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                بازگشت نادرست;
            }
            اگر (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                بازگشت نادرست;
            } وگرنه {
                m_aliasedVars.درج(varp);
            }
            بازگشت درست;
        };

        check_If_عبارت_Ok(nodep->itemsp());
        Ast_گره_DType* ثابت firstItemDtypep = nodep->itemsp()->dtypep();
        برای (Ast_گره* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_عبارت_Ok(VN_AS(itemp, گره_عبارت));
            اگر (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    تهی visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            ثابت Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstWildcardArrayDType* ثابت adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            اگر (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ثابت AstBasicDType* ثابت basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            اگر (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    تهی visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ثابت Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            اگر (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_گره_DType* ثابت newDtp
                = جدید AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          جدید AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            اگر (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ثابت صحیح subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ثابت VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                اگر ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(جدید AstConsArray{
                        nodep->fileline(), newDtp,
                        جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                } وگرنه اگر ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            اگر (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ثابت fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ثابت newp = جدید AstConsArray{nodep->fileline(), newDtp};
                ثابت VNumRange range = nodep->declRange();
                برای (صحیح n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_درخت(نادرست));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }

    تهی visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_گره* ثابت selp = V3Width::widthSelNoIterEdit(nodep);
        اگر (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            بازگشت;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    تهی visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_گره* ثابت selp = V3Width::widthSelNoIterEdit(nodep);
        اگر (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            بازگشت;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    تهی visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_گره* ثابت selp = V3Width::widthSelNoIterEdit(nodep);
        اگر (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            بازگشت;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    تهی visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_گره* ثابت selp = V3Width::widthSelNoIterEdit(nodep);
        اگر (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            بازگشت;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    تهی visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        اگر (nodep->didWidthAndSet()) بازگشت;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        اگر (nodep->didWidthAndSet()) بازگشت;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        اگر (nodep->didWidthAndSet()) بازگشت;
        اگر (m_vup && m_vup->prelim()) {
            اگر (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } وگرنه اگر (nodep->رقم().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } وگرنه اگر (nodep->رقم().isString()) {
                nodep->dtypeSetString();
            } وگرنه اگر (nodep->رقم().isDouble()) {
                nodep->dtypeChgWidth(nodep->رقم().width(), nodep->رقم().sized()
                                                               ? nodep->رقم().width()
                                                               : nodep->رقم().widthToFit());
            } وگرنه {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->رقم().width(),
                    nodep->رقم().sized() ? nodep->رقم().width() : nodep->رقم().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->رقم().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    تهی visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        اگر (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    تهی visit(AstFell* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstFalling* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstFuture* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    تهی visit(AstPast* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtypeFrom(nodep->exprp());
            اگر (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ درست);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ثابت AstConst* ثابت constp = VN_CAST(nodep->ticksp(), Const);
                اگر (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_درخت();
                } وگرنه اگر (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_درخت();
                } وگرنه اگر (V3AssertCommon::check_Sva_مقدار_Limit(nodep, constp, "$past tick value")) {
                    اگر (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    تهی visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
            اگر (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } وگرنه اگر (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } وگرنه اگر (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            اگر (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
                اگر (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } وگرنه اگر (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } وگرنه اگر (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_گره_عبارت* propp = nodep->propp();
            اگر (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            اگر (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ثابت منطقی loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ثابت منطقی hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            اگر (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_گره* ثابت boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                بازگشت;
            }
            اگر (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                بازگشت;
            }
            ثابت AstConst* ثابت loConstp = VN_CAST(nodep->loBoundp(), Const);
            ثابت AstConst* ثابت hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            اگر (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            اگر (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            اگر (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } وگرنه اگر (loConstp) {
                V3AssertCommon::check_Sva_مقدار_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            اگر (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_مقدار_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            اگر (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            منطقی hasPropertyOp = propp->isMultiCycleSva();
            اگر (!hasPropertyOp) {
                propp->foreach([&](ثابت Ast_گره* np) {
                    اگر (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = درست;
                    }
                });
            }
            اگر (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstRising* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstRose* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    تهی visit(AstSampled* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    تهی visit(AstSetuphold* nodep) override {
        File_سطر* ثابت flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        اگر (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        اگر (nodep->deldatap()) {
            اگر (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } وگرنه {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        اگر (!newp) {
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        nodep->replaceWith(جدید AstAlways{newp});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت evp,
                                         Ast_گره_عبارت* ثابت delp) {
        Ast_گره_عبارت* ثابت lhsp = delp->clone_درخت_Pure(نادرست);
        Ast_گره_عبارت* ثابت rhsp = evp->clone_درخت_Pure(نادرست);
        UASSERT_OBJ(VN_IS(lhsp, گره_Var_Ref) || VN_IS(lhsp, گره_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        اگر (Ast_گره_Var_Ref* varRefp = VN_CAST(lhsp, گره_Var_Ref)) {
            اگر (varRefp->varp()->direction() == VDirection::درون_داد) {
                VL_DO_DANGLING(افزودن_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
                بازگشت nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        اگر (Ast_گره_Pre_Sel* selp = VN_CAST(lhsp, گره_Pre_Sel)) {
            اگر (Ast_گره_Var_Ref* varRefp = VN_CAST(selp->fromp(), گره_Var_Ref)) {
                اگر (varRefp->varp()->direction() == VDirection::درون_داد) {
                    VL_DO_DANGLING(افزودن_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
                    بازگشت nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        بازگشت جدید AstAssignW{flp, lhsp, rhsp};
    }

    تهی visit(AstStable* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstSteady* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtypeSetBit();
        }
    }

    تهی visit(Ast_Stmt_عبارت* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    تهی visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    تهی visit(AstUntil* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            اگر (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            اگر (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    تهی visit(AstRand* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            اگر (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } وگرنه {
                nodep->dtypeSetInt();  // Says the spec
            }
            اگر (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    تهی visit(AstSEventually* nodep) override {
        اگر (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::Widthed_مقدار{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        }
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstSGotoRep* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            اگر (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } وگرنه {
                width_Check_Rep_شمار(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstSNonConsRep* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            اگر (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } وگرنه {
                width_Check_Rep_شمار(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    تهی width_Check_Rep_شمار(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
        اگر (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } وگرنه اگر (constp->dtypep()->isSigned() && constp->رقم().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } وگرنه اگر (constp->رقم().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } وگرنه اگر (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    تهی widthCheckGotoRepRange(T_Rep* nodep, ثابت نویسه* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
        ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
        اگر (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            بازگشت;
        }
        اگر (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            بازگشت;
        }
        اگر (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            بازگشت;
        }
        اگر (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    تهی visit(AstSThroughout* nodep) override {
        m_hasSExpr = درست;
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = نادرست;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = درست;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = درست;
        m_hasSExpr = درست;
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            اگر (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = درست;
        m_hasSExpr = درست;
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            برای (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstURandomRange* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_گره_DType* ثابت expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    تهی visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        اگر (VN_IS(nodep->backp(), IsUnbounded)) بازگشت;  // Ok, leave
        اگر (VN_IS(nodep->backp(), BracketArrayDType)) بازگشت;  // Ok, leave
        اگر (VN_IS(nodep->backp(), InsideRange)) بازگشت;  // Ok, leave
        اگر (ثابت auto* ثابت varp = VN_CAST(nodep->backp(), Var)) {
            اگر (varp->isParam()) بازگشت;  // Ok, leave
        }
        Ast_گره* backp = nodep->backp();
        اگر (VN_IS(backp, Sub)) backp = backp->backp();
        اگر (ثابت auto* ثابت selp = VN_CAST(backp, SelExtract)) {
            اگر (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) بازگشت;
        }
        اگر (ثابت auto* ثابت selp = VN_CAST(backp, SelBit)) {
            اگر (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) بازگشت;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    تهی visit(AstInferredDisable* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) nodep->dtypeSetBit();
    }
    تهی visit(AstIsUnbounded* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    تهی visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        اگر (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        اگر (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            اگر (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    تهی visit(AstCLog2* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    تهی widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_گره_عبارت* ثابت valuep = nodep->valuep();
        ثابت std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            اگر (!constp->رقم().isNegative()) بازگشت;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } وگرنه اگر (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } وگرنه {
            بازگشت;
        }
        // Continue with the default weight
        valuep->replaceWith(جدید AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    تهی widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    تهی visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ثابت cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        اگر (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            بازگشت;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        اگر (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            بازگشت;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        اگر (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            اگر (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } وگرنه {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } وگرنه اگر (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_گره_عبارت* ثابت valuep = nodep->valuep();
            اگر (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(جدید AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            }
            بازگشت;
        } وگرنه اگر (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            بازگشت;
        } وگرنه اگر (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ثابت AstConst* ثابت constp = VN_CAST(nodep->valuep(), Const);
            اگر (!constp || !constp->رقم().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstCoverOption* nodep) override {
        اگر (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            بازگشت;
        }
        اگر (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            بازگشت;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        اگر (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    تهی visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        اگر (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    تهی visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        اگر (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    تهی widthCovergroupRanges(Ast_گره* rangesp, صحیح fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ثابت auto fill = [&](Ast_گره* nodep) {
            اگر (!fillWidth) بازگشت;
            Ast_گره_عبارت* exprp = VN_AS(nodep, گره_عبارت);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        برای (Ast_گره *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (AstInsideRange* ثابت rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } وگرنه اگر (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } وگرنه {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    تهی visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    تهی visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        اگر (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        اگر (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    تهی visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        اگر (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        اگر (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    تهی visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        اگر (Ast_گره_عبارت* ثابت countp = nodep->countp()) {
            اگر (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } وگرنه {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    تهی widthCoverFilter(Ast_گره_عبارت* filterp, ثابت نویسه* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), گره_عبارت);
        File_سطر* ثابت fl = filterp->fileline();
        ثابت Ast_گره_DType* ثابت resultp = filterp->dtypep()->skipRefp();
        اگر (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                جدید AstNeqD{fl, filterp, جدید AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } وگرنه اگر (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(جدید AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(filterp), filterp);
        } وگرنه {
            fixWidthReduce(filterp);
        }
    }
    تهی visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    تهی visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    تهی visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            اگر (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                بازگشت;
            }
            checkCvtUS(nodep->lhsp(), نادرست);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            اگر (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->رقم().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        اگر (m_vup->final()) {
            Ast_گره_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_گره* newp = nullptr;  // No change
            اگر (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = جدید AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } وگرنه اگر (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = جدید AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } وگرنه اگر (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = جدید AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            اگر (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }
    تهی visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(Ast_شمار_Bits* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحیح widthMin = V3_عدد::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    تهی visit(Ast_شمار_Ones* nodep) override {
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحیح widthMin = V3_عدد::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    تهی visit(AstCvtPackString* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        اگر (Ast_گره_DType* ثابت dt = nodep->lhsp()->dtypep()) {
            اگر (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    تهی visit(AstCvtPackedToArray* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    تهی visit(AstCvtArrayToArray* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    تهی visit(AstCvtArrayToPacked* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    تهی visit(AstCvtUnpackToQueue* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    تهی visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        اگر (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    تهی visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ثابت دوگانه timePrescale = constp->رقم().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ثابت دوگانه time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ثابت newp = جدید AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstEventControl* nodep) override {
        اگر (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }
        اگر (nodep->fileline()->timingOn()) {
            اگر (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                بازگشت;
            } وگرنه اگر (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } وگرنه {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        اگر (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    ایستا string typenameExtends(ثابت AstClass* classp) {
        // An interface class may extend several, so those are not named
        اگر (!classp || classp->isInterfaceClass()) بازگشت "";
        برای (ثابت AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            اگر (extendsp->isImplements()) ادامه;
            ثابت AstClass* ثابت basep = extendsp->classOrNullp();
            بازگشت basep ? " extends class " + basep->dtypeName(درست) : "";
        }
        بازگشت "";
    }
    تهی visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        اگر (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        انتخاب (nodep->attrType()) {
        حالت VAttrType::DIM_DIMENSIONS:
        حالت VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_گره_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ثابت std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(درست);
            ثابت صحیح val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(جدید AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بشکن;
        }
        حالت VAttrType::DIM_BITS_OR_عدد: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            اگر (VN_IS(nodep->fromp(), گره_عبارت)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه {
                Ast_گره* newp = جدید AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            بازگشت;
        }
        حالت VAttrType::DIM_BITS:
        حالت VAttrType::DIM_بالا:
        حالت VAttrType::DIM_INCREMENT:
        حالت VAttrType::DIM_چپ:
        حالت VAttrType::DIM_کم:
        حالت VAttrType::DIM_راست:
        حالت VAttrType::DIM_SIZE: {
            Ast_گره_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            اگر (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ثابت Ast_گره_Module* ثابت ownModp = v3Global.rootp()->containingModule(dtypep);
                اگر (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    بازگشت;
                }
            }

            اگر (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                انتخاب (nodep->attrType()) {
                حالت VAttrType::DIM_SIZE: {
                    Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), گره_عبارت);
                    Ast_گره* ثابت newp
                        = جدید AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(درست);
                    newp->protect(نادرست);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    بشکن;
                }
                حالت VAttrType::DIM_چپ:
                حالت VAttrType::DIM_کم: {
                    Ast_گره* ثابت newp = جدید AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بشکن;
                }
                حالت VAttrType::DIM_راست:
                حالت VAttrType::DIM_بالا: {
                    Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), گره_عبارت);
                    Ast_گره_عبارت* ثابت sizep
                        = جدید AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(درست);
                    sizep->protect(نادرست);
                    Ast_گره* ثابت newp
                        = جدید AstSub{nodep->fileline(), sizep,
                                     جدید AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    بشکن;
                }
                حالت VAttrType::DIM_INCREMENT: {
                    Ast_گره_عبارت* ثابت newp
                        = جدید AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بشکن;
                }
                حالت VAttrType::DIM_BITS: {
                    اگر (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } وگرنه {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    بشکن;
                }
                پیشفرض: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } وگرنه {
                ثابت std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(درست);
                ثابت uint32_t msbdim = dimpair.first + dimpair.second;
                اگر (!nodep->dimp() || msbdim < 1) {
                    اگر (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), گره_عبارت);
                        Ast_گره* ثابت newp = جدید AstShiftL{
                            nodep->fileline(), جدید AstLenN{nodep->fileline(), fromp},
                            جدید AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    } وگرنه {
                        Ast_گره_DType* ثابت baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        ثابت صحیح dim = 1;
                        AstConst* ثابت newp = dimension_مقدار(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    }
                } وگرنه اگر (VN_IS(nodep->dimp(), Const)) {
                    ثابت صحیح dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* ثابت newp
                        = dimension_مقدار(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                } وگرنه {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ثابت tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_گره_عبارت* ثابت dimp = nodep->dimp()->unlinkFrBack();
                    Ast_گره_عبارت* ثابت newp = جدید AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                }
            }
            بشکن;
        }
        حالت VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            بشکن;
        حالت VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_گره_DType* ثابت protoDtp = nodep->fromp()->dtypep();
            Ast_گره_DType* ثابت declDtp = [&]() {
                اگر (m_ftaskp->fvarp()) بازگشت m_ftaskp->fvarp()->dtypep();
                Ast_گره_DType* ثابت voidp = جدید AstVoidDType{m_ftaskp->fileline()};
                افزودن_Deletep(voidp);  // Note voidp used past here
                بازگشت voidp;
            }();
            اگر (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            بشکن;
        }
        حالت VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_گره_DType* ثابت dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(درست);
            اگر (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_گره* ثابت newp = جدید AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بشکن;
        }
        حالت VAttrType::TYPEID:
            اگر (Ast_گره_DType* dtypep = VN_CAST(nodep->fromp(), گره_DType)) {
                nodep->dtypep(dtypep);
            } وگرنه {
                nodep->dtypep(VN_AS(nodep->fromp(), گره_عبارت)->dtypep());
            }
            بشکن;
        حالت VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            بشکن;
        پیشفرض: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            بشکن;
        }
        }
    }
    تهی visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    تهی visit(AstCellArrayRef* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(درست);
    }

    تهی visit(AstCellRef* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;

        اگر (Ast_گره_عبارت* ثابت cellExprp = VN_CAST(nodep->cellp(), گره_عبارت)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        اگر (Ast_گره_عبارت* ثابت exprp = VN_CAST(nodep->exprp(), گره_عبارت)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } وگرنه {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(درست);
    }

    // DTYPES
    تهی visit(Ast_گره_Array_DType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed

        اگر (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ثابت basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            اگر (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* ثابت newp = جدید AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        اگر (VN_IS(nodep, PackArrayDType)) m_underPackedArray = درست;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        اگر (AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            اگر (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(درست);
        } وگرنه {
            ثابت Ast_گره_DType* ثابت elemDTypep = nodep->subDTypep()->skipRefp();
            اگر (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ثابت صحیح width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            اگر (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ثابت Ast_گره_DType* baseDTypep = nodep->subDTypep();
                وقتی (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                ثابت AstBasicDType* ثابت basicp = nodep->basicp();
                اگر (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(درست));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    تهی visit(AstAssocArrayDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->کلید_DTypep(iterateEditMoveDTypep(nodep, nodep->کلید_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    تهی visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_گره_DType* ثابت childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_گره* ثابت elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_گره* newp;
        اگر (VN_IS(elementsp, Unbounded)) {
            newp = جدید AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_درخت(), elementsp);
        } وگرنه اگر (Ast_گره_DType* ثابت keyp = VN_CAST(elementsp, گره_DType)) {
            newp = جدید AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } وگرنه {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_سطر* ثابت elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, درست);
            // Must be expression that is constant, but we'll determine that later
            newp = جدید AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                جدید AstRange{nodep->fileline(), جدید AstConst(elementsp->fileline(), 0),
                             جدید AstSub{elementsNewFl, VN_AS(elementsp, گره_عبارت),
                                        جدید AstConst(elementsNewFl, 1)},
                             درست}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    تهی visit(AstConstraintRefDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    تهی visit(AstDynArrayDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    تهی visit(AstQueueDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        اگر (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_درخت();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    تهی visit(AstVoidDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    تهی visit(AstUnsizedArrayDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        اگر (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    تهی visit(AstWildcardArrayDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    تهی visit(AstBasicDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        اگر (nodep->generic()) بازگشت;  // Already perfect
        اگر (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } وگرنه اگر (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } وگرنه اگر (nodep->implicit()) {
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
    تهی visit(AstConstDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    تهی visit(AstRefDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(درست);
        اگر (nodep->typeofp()) {  // type(typeofp_expression)
            اگر (Ast_گره_DType* typeofDtp = VN_CAST(nodep->typeofp(), گره_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } وگرنه {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_گره* ثابت typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_درخت(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        اگر (nodep->subDTypep()) {
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
        اگر (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ثابت منطقی inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            اگر (inTemplateModule) {
                nodep->doingWidth(نادرست);
                بازگشت;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(نادرست);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    تهی widthDTypeNameParams(ثابت AstTypedef* nodep) {
        ثابت VDTypeNameScopes* ثابت scopesp = VDTypeNameScopes::currentp();
        اگر (!scopesp) بازگشت;  // Computing parameters, as by V3Param, so naming as found
        Ast_گره* scopep = nullptr;
        برای (ثابت Ast_گره* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            اگر (ثابت std::vector<Ast_گره*>* ثابت paramsp = scopesp->paramsp(scopep)) {
                برای (Ast_گره* ثابت paramp : *paramsp) {
                    اگر (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    تهی visit(AstTypedef* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        اگر (auto* ثابت refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ثابت منطقی declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        اگر (declares) {
            Ast_گره_DType* ثابت dtypep = nodep->dtypep();
            اگر (Ast_گره_UOr_Struct_DType* ثابت sdtypep = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } وگرنه اگر (AstEnumDType* ثابت edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    تهی visit(AstParamTypeDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        اگر (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    تهی visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        اگر (Ast_گره_DType* ثابت dtp = VN_CAST(nodep->lhsp(), گره_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } وگرنه {
            اگر (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(جدید AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_گره_DType* ثابت toDtp = nodep->بالاترین()->dtypep()->skipRefToEnump();
        Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_سطر* ثابت fl = nodep->fileline();
        ثابت auto castable = Ast_گره::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_گره* newp;
        اگر (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            بازگشت;
        } وگرنه اگر (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ثابت enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_گره_عبارت* ثابت testp = enumTestValid(nodep->fromp(), enumDtp);
            File_سطر* ثابت fl_novalue = جدید File_سطر{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, درست);
            newp = جدید AstCond{
                fl, testp,
                جدید Ast_عبارت_Stmt{fl,
                                جدید AstAssign{fl_novalue, nodep->بالاترین()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                جدید AstConst{fl, AstConst::Signed32{}, 1}},
                جدید AstConst{fl, AstConst::Signed32{}, 0}};
        } وگرنه اگر ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = جدید Ast_عبارت_Stmt{
                fl,
                جدید AstAssign{fl, nodep->بالاترین()->unlinkFrBack(),
                              جدید AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                جدید AstConst{fl, AstConst::Signed32{}, 1}};
        } وگرنه اگر (castable == VCastable::INCOMPATIBLE) {
            newp = جدید AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } وگرنه {
            newp = جدید AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    تهی visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        اگر (AstConst* ثابت constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_گره* ثابت newp
                = جدید AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } وگرنه اگر (Ast_گره_DType* ثابت refp = VN_CAST(nodep->dtp(), گره_DType)) {
            refp->unlinkFrBack();
            Ast_گره* ثابت newp = جدید AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } وگرنه {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی visit(AstCast* nodep) override {
        اگر (nodep->didWidth()) بازگشت;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        اگر (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            اگر (AstSigned* ثابت fromp = VN_CAST(nodep->fromp(), Signed)) {
                اگر (VN_IS(fromp->lhsp(), گره_Stream)) {
                    Ast_گره* ثابت lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_درخت(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_گره_DType* ثابت toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            ثابت auto castable = Ast_گره::computeCastable(toDtp, fromDtp, nodep->fromp());
            منطقی bad = نادرست;
            اگر (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = درست;
            } وگرنه اگر (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } وگرنه اگر (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = درست;
            } وگرنه اگر (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = درست;
            } وگرنه {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_گره_عبارت* newp = nullptr;
            اگر (bad) {
            } وگرنه اگر (AstBasicDType* ثابت basicp = toDtp->basicp()) {
                اگر (!basicp->isString() && fromDtp->isString()) {
                    newp = جدید AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } وگرنه اگر (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_گره_DType* ثابت origDTypep = nodep->dtypep();
                    اگر (!VN_IS(fromDtp, StreamDType)) {
                        ثابت صحیح width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } وگرنه {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 نادرست);
                }
                اگر (newp) {
                } وگرنه اگر (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    اگر (nodep->fromp()->isSigned()) {
                        newp = جدید AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } وگرنه {
                        newp = جدید AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } وگرنه اگر (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = جدید AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } وگرنه اگر (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = جدید AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } وگرنه اگر (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = جدید AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } وگرنه {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } وگرنه اگر (VN_IS(toDtp, QueueDType)) {
                اگر (VN_IS(fromDtp, BasicDType)) {
                    newp = جدید AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } وگرنه اگر (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    صحیح srcElementBits = 1;
                    صحیح dstElementBits = 1;
                    اگر (Ast_گره_DType* ثابت elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ثابت AstQueueDType* ثابت dstQueueDtp = VN_AS(toDtp, QueueDType);
                    اگر (Ast_گره_DType* ثابت elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = جدید AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  نادرست,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } وگرنه اگر (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } وگرنه {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            اگر (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        اگر (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, نادرست, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_گره_عبارت* ثابت underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(درست);
            Ast_گره_عبارت* ثابت newp = جدید AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(درست);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            صحیح width = nodep->rhsp()->toSInt();
            اگر (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        اگر (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_گره* ثابت underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    تهی visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    تهی castSized(Ast_گره* nodep, Ast_گره* underp, صحیح width) {
        ثابت AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        اگر (!underDtp) underDtp = underp->dtypep()->basicp();
        اگر (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ثابت صحیح calcWidth = std::max(width, underDtp->width());
            Ast_گره_DType* ثابت calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         نادرست, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_گره_DType* ثابت outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, گره_عبارت), outDtp, EXTEND_EXP,
                            نادرست);
            VL_DANGLING(underp);
        }
    }
    تهی visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(Ast_Constraint_عبارت* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    تهی visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        اگر (nodep->didWidth()) بازگشت;
        اگر (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            افزودن_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(درست);
            بازگشت;
        }
        nodep->doingWidth(درست);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        اگر (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.افزودن_back(nodep);
        اگر (nodep->attrsp()) {
            nodep->attrsp()->foreach([این, nodep](AstAttrOf* attrp) {
                اگر (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    اگر (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), گره_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(افزودن_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        اگر (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_گره_DType* dtp = nodep->dtypep();
            Ast_گره_DType* np = nullptr;
            وقتی (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                اگر (ثابت AstDynArrayDType* ثابت unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    اگر (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } وگرنه {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ثابت newp
                        = جدید AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    اگر (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } وگرنه {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } وگرنه {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        اگر (AstWildcardArrayDType* ثابت wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        اگر (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(درست);
        // Parameters if implicit untyped inherit from what they are assigned to
        ثابت AstBasicDType* ثابت bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        منطقی didchk = نادرست;
        ثابت منطقی implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        اگر (implicitParam) {
            اگر (nodep->valuep()) {
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
                اگر (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } وگرنه اگر (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } وگرنه {
                    صحیح width = 0;
                    Ast_گره_DType* ثابت مقدار_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(مقدار_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ثابت AstBasicDType* ثابت مقدار_Bdtypep = مقدار_DTypep->basicp();
                    منطقی issigned = نادرست;
                    اگر (bdtypep->isNosign()) {
                        اگر (مقدار_Bdtypep && مقدار_Bdtypep->isSigned()) issigned = درست;
                    } وگرنه {
                        issigned = bdtypep->isSigned();
                    }
                    اگر (مقدار_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        اگر (ثابت auto* ثابت constp = VN_CAST(nodep->valuep(), Const)) {
                            اگر (constp->رقم().isString()) {
                                width = constp->رقم().toString().length() * 8;
                            }
                        }
                        اگر (width < 8) width = 8;
                    } وگرنه اگر (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } وگرنه {
                        اگر (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ثابت منطقی is_مقدار_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    اگر (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_مقدار_Fourstate));
                    } وگرنه {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_مقدار_Fourstate);
                    }
                    didchk = درست;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } وگرنه {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } وگرنه اگر (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        اگر (nodep->isNet()) {
            Ast_گره_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            اگر (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        اگر (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(درست);
        nodep->doingWidth(نادرست);
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (nodep->didWidth()) بازگشت;
        اگر (!nodep->varp()) {
            اگر (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, نادرست,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                بازگشت;
            } وگرنه {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        اگر (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        اگر (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            بازگشت;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        اگر (VN_IS(nodep->backp(), گره_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        اگر (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(درست);
        اگر (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } وگرنه اگر (nodep->access().isWriteOrRW() && nodep->varp()->is_درون_داد()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ثابت منطقی hierRef = !m_curModVars.شمار(nodep->varp());
            ثابت منطقی netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            ثابت منطقی contAssign = VN_IS(nodep->abovep(), AssignW);
            ثابت منطقی pinConn = VN_IS(nodep->abovep(), Pin);
            اگر (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } وگرنه اگر (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        اگر (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(درست);
    }

    تهی visit(AstEnumDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_گره_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_گره_DType* ثابت badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        اگر (badDtp) {
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
        V3_عدد رقم(nodep, nodep->width(), 0);
        ثابت V3_عدد one{nodep, nodep->width(), 1};
        منطقی wrapAround = نادرست;
        std::map<ثابت V3_عدد, AstEnumItem*> inits;
        برای (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            اگر (itemp->valuep()) {
                اگر (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_درخت("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                اگر (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_درخت();
                    ادامه;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = نادرست;  // Explicit value resets wrap-around tracking
            }
            اگر (!itemp->valuep()) {
                اگر (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                اگر (رقم.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(جدید AstConst{itemp->fileline(), رقم});
            }

            ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
            اگر (constp->رقم().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            رقم.opAssign(constp->رقم());
            // Look for duplicates
            ثابت auto pair = inits.emplace(رقم, itemp);
            اگر (!pair.second) {  // IEEE says illegal
                ثابت Ast_گره* ثابت otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ثابت منطقی prevNeg = constp->رقم().isNegative();
            رقم.opAdd(one, constp->رقم());
            اگر (basicp->isSigned()) {
                wrapAround = !prevNeg && رقم.isNegative();
            } وگرنه {
                wrapAround = رقم.isEqZero();
            }
        }
    }
    تهی visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_گره_DType* ثابت vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        اگر (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            منطقی warnOn = درست;
            Ast_گره_عبارت* valuep = nodep->valuep();
            اگر (ثابت AstAdd* ثابت anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                اگر (anodep->fileline()->equal_First_سطر_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                اگر (static_cast<صحیح>(constp->رقم().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = نادرست;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(جدید AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    تهی visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        اگر (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_گره* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            برای (; enump; enump = enump->backp()) {
                اگر (VN_IS(enump, EnumDType)) بشکن;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    تهی visit(Ast_گره_Cons* nodep) override {}  // Widthed when created
    تهی visit(Ast_گره_Cons_Item* nodep) override {}  // Widthed when created
    تهی visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        برای (Ast_گره *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_گره_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        اگر (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } وگرنه اگر (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } وگرنه {
            // Take width as maximum across all items
            صحیح width = nodep->exprp()->width();
            صحیح mwidth = nodep->exprp()->widthMin();
            منطقی isFourstate = nodep->exprp()->dtypep()->isFourstate();
            برای (ثابت Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        برای (Ast_گره *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            اگر (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } وگرنه {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        اگر (m_constraintp) {
            منطقی canLower = درست;
            برای (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                اگر (VN_IS(ditemp->rangep(), InsideRange)) ادامه;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ثابت Ast_گره_DType* ثابت itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                اگر (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = نادرست;
                    بشکن;
                }
            }
            اگر (canLower) بازگشت;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_گره_عبارت* newp = nullptr;
        برای (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_گره_عبارت* inewp
                = insideItem(nodep, nodep->exprp()->clone_درخت_Pure(نادرست), itemp->rangep());
            اگر (!inewp) ادامه;
            Ast_گره_عبارت* ثابت cmpp
                = جدید AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            جدید AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, درست);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, درست);
            inewp = جدید AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? جدید AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        اگر (!newp) newp = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        برای (Ast_گره *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_گره_DType* expDTypep = nullptr;

        اگر (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } وگرنه اگر (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } وگرنه {
            // Take width as maximum across all items
            صحیح width = nodep->exprp()->width();
            صحیح mwidth = nodep->exprp()->widthMin();
            منطقی isFourstate = nodep->exprp()->dtypep()->isFourstate();
            برای (ثابت Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
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
        برای (Ast_گره *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            اگر (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } وگرنه اگر (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_گره_عبارت* exprp;
        Ast_عبارت_Stmt* عبارت_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        اگر (!m_constraintp && !nodep->exprp()->isPure()) {
            File_سطر* ثابت fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_گره_DType* ثابت عبارت_Dtp = nodep->exprp()->dtypep();
            ثابت صحیح w = عبارت_Dtp->width();
            Ast_گره_DType* ثابت tempDTypep
                = عبارت_Dtp->widthSized() ? عبارت_Dtp
                                        : nodep->findBitOrLogicDType(w, w, عبارت_Dtp->numeric(),
                                                                     عبارت_Dtp->isFourstate());
            AstVar* ثابت varp
                = جدید AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = جدید AstVarRef{fl, varp, VAccess::READ};
            عبارت_Stmtp = جدید Ast_عبارت_Stmt{fl,
                                        جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            اگر (m_ftaskp) {
                varp->funcLocal(درست);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } وگرنه {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(عبارت_Stmtp);
        } وگرنه {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_گره_عبارت* newp = nullptr;
        برای (Ast_گره_عبارت *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), گره_عبارت);  // Will be unlinking
            Ast_گره_عبارت* inewp;
            اگر (عبارت_Stmtp) {
                inewp = insideItem(nodep, عبارت_Stmtp, itemp);
                عبارت_Stmtp = nullptr;
            } وگرنه {
                inewp = insideItem(nodep, exprp->clone_درخت_Pure(نادرست), itemp);
            }
            اگر (!inewp) ادامه;
            newp = newp ? جدید AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        اگر (عبارت_Stmtp) VL_DO_DANGLING(عبارت_Stmtp->delete_درخت(), عبارت_Stmtp);
        اگر (!newp) newp = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    Ast_گره_عبارت* insideItem(Ast_گره* nodep, Ast_گره_عبارت* exprp, Ast_گره_عبارت* itemp) {
        ثابت Ast_گره_DType* ثابت itemDtp = itemp->dtypep()->skipRefp();
        اگر (AstInsideRange* ثابت irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            بازگشت irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } وگرنه اگر (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_گره_عبارت* ثابت inewp = جدید AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(درست);
            بازگشت inewp;
        } وگرنه اگر (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_درخت(), exprp);
            بازگشت nullptr;
        }
        بازگشت AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    تهی visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        اگر (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } وگرنه {
            اگر (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            اگر (m_vup->final()) {
                Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ثابت صحیح expWidth = expDTypep->width();
                ثابت منطقی waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                ثابت منطقی waiveRhs = expWidth == 32
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

    تهی visit(AstIfaceRefDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        اگر (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    تهی visit(Ast_گره_UOr_Struct_DType* nodep) override {
        اگر (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(نادرست);
            بازگشت;
        }
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(درست);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        اگر (ثابت AstUnionDType* ثابت unionp = VN_CAST(nodep, UnionDType)) {
            اگر (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(نادرست);
        // Error checks
        برای (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گره_DType* ثابت dtp = itemp->subDTypep()->skipRefp();
            اگر (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            اگر ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                افزودن_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        ثابت منطقی isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ثابت منطقی inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        اگر (VN_IS(nodep, UnionDType) || nodep->packed()) {
            صحیح lsb = 0;
            صحیح width = 0;
            منطقی first = درست;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            برای (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            برای (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                اگر (itemp->isFourstate()) nodep->isFourstate(درست);
                itemp->lsb(lsb);
                اگر (VN_IS(nodep, UnionDType)) {
                    ثابت صحیح itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    اگر (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } وگرنه {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = نادرست;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } وگرنه {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(نادرست);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    تهی visit(AstThisRef* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        اگر (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    تهی visit(AstClassRefDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    تهی visit(AstClassOrPackageRef* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        userIterateChildren(nodep, nullptr);
    }
    تهی visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstClassExtends* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        اگر (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    تهی visit(AstMemberDType* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        اگر (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    تهی visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        اگر (nodep->didWidth()) بازگشت;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = درست;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        ثابت AstMemberSel* ثابت fromSel = VN_CAST(nodep->fromp(), MemberSel);
        اگر (AstClocking* ثابت clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            اگر (AstVar* ثابت varp = memberSelClocking(nodep, clockingp)) {
                اگر (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_درخت(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                اگر (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                اگر (AstIfaceRefDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(درست);  // Must not visit again: will confuse scopes
                بازگشت;
            }
        } وگرنه اگر (Ast_گره_UOr_Struct_DType* ثابت adtypep = VN_CAST(fromDtp, گره_UOr_Struct_DType)) {
            اگر (memberSelStruct(nodep, adtypep)) بازگشت;
        } وگرنه اگر (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            اگر (memberSelClass(nodep, adtypep)) بازگشت;
        } وگرنه اگر (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            اگر (Ast_گره* foundp = memberSelIface(nodep, adtypep)) {
                اگر (AstClocking* ثابت clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                اگر (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    اگر (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* ثابت ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ثابت AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
                    اگر (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ثابت newp = جدید AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(درست);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                        بازگشت;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(درست);
                    بازگشت;
                }
                اگر (AstModport* ثابت modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ثابت newDtypep = جدید AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_گره_عبارت* ثابت fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
                اگر (AstCell* ثابت cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    اگر (VN_IS(cellp->modp(), Iface)) {
                        ثابت string viftopName = cellp->name() + "__Viftop";
                        Ast_گره_Module* ثابت parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        برای (Ast_گره* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            اگر (AstVar* ثابت vp = VN_CAST(itemp, Var)) {
                                اگر (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    بشکن;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        اگر (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_گره_DType* subDtypep = viftopVarp->dtypep();
                        اگر (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            اگر (AstIfaceRefDType* ثابت subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ثابت newDtypep = جدید AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(درست);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(درست);
                        بازگشت;
                    }
                }
                اگر (Ast_گره_FTask* ثابت ftaskp = VN_CAST(foundp, گره_FTask)) {
                    AstMethodCall* ثابت newp = جدید AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    بازگشت;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } وگرنه اگر (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_گره* ثابت newp = جدید AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            بازگشت;
        } وگرنه {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    منطقی memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        اگر (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* ثابت newp = جدید AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            visit(newp);
            بازگشت درست;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        برای (AstClass* classp = first_classp; classp;) {
            اگر (Ast_گره* ثابت foundp = m_memberMap.findMember(classp, nodep->name())) {
                اگر (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    اگر (!varp->didWidth()) userIterate(varp, nullptr);
                    اگر (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ثابت varRefp
                            = جدید AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                        بازگشت درست;
                    }
                    اگر (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(درست);
                    اگر (nodep->fromp()->same_درخت(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    بازگشت درست;
                }
                اگر (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(جدید AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت درست;
                }
                اگر (AstEnumItemRef* ثابت adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_درخت(نادرست));
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت درست;
                }
                اگر (Ast_گره_FTask* ftaskp = VN_CAST(foundp, گره_FTask)) {
                    AstMethodCall* newp = جدید AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت درست;
                }
                اگر (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    بازگشت درست;
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
        برای (AstClass* classp = first_classp; classp;) {
            برای (Ast_گره* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                اگر (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.افزودن_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        ثابت string suggest = speller.best_Candidate_پیام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        بازگشت نادرست;  // Caller handles error
    }
    منطقی modportAllowsItem(Ast_گره* nodep, ثابت AstIfaceRefDType* adtypep, ثابت Ast_گره* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ثابت AstModport* ثابت modportp = adtypep->modportp();
        اگر (!modportp) بازگشت درست;
        اگر (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) {
            اگر (varp->isParam()) بازگشت درست;
        } وگرنه اگر (!VN_IS(itemp, گره_FTask) && !VN_IS(itemp, Clocking)) {
            بازگشت درست;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        اگر (pair.second) {
            برای (ثابت Ast_گره* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.درج(mpItemp->name());
            }
        }
        اگر (pair.first->second.شمار(itemp->name())) بازگشت درست;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        بازگشت نادرست;
    }
    Ast_گره* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_گره_Module* ثابت ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        برای (Ast_گره* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            اگر (itemp->name() == nodep->name()) {
                اگر (!modportAllowsItem(nodep, adtypep, itemp)) {
                    بازگشت nullptr;  // Caller handles error
                }
                بازگشت itemp;
            }
            اگر (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.افزودن_Candidate(itemp->prettyName());
            }
        }
        ثابت string suggest = speller.best_Candidate_پیام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        بازگشت nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        برای (Ast_گره* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            اگر (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                اگر (citemp->varp()->name() == nodep->name()) بازگشت citemp->varp();
                speller.افزودن_Candidate(citemp->varp()->prettyName());
            }
        }
        ثابت string suggest = speller.best_Candidate_پیام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        بازگشت nullptr;  // Caller handles error
    }
    منطقی memberSelStruct(AstMemberSel* nodep, Ast_گره_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        اگر (AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            اگر (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } وگرنه اگر (adtypep->packed()) {
                AstSel* ثابت newp = جدید AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(درست);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } وگرنه {
                AstStructSel* ثابت newp = جدید AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(درست);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            بازگشت درست;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        بازگشت نادرست;
    }

    تهی visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    تهی visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        اگر (nodep->didWidth()) بازگشت;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* ثابت basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        اگر (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } وگرنه اگر (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } وگرنه اگر (AstEnumDType* ثابت adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } وگرنه اگر (AstAssocArrayDType* ثابت adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } وگرنه اگر (AstWildcardArrayDType* ثابت adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } وگرنه اگر (AstDynArrayDType* ثابت adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } وگرنه اگر (AstQueueDType* ثابت adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } وگرنه اگر (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } وگرنه اگر (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } وگرنه اگر (AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } وگرنه اگر (AstConstraintRefDType* ثابت adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } وگرنه اگر (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } وگرنه اگر (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } وگرنه {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_گره_FTask_Ref* nodep, منطقی required, منطقی arbReturn,
                              Ast_گره_DType* returnDtp, Ast_گره_DType* indexDtp,
                              Ast_گره_DType* مقدار_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        اگر (AstWith* ثابت withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->مقدار_Arg_Refp()->dtypep(مقدار_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            بازگشت withp;
        }
        اگر (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        بازگشت nullptr;
    }
    تهی methodOkArguments(Ast_گره_FTask_Ref* nodep, صحیح minArg, صحیح maxArg) {
        صحیح narg = 0;
        اگر (AstWith* ثابت withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(افزودن_Deletep(withp->unlinkFrBack()), withp);
        }
        برای (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ثابت منطقی ok = (narg >= minArg) && (narg <= maxArg);
        اگر (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            برای (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    جدید AstArg{nodep->fileline(), "",
                               جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            برای (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                وقتی (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_درخت(), argp);
            }
        }
    }

    Ast_گره_عبارت* methodArg(AstMethodCall* nodep, صحیح arg) {
        AstArg* argp = nodep->argsp();
        برای (صحیح narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        بازگشت argp->exprp();
    }

    تهی methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        اگر (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            اگر (nodep->name() == "num") {
                صحیح اقلام = 0;
                برای (Ast_گره* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++اقلام;
                newp = جدید AstConst(nodep->fileline(), AstConst::Signed32{}, اقلام);
            } وگرنه اگر (nodep->name() == "first") {
                ثابت AstEnumItem* itemp = adtypep->itemsp();
                اگر (!itemp) {
                    newp = جدید AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } وگرنه {
                    newp = VN_AS(itemp->valuep()->clone_درخت(نادرست), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } وگرنه اگر (nodep->name() == "last") {
                ثابت AstEnumItem* itemp = adtypep->itemsp();
                وقتی (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                اگر (!itemp) {
                    newp = جدید AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } وگرنه {
                    newp = VN_AS(itemp->valuep()->clone_درخت(نادرست), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            اگر (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } وگرنه اگر (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } وگرنه اگر (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } وگرنه {
                nodep->v3fatalSrc("Bad case");
            }

            اگر (nodep->name() != "name" && nodep->argsp()) {
                Ast_گره_عبارت* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, درست);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                اگر (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_گره_عبارت* ثابت newp = جدید AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ثابت uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                اگر (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* ثابت newp = جدید AstMethodCall{
                        nodep->fileline(),
                        جدید AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    بازگشت;
                } وگرنه اگر (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    افزودن_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* ثابت clonep = nodep->clone_درخت(نادرست);
                    VN_AS(stepp, Const)->رقم().setLong(1);
                    AstConst* ثابت constp = جدید AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* ثابت argp = جدید AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* ثابت newp
                        = جدید AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    بازگشت;
                }
            }
            Ast_گره_عبارت* ثابت newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    تهی methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        اگر (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } وگرنه اگر (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } وگرنه اگر (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_گره_عبارت* ثابت index_exprp = method_Call_Wildcard_Index_عبارت(nodep, adtypep);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } وگرنه اگر (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            اگر (!nodep->argsp()) {
                newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } وگرنه {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_گره_عبارت* ثابت index_exprp = method_Call_Wildcard_Index_عبارت(nodep, adtypep);
                newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } وگرنه اگر (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } وگرنه اگر (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = methodWithClause(nodep, نادرست, نادرست, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp
                = methodWithClause(nodep, درست, نادرست, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "map") {
            AstWith* ثابت withp
                = methodWithClause(nodep, درست, نادرست, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        اگر (newp) {
            newp->protect(نادرست);
            newp->didWidth(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    تهی methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        اگر (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        اگر (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } وگرنه اگر (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->کلید_DTypep(), BOTH);
            Ast_گره_عبارت* ثابت index_exprp = method_Call_Assoc_Index_عبارت(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->کلید_DTypep(), BOTH);
            Ast_گره_عبارت* ثابت index_exprp = method_Call_Assoc_Index_عبارت(nodep, adtypep);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } وگرنه اگر (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            اگر (!nodep->argsp()) {
                newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } وگرنه {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->کلید_DTypep(),
                                  BOTH);
                Ast_گره_عبارت* ثابت index_exprp = method_Call_Assoc_Index_عبارت(nodep, adtypep);
                newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } وگرنه اگر (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } وگرنه اگر (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp = methodWithClause(nodep, نادرست, نادرست, adtypep->subDTypep(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = methodWithClause(
                nodep, نادرست, درست, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            اگر (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->کلید_DTypep()));
            } وگرنه {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp = methodWithClause(nodep, درست, نادرست, nodep->findBitDType(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp = methodWithClause(nodep, درست, نادرست, nodep->findBitDType(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->کلید_DTypep()));
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "map") {
            AstWith* ثابت withp = methodWithClause(nodep, درست, نادرست, adtypep->subDTypep(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        اگر (newp) {
            newp->protect(نادرست);
            newp->didWidth(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    Ast_گره_عبارت* method_Call_Assoc_Index_عبارت(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_گره* ثابت index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->کلید_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        بازگشت nodep->argsp()->exprp();
    }
    Ast_گره_عبارت* method_Call_Wildcard_Index_عبارت(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_گره* ثابت index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        بازگشت nodep->argsp()->exprp();
    }
    تهی methodCallLValueRecurse(AstMethodCall* nodep, Ast_گره* childp, ثابت VAccess& access) {
        اگر (AstCMethodHard* ثابت ichildp = VN_CAST(childp, CMethodHard)) {
            منطقی methAt = نادرست;
            منطقی methAtBack = نادرست;
            منطقی methAtWrite = نادرست;
            انتخاب (ichildp->method()) {
            حالت VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = درست;
                بشکن;
            حالت VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = درست;
                بشکن;
            حالت VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            حالت VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            حالت VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = درست;
                بشکن;
            پیشفرض: بشکن;
            }
            اگر (methAt || methAtBack || methAtWrite) {
                ثابت Ast_گره_DType* ثابت fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                اگر (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    اگر (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } وگرنه اگر (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                بازگشت;
            }
        }
        اگر (Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(childp, گره_Var_Ref)) {
            varrefp->access(access);
        } وگرنه اگر (ثابت AstMemberSel* ثابت ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } وگرنه اگر (ثابت AstStructSel* ثابت ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } وگرنه اگر (ثابت Ast_گره_Sel* ثابت ichildp = VN_CAST(childp, گره_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } وگرنه اگر (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } وگرنه {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_گره_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        اگر (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            اگر (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, نادرست, درست, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = methodWithClause(
                nodep, نادرست, درست, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            اگر (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } وگرنه {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ثابت withp
                = methodWithClause(nodep, درست, نادرست, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp
                = methodWithClause(nodep, درست, نادرست, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ثابت withp
                = methodWithClause(nodep, درست, نادرست, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        بازگشت newp;
    }
    تهی methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        اگر (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } وگرنه اگر (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } وگرنه اگر (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } وگرنه اگر (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = methodWithClause(nodep, نادرست, نادرست, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر ((newp = methodCallArray(nodep, adtypep))) {
        } وگرنه {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        اگر (newp) {
            newp->protect(نادرست);
            newp->didWidth(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    تهی methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        اگر (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وگرنه اگر (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وگرنه اگر (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } وگرنه اگر (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            اگر (!nodep->argsp()) {
                newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } وگرنه {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_گره_عبارت* ثابت index_exprp = method_Call_Queue_Index_عبارت(nodep);
                اگر (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_برداشت_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } وگرنه {
                    newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } وگرنه اگر (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_گره_عبارت* ثابت index_exprp = method_Call_Queue_Index_عبارت(nodep);
            AstArg* ثابت argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            اگر (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_افزودن_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } وگرنه {
                newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_درج, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } وگرنه اگر (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وگرنه اگر (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ثابت argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } وگرنه اگر (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ثابت withp
                = methodWithClause(nodep, نادرست, نادرست, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } وگرنه اگر ((newp = methodCallArray(nodep, adtypep))) {
        } وگرنه {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        اگر (newp) {
            newp->protect(نادرست);
            newp->didWidth(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    Ast_گره_عبارت* method_Call_Queue_Index_عبارت(AstMethodCall* nodep) {
        Ast_گره* ثابت index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        بازگشت nodep->argsp()->exprp();
    }
    تهی methodCallWarnTiming(Ast_گره_FTask_Ref* ثابت nodep, ثابت std::string& className) {
        اگر (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } وگرنه اگر (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    تهی methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ثابت ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        اگر (Ast_گره_FTask* ثابت ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), گره_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            اگر (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                بازگشت;
            }
            اگر (adtypep->isVirtual()) {
                برای (Ast_گره* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    اگر (AstVar* ثابت mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            اگر (ftaskp->isStatic()) {
                AstArg* ثابت argsp = nodep->argsp();
                اگر (argsp) argsp->unlinkFrBackWithNext();
                Ast_گره_FTask_Ref* newp = nullptr;
                اگر (VN_IS(ftaskp, Task)) {
                    newp = جدید AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } وگرنه {
                    newp = جدید AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            } وگرنه {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                اگر (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            بازگشت;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    تهی handleRandomizeArgs(Ast_گره_FTask_Ref* ثابت nodep, AstClass* ثابت classp) {
        منطقی hasNonNullArgs = نادرست;
        AstConst* nullp = nullptr;
        برای (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_گره_عبارت* exprp = argp->exprp();
            اگر (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                اگر (constp->رقم().isNull()) {
                    nullp = constp;
                    ادامه;
                }
            }
            hasNonNullArgs = درست;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            اگر (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_گره_عبارت* fromp = methodCallp->fromp();
                وقتی (AstMemberSel* ثابت memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ثابت varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            اگر (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_درخت(), argp);
                ادامه;
            }
            وقتی (exprp) {
                اگر (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } وگرنه {
                    اگر (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } وگرنه {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_درخت(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                اگر (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            اگر (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } وگرنه اگر (classp->existsMember([&](ثابت AstClass*, ثابت AstVar* memberVarp) {
                           بازگشت memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } وگرنه {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_درخت(), argp);
            }
        }
        اگر (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    تهی methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        اگر (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, نادرست, نادرست, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            برای (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                اگر (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } وگرنه اگر (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        برای (AstClass* classp = first_classp; classp;) {
            اگر (nodep->fileline()->timingOn()) {
                اگر (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ثابت packagep = getItemPackage(classp);
                    // Check if it's std
                    اگر (packagep && packagep->name() == "std") {
                        اگر (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } وگرنه اگر (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } وگرنه اگر (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            اگر (Ast_گره_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), گره_FTask)) {
                userIterate(ftaskp, nullptr);
                اگر (ftaskp->isStatic()) {
                    AstArg* ثابت argsp = nodep->argsp();
                    اگر (argsp) argsp->unlinkFrBackWithNext();
                    Ast_گره_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    اگر (!m_vup) {  // Called as task
                        newp = جدید AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } وگرنه {
                        newp = جدید AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                } وگرنه {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    اگر (VN_IS(ftaskp, Task)) {
                        اگر (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    اگر (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                بازگشت;
            } وگرنه اگر (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(درست);
                v3Global.useRandomizeMethods(درست);
                AstCMethodHard* ثابت newp
                    = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(درست);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            } وگرنه اگر (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_گره_عبارت* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_گره_عبارت* ثابت exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(درست);
                v3Global.useRandomizeMethods(درست);
                AstCMethodHard* ثابت newp
                    = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(درست);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            } وگرنه اگر (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(درست);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            برای (AstClass* classp = first_classp; classp;) {
                برای (Ast_گره* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    اگر (VN_IS(itemp, گره_FTask)) speller.افزودن_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            ثابت string suggest = speller.best_Candidate_پیام(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    تهی methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        اگر (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            اگر (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } وگرنه {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(درست);
        } وگرنه {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        اگر (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } وگرنه {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(درست);
    }
    تهی methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        شمارش : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        اگر (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } وگرنه اگر (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } وگرنه اگر (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } وگرنه اگر (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } وگرنه اگر (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        اگر (methodId) {
            AstWith* ثابت withp
                = methodWithClause(nodep, نادرست, نادرست, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            اگر (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ثابت newp
                    = جدید AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                اگر (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(نادرست);
                newp->didWidth(درست);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                بازگشت;
            } وگرنه {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_سطر* ثابت fl = nodep->fileline();
                Ast_گره_عبارت* newp = nullptr;
                برای (صحیح i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_گره_عبارت* ثابت arrayRef = nodep->fromp()->clone_درخت_Pure(نادرست);
                    Ast_گره_عبارت* ثابت selector = جدید AstArraySel{fl, arrayRef, i};
                    اگر (!newp) {
                        newp = selector;
                    } وگرنه {
                        انتخاب (methodId) {
                        حالت ARRAY_OR: newp = جدید AstOr{fl, newp, selector}; بشکن;
                        حالت ARRAY_AND: newp = جدید AstAnd{fl, newp, selector}; بشکن;
                        حالت ARRAY_XOR: newp = جدید AstXor{fl, newp, selector}; بشکن;
                        حالت ARRAY_SUM: newp = جدید AstAdd{fl, newp, selector}; بشکن;
                        حالت ARRAY_PRODUCT: newp = جدید AstMul{fl, newp, selector}; بشکن;
                        پیشفرض: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            }
        } وگرنه اگر (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(نادرست);
            newp->didWidth(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    تهی methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        اگر (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* ثابت callp = جدید AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    تهی methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        اگر (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_گره* ثابت newp = جدید AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } وگرنه اگر (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } وگرنه اگر (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } وگرنه اگر (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } وگرنه اگر (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } وگرنه اگر (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_گره* ثابت newp
                = جدید AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه اگر (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_گره* ثابت newp
                = جدید AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه اگر (nodep->name() == "compare" || nodep->name() == "icompare") {
            ثابت منطقی ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ثابت argp = nodep->argsp();
            Ast_گره_عبارت* ثابت lhs = nodep->fromp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت rhs = argp->exprp()->unlinkFrBack();
            Ast_گره* ثابت newp = جدید AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه اگر (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_گره_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_گره_عبارت* ثابت rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* ثابت varrefp
                = جدید AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_گره* ثابت newp = جدید AstAssign{
                nodep->fileline(), fromp, جدید AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            افزودن_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } وگرنه اگر (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            Ast_گره_عبارت* ثابت lhsp = nodep->fromp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت newp = جدید AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه اگر (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_گره_عبارت* ثابت lhsp = nodep->fromp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت thsp = arg1p->exprp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت newp = جدید AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه اگر (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            اگر (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } وگرنه اگر (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } وگرنه اگر (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } وگرنه اگر (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } وگرنه اگر (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } وگرنه {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_گره* ثابت newp
                = جدید AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_گره_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        اگر (AstQueueDType* ثابت queuep = m_queueDTypeIndexed[indexDTypep]) {
            بازگشت queuep;
        } وگرنه {
            AstQueueDType* ثابت newp
                = جدید AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            بازگشت newp;
        }
    }

    تهی visit(AstNew* nodep) override {
        اگر (nodep->didWidth()) بازگشت;
        AstClass* classp = nullptr;
        منطقی assign = نادرست;
        اگر (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = درست;
            Ast_گره* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            اگر (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                اگر (AstClass* ثابت clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ثابت adtypep
                        = جدید AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } وگرنه {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            اگر (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, گره_DType)
                           ? ("data type "s + VN_AS(warnp, گره_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                بازگشت;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            اگر (Ast_گره_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } وگرنه {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } وگرنه {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        اگر (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    تهی visit(AstNewCopy* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        AstClassRefDType* ثابت refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        اگر (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            بازگشت;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        اگر (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    تهی visit(AstNewDynamic* nodep) override {
        اگر (nodep->didWidthAndSet()) بازگشت;
        AstDynArrayDType* ثابت adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        اگر (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            بازگشت;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        اگر (!VN_IS(nodep->backp(), گره_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            بازگشت;
        }
        nodep->dtypep(adtypep);
        اگر (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        اگر (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ثابت consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            اگر (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }

    تهی visit(Ast_Tagged_عبارت* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    تهی visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    تهی visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    تهی visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    تهی visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    تهی visit(AstPattern* nodep) override {
        اگر (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            برای (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = نادرست;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                اگر (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), گره_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                برای (Ast_گره *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            بازگشت;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = نادرست;
        اگر (nodep->didWidthAndSet()) بازگشت;
        UINFO(9, "PATTERN " << nodep);
        اگر (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        اگر (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_گره_DType* dtypep = nodep->dtypep();
        اگر (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            اگر (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                بازگشت;
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            اگر (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                بازگشت;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            برای (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ثابت صحیح times = visitPatMemberRep(patp);
                برای (صحیح i = 1; i < times; i++) {
                    AstPatMember* ثابت newp = patp->clone_درخت(نادرست);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            برای (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                اگر (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    وقتی (Ast_گره_عبارت* ثابت movep = VN_AS(patp->lhssp()->nextp(), گره_عبارت)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_گره* newkeyp = nullptr;
                        اگر (patp->keyp()) newkeyp = patp->keyp()->clone_درخت(درست);
                        AstPatMember* ثابت newp
                            = جدید AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            برای (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ثابت nextp = VN_AS(patp->nextp(), PatMember);
                اگر (patp->isDefault()) {
                    اگر (defaultp) {
                        VL_DO_DANGLING(افزودن_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            وقتی (ثابت AstConstDType* ثابت vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            اگر (auto* ثابت vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } وگرنه اگر (auto* ثابت vdtypep = VN_CAST(dtypep, گره_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } وگرنه اگر (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ثابت consp = جدید AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } وگرنه اگر (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ثابت consp = جدید AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } وگرنه اگر (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } وگرنه اگر (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } وگرنه {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            اگر (defaultp) VL_DO_DANGLING(افزودن_Deletep(defaultp), defaultp);
        }
    }
    تهی patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
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
            وقتی (patp) {
                انجامبده {
                    اگر (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        اگر (ثابت AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            اگر (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                بشکن;
                            } وگرنه {
                                ثابت std::pair<PatMap::iterator, منطقی> ret
                                    = patmap.emplace(memp, patp);
                                اگر (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } وگرنه اگر (Ast_گره_DType* nodedtypep = VN_CAST(patp->keyp(), گره_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            ثابت string dtype = nodedtypep->dtypep()->prettyDTypeName(درست);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } وگرنه {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } وگرنه اگر (memp) {
                        // constant expr
                        ثابت std::pair<PatMap::iterator, منطقی> ret = patmap.emplace(memp, patp);
                        اگر (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } وگرنه {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } وقتی (نادرست);

                // Next
                اگر (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_گره_عبارت* newp = nullptr;
        اگر (vdtypep->packed()) {
            برای (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت auto it = patmap.find(memp);
                اگر (it == patmap.end()) {  // default or default_type assignment
                    اگر (AstStructDType* ثابت memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } وگرنه {
                        AstPatMember* ثابت patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = مقدار_Concat_pattern_Struct(patp, newp, memp, nodep);
                        اگر (patp) VL_DO_DANGLING(افزودن_Deletep(patp), patp);
                    }
                } وگرنه {  // member assignment
                    newp = مقدار_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } وگرنه {  // Unpacked
            AstConsStruct* ثابت consp = جدید AstConsStruct{nodep->fileline(), vdtypep};
            برای (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                اگر (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    افزودن_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    افزودن_Deletep(patp);  // patp used below
                } وگرنه {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_گره_عبارت* ثابت valuep = pattern_Member_مقدار_Iterate(patp);
                consp->addItemsp(جدید AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        اگر (newp) {
            nodep->replaceWith(newp);
        } وگرنه {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_گره_عبارت* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_گره_عبارت* newp,
                                                 AstPattern* nodep, ثابت DTypeMap& dtypemap) {
        برای (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            اگر (AstStructDType* ثابت memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } وگرنه {
                AstPatMember* ثابت patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = مقدار_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                اگر (patp) VL_DO_DANGLING(افزودن_Deletep(patp), patp);
            }
        }
        بازگشت newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ثابت DTypeMap& dtypemap) {
        ثابت string memp_DType = memp->virtRefDTypep()->prettyDTypeName(درست);
        ثابت auto it = dtypemap.find(memp_DType);
        اگر (it != dtypemap.end()) {
            // default_value for data_type
            بازگشت it->second->clone_درخت(نادرست);
        }
        اگر (defaultp) {
            // default_value for any unmatched member yet
            بازگشت defaultp->clone_درخت(نادرست);
        }
        اگر (memp->valuep()) {
            بازگشت جدید AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_درخت(نادرست), گره_عبارت),
                                    جدید AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        بازگشت nullptr;
    }

    Ast_گره_عبارت* مقدار_Concat_pattern_Struct(AstPatMember* patp, Ast_گره_عبارت* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        اگر (patp) {
            patp->dtypep(memp);
            Ast_گره_عبارت* ثابت valuep = pattern_Member_مقدار_Iterate(patp);
            اگر (!newp) {
                newp = valuep;
            } وگرنه {
                AstConcat* ثابت concatp = جدید AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, درست);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, درست);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        بازگشت newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_گره_DType* elemDTypep,
                                       ثابت DTypeMap& dtypemap = DTypeMap{}) {
        Ast_گره_عبارت* ثابت valuep = defaultp->lhssp()->clone_درخت(نادرست);
        Ast_گره_DType* ثابت elemDTypeSkipRefp = elemDTypep->skipRefp();
        ثابت AstStructDType* ثابت structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ثابت منطقی unpackedStruct = structp && !structp->packed();

        اگر (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            بازگشت defaultp->clone_درخت(نادرست);
        }
        اگر (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            بازگشت defaultp->clone_درخت(نادرست);
        }
        اگر (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        منطقی wholeElement = unpackedStruct;
        اگر (Ast_گره_DType* ثابت مقدار_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !مقدار_DTypep->skipRefp()->isIntegralOrPacked()
                                          : مقدار_DTypep->similarDType(elemDTypeSkipRefp);
        }
        اگر (wholeElement) {
            VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            بازگشت defaultp->clone_درخت(نادرست);
        }

        AstPatMember* ثابت nestedDefaultp
            = جدید AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(درست);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        برای (ثابت auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_درخت(نادرست));
        AstPattern* ثابت recursivePatternp = جدید AstPattern{defaultp->fileline(), nestedDefaultp};
        بازگشت جدید AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_گره_عبارت* patternDefaultValuep(AstPatMember* defaultp, Ast_گره_DType* elemDTypep) {
        اگر (!defaultp) بازگشت nullptr;
        AstPatMember* ثابت elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_گره_عبارت* ثابت valuep = pattern_Member_مقدار_Iterate(elemDefaultp);
        VL_DO_DANGLING(افزودن_Deletep(elemDefaultp), elemDefaultp);
        بازگشت valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    تهی patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        ثابت VNumRange range = arrayDtp->declRange();
        ثابت صحیح elements = range.elements();
        AstConsArray* ثابت newp = جدید AstConsArray{nodep->fileline(), arrayDtp};
        صحیح entn = 0;  // Number of elements given so far, from the left
        منطقی error = نادرست;  // Reported an error, so the value is not complete
        برای (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_گره_عبارت* ثابت valuep = pattern_Member_مقدار_Iterate(patp);
            ثابت uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            ثابت AstUnpackArrayDType* ثابت spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            اگر (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_مقادیر(index, valuep);
                ادامه;
            }
            ثابت VCastable castable = Ast_گره::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            اگر (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = درست;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        اگر (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = درست;
        }
        اگر (error) {
            VL_DO_DANGLING(افزودن_Deletep(newp), newp);
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } وگرنه {
            nodep->replaceWith(newp);
        }
    }
    تهی patternArray(AstPattern* nodep, Ast_گره_Array_DType* arrayDtp, AstPatMember* defaultp) {
        اگر (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            بازگشت;
        }
        ثابت VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.چپ() << " to " << range.راست());
        Ast_گره* newp = nullptr;
        منطقی missed = نادرست;  // Some element missed, so not a complete value
        برای (صحیح entn = 0, ent = range.چپ(); entn < range.elements();
             ++entn, ent += range.چپ_To_راست_Inc()) {
            Ast_گره_عبارت* valuep = nullptr;
            ثابت auto it = patmap.find(ent);
            اگر (it == patmap.end()) {
                اگر (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } وگرنه {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = درست;
                }
            } وگرنه {
                AstPatMember* ثابت patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_مقدار_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ثابت AstUnpackArrayDType* ثابت memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                اگر (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
                    valuep = جدید AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            اگر (valuep) {
                اگر (VN_IS(arrayDtp, UnpackArrayDType)) {
                    اگر (!newp) newp = جدید AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } وگرنه {  // Packed. Convert to concat for now.
                    اگر (!newp) {
                        newp = valuep;
                    } وگرنه {
                        AstConcat* ثابت concatp
                            = جدید AstConcat{valuep->fileline(), VN_AS(newp, گره_عبارت), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        اگر (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        اگر (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(افزودن_Deletep(newp), newp);
            newp = جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        اگر (newp) {
            nodep->replaceWith(newp);
        } وگرنه {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        اگر (VN_IS(patp->keyp(), گره_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            بازگشت nullptr;
        }
        اگر (patp->کلید_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->کلید_Exprp());  // May change
        AstConst* ثابت constp = VN_CAST(patp->کلید_Exprp(), Const);
        اگر (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        بازگشت constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_گره_DType* کلید_DTypep) {
        اگر (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            بازگشت nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        اگر (کلید_DTypep->skipRefp()->isIntegralOrPacked() && patp->کلید_Exprp()) {
            userIterate(patp->کلید_Exprp(), WidthVP{کلید_DTypep, PRELIM}.p());
            ثابت صحیح width = std::max(patp->کلید_Exprp()->width(), کلید_DTypep->width());
            Ast_گره_DType* ثابت contextDTypep
                = کلید_DTypep->findBitOrLogicDType(width, width, کلید_DTypep->numeric(), درست);
            iterateCheck(patp, "key", patp->کلید_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, نادرست);
        }
        AstConst* ثابت constp = patternConstKeyp(patp);
        اگر (!constp) بازگشت nullptr;
        اگر (constp->رقم().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            بازگشت nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, کلید_DTypep, EXTEND_EXP, نادرست);
        Ast_گره_عبارت* ثابت convp = patp->کلید_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_گره_عبارت* ثابت keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        بازگشت VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_گره_عبارت* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_گره_DType* arrayDtp) {
        Ast_گره_DType* ثابت کلید_DTypep = VN_AS(arrayDtp, AssocArrayDType)->کلید_DTypep();
        اگر (!کلید_DTypep->skipRefp()->isAggregateType() || !patp->کلید_Exprp()) {
            بازگشت patternScalarKeyp(patp, کلید_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->کلید_Exprp(), کلید_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->کلید_Exprp());
        Ast_گره_عبارت* ثابت foldedp = patp->کلید_Exprp();
        اگر (!foldedp->is_Constant_درخت()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            بازگشت nullptr;
        }
        اگر (foldedp->exists([](ثابت AstConst* constp) { بازگشت constp->رقم().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            بازگشت nullptr;
        }
        بازگشت foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_گره_DType* arrayDtp) {
        بازگشت patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_گره_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        بازگشت patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    تهی sort_Cons_اقلام(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        وقتی (T_Item* ثابت itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.افزودن_back(itemp);
        }
        ثابت auto کلید_Of = [](ثابت T_Item* itemp) -> ثابت V3_عدد& {
            ثابت Ast_گره* ثابت keyp = itemp->keyp();
            بازگشت VN_AS(keyp, Const)->رقم();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](ثابت T_Item* ap, ثابت T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            بازگشت کلید_Of(ap).isLtXZ(کلید_Of(bp));
        });
        ثابت T_Item* firstp = nullptr;  // First item with the current key
        برای (T_Item* ثابت itemp : itemps) {
            اگر (firstp && کلید_Of(firstp).isCaseEq(کلید_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } وگرنه {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    تهی patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_گره_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        برای (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ثابت auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_گره_عبارت* ثابت valuep = pattern_Member_مقدار_Iterate(patp);
            اگر (keyp) {
                consp->addItemsp(جدید T_Item{patp->fileline(), keyp, valuep});
            } وگرنه {
                VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ثابت AstAssocArrayDType* ثابت assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        اگر (!assocDtp || !assocDtp->کلید_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_اقلام<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_گره_عبارت* newConsDynArrayOrQueue(File_سطر* fileline, ثابت Ast_گره_DType* dtypep) {
        اگر (VN_IS(dtypep, DynArrayDType)) بازگشت جدید AstConsDynArray{fileline};
        اگر (VN_IS(dtypep, QueueDType)) بازگشت جدید AstConsQueue{fileline};
        بازگشت nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    تهی addConsItem(T_Cons* consp, Ast_گره_عبارت* valuep) {
        Ast_گره_DType* ثابت arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ثابت منطقی isElement
            = Ast_گره::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ثابت AstQueueDType* ثابت subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        اگر (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* ثابت subp = Ast_گره::cast<T_Cons>(valuep);
            اگر (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                اگر (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(افزودن_Deletep(subp), subp);
                بازگشت;
            }
            اگر (AstConsQueue* ثابت queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_اقلام(consp, queuep);
                بازگشت;
            }
            اگر (AstConsDynArray* ثابت dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_اقلام(consp, dynp);
                بازگشت;
            }
        }
        T_Item* ثابت itemp = جدید T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        اگر (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } وگرنه {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    تهی addConsItem(Ast_گره_عبارت* consp, Ast_گره_عبارت* valuep) {
        اگر (AstConsDynArray* ثابت dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } وگرنه {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    تهی add_Cons_اقلام(Ast_گره_عبارت* consp, T_Sub* subp) {
        وقتی (auto* ثابت itemp = subp->itemsp()) {
            Ast_گره_عبارت* ثابت valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(افزودن_Deletep(subp), subp);
    }
    تهی patternDynArrayOrQueue(AstPattern* nodep, Ast_گره_DType* arrayp, AstPatMember* defaultp) {
        Ast_گره_عبارت* ثابت newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ثابت AstPatMember* ثابت firstp = VN_AS(nodep->itemsp(), PatMember);
        ثابت منطقی keyed = defaultp || (firstp && firstp->keyp());
        اگر (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            بازگشت;
        }
        ثابت نویسه* ثابت kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        اگر (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        برای (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            اگر (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_مقدار_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    تهی patternBasic(AstPattern* nodep, Ast_گره_DType* vdtypep, AstPatMember* defaultp) {
        ثابت AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ثابت VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_گره_عبارت* newp = nullptr;
        برای (ثابت صحیح ent : range.seqHiToLo()) {
            Ast_گره_عبارت* valuep = nullptr;
            ثابت auto it = patmap.find(ent);
            اگر (it == patmap.end()) {
                اگر (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } وگرنه {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } وگرنه {
                AstPatMember* ثابت patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_مقدار_Iterate(patp);
            }
            اگر (valuep) {  // Packed. Convert to concat for now.
                اگر (!newp) {
                    newp = valuep;
                } وگرنه {
                    AstConcat* ثابت concatp = جدید AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        اگر (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        اگر (newp) {
            nodep->replaceWith(newp);
        } وگرنه {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_گره_عبارت* pattern_Member_مقدار_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_گره_عبارت* valuep = patp->lhssp()->unlinkFrBack();
        اگر (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            اگر (AstConst* ثابت newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        بازگشت valuep;
    }

    ایستا تهی checkEventAssignment(ثابت Ast_گره_Assign* ثابت asgnp) {
        string unsupEvtAsgn;
        اگر (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        اگر (VN_IS(asgnp->rhsp(), CReset)) بازگشت;
        اگر (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        اگر (!unsupEvtAsgn.empty()) {
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

    ایستا منطقی usesDynamicScheduler(Ast_گره* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        وقتی (درست) {
            AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef);
            اگر (vrefp) بازگشت usesDynamicScheduler(vrefp);
            اگر (VN_IS(nodep, MemberSel)) {
                بازگشت درست;
            } وگرنه اگر (Ast_گره_Sel* selp = VN_CAST(nodep, گره_Sel)) {
                nodep = selp->fromp();
            } وگرنه {
                بازگشت نادرست;
            }
        }
    }

    ایستا منطقی usesDynamicScheduler(AstVarRef* vrefp) {
        بازگشت VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    تهی visit(AstPatMember* nodep) override {
        Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    صحیح visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        اگر (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ثابت AstConst* ثابت constp = VN_CAST(nodep->repp(), Const);
            اگر (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } وگرنه {
                times = constp->toUInt();
            }
            اگر (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_درخت();  // Done with replicate before cloning
        }
        بازگشت times;
    }

    تهی visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_گره* propp = nodep->propp();
            وقتی (AstLogNot* ثابت notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ثابت untilp = VN_CAST(propp, Until);
            اگر (untilp && VN_IS(nodep->backp(), گره_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(جدید AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(افزودن_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            اگر (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    منطقی firstNewStatementOkRecurse(Ast_گره* nodep) {
        اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            اگر (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) بازگشت درست;
        }
        اگر (AstAssign* ثابت aitemp = VN_CAST(nodep, Assign)) {
            اگر (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) بازگشت درست;
        }
        اگر (AstInitialStaticStmt* ثابت aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            برای (Ast_گره* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (!firstNewStatementOkRecurse(stmtp)) بازگشت نادرست;
            }
            بازگشت درست;
        }
        اگر (AstInitialAutomaticStmt* ثابت aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            برای (Ast_گره* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (!firstNewStatementOkRecurse(stmtp)) بازگشت نادرست;
            }
            بازگشت درست;
        }
        بازگشت نادرست;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    تهی handleCaseType(Ast_گره* casep, Ast_گره_عبارت* exprp, CaseItem* itemsp) {
        AstAttrOf* ثابت exprap = VN_CAST(exprp, AttrOf);
        اگر (!exprap) بازگشت;
        اگر (exprap->attrType() != VAttrType::TYPEID) بازگشت;

        ثابت Ast_گره_DType* ثابت عبارت_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << عبارت_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_سطر* ثابت newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, درست);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, درست);  // Side effect of transform
        casep->fileline(newfl);
        برای (CaseItem* itemp = itemsp; itemp; itemp = Ast_گره::as<CaseItem>(itemp->nextp())) {
            اگر (itemp->isDefault()) ادامه;
            منطقی hit = نادرست;
            برای (Ast_گره* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ثابت AstAttrOf* ثابت condAttrp = VN_CAST(condp, AttrOf);
                اگر (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } وگرنه {
                    Ast_گره_DType* ثابت condDtp = condAttrp->dtypep();
                    اگر (Ast_گره::computeCastable(عبارت_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = درست;
                        بشکن;
                    }
                }
            }
            افزودن_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(جدید AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(جدید AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(افزودن_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    تهی handleCase(Ast_گره* casep, Ast_گره_عبارت* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_گره_DType* subDTypep = exprp->dtypep();
        برای (CaseItem* itemp = itemsp; itemp; itemp = Ast_گره::as<CaseItem>(itemp->nextp())) {
            برای (Ast_گره* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                اگر (condp->dtypep() == subDTypep) ادامه;

                اگر (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } وگرنه اگر (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } وگرنه {
                    ثابت صحیح width = std::max(subDTypep->width(), condp->width());
                    ثابت صحیح mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    ثابت منطقی issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        برای (CaseItem* itemp = itemsp; itemp; itemp = Ast_گره::as<CaseItem>(itemp->nextp())) {
            برای (Ast_گره *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    تهی visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        برای (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            برای (Ast_گره *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    تهی visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    تهی visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    تهی visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        برای (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            برای (Ast_گره *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    تهی visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_گره_DType* ثابت itemDTypep = nodep->findUInt32DType();
        برای (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            برای (Ast_گره *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    تهی visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    تهی visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    تهی visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    تهی visit(Ast_گره_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    تهی visit(Ast_عبارت_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    تهی visit(Ast_گره_Foreach* nodep) override {
        اگر (nodep->didWidth()) بازگشت;
        nodep->didWidth(درست);
        ثابت AstForeachHeader* ثابت headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_گره_عبارت* ثابت fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_گره_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        برای (Ast_گره *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            ثابت منطقی empty = VN_IS(argsp, Empty);
            AstVar* ثابت varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            اگر (varp) varp->usedLoopIdx(درست);
            اگر (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
                بازگشت;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            اگر (VN_IS(fromDtp, گره_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } وگرنه اگر (AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                اگر (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
                    بازگشت;
                }
            } وگرنه اگر (ثابت AstAssocArrayDType* ثابت adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->کلید_DTypep());
            } وگرنه {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
                بازگشت;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_گره* ثابت bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        اگر (AstForeach* ثابت loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            بازگشت;
        }
    }

    تهی visit(Ast_گره_Assign* nodep) override {
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
            اگر (ثابت AstIfaceRefDType* ثابت irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                اگر (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                    بازگشت;
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
            Ast_گره_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            اگر (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ثابت Ast_گره_DType* ثابت lhsDtp = lhsDTypep->skipRefp();
                ثابت Ast_گره_DType* ثابت rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ثابت منطقی lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ثابت منطقی rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                اگر (lhsIsUnpackArray && rhsIsUnpackArray) {
                    اگر (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_گره_Var_Ref, Ast_گره_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), گره_Var_Ref), VN_CAST(nodep->rhsp(), گره_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                اگر (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), گره_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        اگر (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        اگر (Ast_گره* ثابت controlp = nodep->timingControlp()) {
            اگر (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_درخت(), controlp);
            } وگرنه اگر (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } وگرنه {
                اگر (nodep->fileline()->timingOn()) {
                    اگر (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } وگرنه {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_درخت(), controlp);
            }
        }
        اگر (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ثابت Ast_گره_DType* ثابت lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            اگر (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
            AstMethodCall* ثابت newp
                = جدید AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            بازگشت;
        }
        اگر (ثابت AstNewDynamic* ثابت dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            اگر (!dynp->rhsp()) {
                newp = جدید AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } وگرنه {
                newp = جدید AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(درست);
            newp->protect(نادرست);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        اگر (AstCMethodHard* ثابت slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            اگر (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } وگرنه اگر (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } وگرنه اگر (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } وگرنه {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            اگر (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_گره_عبارت* ثابت fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_گره_عبارت* ثابت lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_گره_عبارت* ثابت msbp = slicep->pinsp()->unlinkFrBack();
                Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* ثابت newp
                    = جدید AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(درست);
                newp->protect(نادرست);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            }
        }

        اگر (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    تهی visit(AstRSRule* nodep) override {
        اگر (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    تهی visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    تهی visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    ایستا منطقی isFormatNonNumericArg(ثابت Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        بازگشت dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, گره_UOr_Struct_DType)
                   && !VN_AS(dtypep, گره_UOr_Struct_DType)->packed());
    }

    تهی checkFormatNumericArg(Ast_گره* argp, نویسه ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        اگر (std::tolower(ch) == 'd') بازگشت;
        // In contrast, %x of a class reference, causes errors on several simulators.
        اگر (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    تهی visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    تهی visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    تهی visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        اگر (m_vup && !m_vup->prelim()) بازگشت;  // Can be called as statement or function
        اگر (nodep->didWidthAndSet()) بازگشت;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        اگر (!nodep->عبارت_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_گره_عبارت* oldExprsp = nodep->exprsp();
        اگر (nodep->عبارت_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), گره_عبارت);
        }
        اگر (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        وقتی (Ast_گره_عبارت* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), گره_عبارت);
            اگر (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            اگر (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                ادامه;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_گره_DType* ثابت dtypep = argp->dtypep()->skipRefp();
            اگر (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } وگرنه اگر (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } وگرنه اگر (isFormatNonNumericArg(dtypep)) {
                اگر (AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    اگر (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(درست);
                    }
                } وگرنه {
                    // Class handles inside containers print type names, not their members.
                    Ast_گره_DType* گره_Dtypep = dtypep;
                    وقتی (گره_Dtypep) {
                        گره_Dtypep = گره_Dtypep->skipRefp();
                        اگر (Ast_گره_UOr_Struct_DType* ثابت uOrStructDTypep
                            = VN_CAST(گره_Dtypep, گره_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(درست);
                        }
                        گره_Dtypep = گره_Dtypep->subDTypep();
                    }
                }
                Ast_گره_عبارت* ثابت newp = جدید AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } وگرنه اگر (nodep->عبارت_Format_2()) {
                اگر (AstEnumDType* ثابت enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    ادامه;
                }
            }
            اگر (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            اگر (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } وگرنه {
                nodep->addExprsp(جدید AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    تهی visit(AstCReset* nodep) override {
        assert_At_عبارت(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    تهی visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    تهی visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    تهی visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        اگر (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        اگر (!m_paramsOnly) {
            nodep->foreach([این](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    جدید AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            اگر (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            انتخاب (nodep->displayType()) {
            حالت VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); بشکن;
            حالت VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); بشکن;
            حالت VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); بشکن;
            حالت VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); بشکن;
            پیشفرض: UASSERT_OBJ(نادرست, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        }
    }
    تهی visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        اگر (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    تهی visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    تهی visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    تهی visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    تهی visit(AstFError* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    تهی visit(AstFEof* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    تهی visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        اگر (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    تهی visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    تهی visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    تهی visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    تهی visit(AstFGetC* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    تهی visit(AstFGetS* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    تهی visit(AstFUngetC* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    تهی visit(AstFRead* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            اگر (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            اگر (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    تهی visit(AstFScanF* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    تهی visit(AstSScanF* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    تهی visit(Ast_پشته_Trace_F* nodep) override { nodep->dtypeSetString(); }
    تهی visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstSystemF* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(Ast_گره_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        ثابت Ast_گره_DType* subp = nullptr;
        اگر (ثابت AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            اگر (!adtypep->کلید_DTypep()->skipRefp()->basicp()
                || !adtypep->کلید_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } وگرنه اگر (ثابت AstUnpackArrayDType* ثابت adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } وگرنه اگر (ثابت AstDynArrayDType* ثابت adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } وگرنه {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        اگر (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstTestPlusArgs* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    تهی visit(Ast_مقدار_Plus_Args* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    تهی visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        اگر (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        اگر (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        اگر (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        اگر (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    تهی visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    تهی visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    تهی visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    تهی visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    تهی visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    منطقی pinInstArrayElement(AstPin* nodep, ثابت AstRange* rangep, صحیح idx) {
        Ast_گره_DType* ثابت modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_گره_DType* ثابت conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        اگر (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) بازگشت درست;
        // Types don't match, but all dimensions done, still need the usual checks
        اگر (!rangep) بازگشت نادرست;

        ثابت AstRange* ثابت subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        صحیح subElems = 1;
        برای (ثابت AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ثابت صحیح elements = rangep->elementsConst();
        ثابت صحیح from_چپ = idx / subElems;

        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ثابت uint32_t conDims = conDTypep->dimensions(نادرست).second;
        ثابت uint32_t modDims = modDTypep->dimensions(نادرست).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        اگر (arrp && conDims != modDims) {
            اگر (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                بازگشت درست;
            }
            // ArraySel index is counted from lo
            ثابت صحیح bit = arrp->declRange().ascending() ? from_چپ : elements - 1 - from_چپ;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->exprp(), گره_عبارت)->unlinkFrBack();
            اگر (Ast_گره_عبارت* ثابت elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(افزودن_Deletep(fromp), fromp);
            } وگرنه {
                AstArraySel* ثابت selp = جدید AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(درست);
                nodep->exprp(selp);
            }
            بازگشت pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        اگر (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) بازگشت نادرست;

        // Packed connection
        ثابت صحیح modwidth = modDTypep->width();
        ثابت صحیح conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        اگر (conwidth == modwidth) بازگشت نادرست;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        اگر (conwidth == modwidth * elements * subElems) {
            اگر (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->چپ_Const() << ":"
                                                     << rangep->راست_Const() << "]");
            }
            ثابت صحیح selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->exprp(), گره_عبارت)->unlinkFrBack();
            AstSel* ثابت selp = جدید AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_چپ), selwidth};
            selp->didWidth(درست);
            nodep->exprp(selp);
            بازگشت pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        بازگشت درست;
    }

    تهی visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        اگر (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ثابت concatp = VN_CAST(nodep->exprp(), Replicate);
            ثابت AstConst* ثابت countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            ثابت Ast_گره_DType* ثابت dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            اگر (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* ثابت patternp = جدید AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(افزودن_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } وگرنه اگر (!m_paramsOnly) {
            اگر (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            اگر (!nodep->exprp()) {  // No-connect
                اگر (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                بازگشت;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ثابت VDirection pinDirection = nodep->modVarp()->direction();
            اگر (VN_IS(nodep->exprp(), گره_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_گره_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_گره_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            اگر (ثابت AstIfaceRefDType* ثابت modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ثابت AstIfaceRefDType* ثابت conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                اگر (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_گره_DType* subDTypep = modDTypep;
            ثابت صحیح modwidth = modDTypep->width();
            صحیح conwidth = conDTypep->width();
            اگر (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } وگرنه {
                اگر (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    اگر (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        اگر (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        بازگشت;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                اگر (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } وگرنه اگر (nodep->modVarp()->isTristate()) {
                    اگر (modwidth != conwidth) {
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
                } وگرنه اگر (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                اگر ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ثابت Ast_گره_Array_DType* ثابت عبارت_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ثابت Ast_گره_Array_DType* ثابت modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                اگر (عبارت_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } وگرنه اگر ((عبارت_Arrayp && !modArrayp) || (!عبارت_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (عبارت_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } وگرنه {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_گره_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), گره_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            اگر (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    تهی visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        اگر (!m_paramsOnly) {
            اگر (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_پیام(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            اگر (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            اگر (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                افزودن_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    تهی visit(AstGatePin* nodep) override {
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ثابت Ast_گره_DType* ثابت pinDTypep = nodep->exprp()->dtypep();
            ثابت صحیح رقم_Insts = nodep->rangep()->elementsConst();
            ثابت صحیح modwidth = رقم_Insts;
            ثابت صحیح pinwidth = pinDTypep->width();
            اگر (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_گره_DType* ثابت subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_گره* ثابت newp
                    = جدید AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(رقم_Insts)};
                nodep->replaceWith(newp);
            } وگرنه {
                اگر (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
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
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        اگر (nodep->didWidth()) بازگشت;
        اگر (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(درست);
            nodep->didWidth(درست);
            بازگشت;
        }
        اگر (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } وگرنه اگر (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        اگر (nodep->classMethod() && nodep->name() == "new") {
            اگر (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            اگر (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_گره* firstp = nullptr;
            برای (Ast_گره* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                اگر (Ast_Stmt_عبارت* ثابت sep = VN_CAST(itemp, Stmt_عبارت)) {
                    اگر (AstNew* ثابت newp = VN_CAST(sep->exprp(), New)) {
                        اگر (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            بشکن;
                        }
                    }
                    ادامه;
                }
                اگر (firstNewStatementOkRecurse(itemp)) ادامه;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(درست);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        اگر (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        اگر (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } وگرنه اگر (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } وگرنه اگر (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        اگر (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(درست);
        nodep->doingWidth(نادرست);
        اگر (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        اگر (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](ثابت AstVarRef* varrefp) {
                   بازگشت varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } وگرنه اگر (AstAttrOf* ثابت attrp = VN_CAST(stmtp, AttrOf)) {
                اگر (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                اگر (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        اگر (protop) {
            برای (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                اگر (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    بشکن;
                } وگرنه {
                    AstVar* ثابت portp = ports[i];
                    AstAttrOf* ثابت rprotop = protos[i];
                    Ast_گره_DType* ثابت declDtp = portp->dtypep();
                    Ast_گره_DType* ثابت protoDtp = rprotop->fromp()->dtypep();
                    اگر (portp->name() != rprotop->name()) {
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
                    } وگرنه اگر (!similarDTypeRecurse(protoDtp, declDtp)) {
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
    تهی visit(AstConstraint* nodep) override {
        اگر (nodep->didWidth()) بازگشت;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(درست);
    }
    تهی visit(AstProperty* nodep) override {
        اگر (nodep->didWidth()) بازگشت;
        اگر (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(درست);
            nodep->didWidth(درست);
            بازگشت;
        }
        nodep->doingWidth(درست);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        برای (Ast_گره* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            اگر (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } وگرنه اگر (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } وگرنه اگر (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } وگرنه {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(درست);
        nodep->doingWidth(نادرست);
    }
    تهی visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    تهی visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        اگر (nodep->didWidth()) بازگشت;
        nodep->dtypeSetBit();
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } وگرنه اگر (VN_IS(stmtp, گره_عبارت)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } وگرنه {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(درست);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_گره* pkgItemp) {
        وقتی (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        بازگشت VN_CAST(pkgItemp->backp(), Package);
    }
    تهی visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_گره_FTask_Ref*>(nodep));
        اگر (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            اگر (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_عبارت))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } وگرنه {
            nodep->dtypeFrom(nodep->taskp());
        }
        اگر (nodep->fileline()->timingOn()) {
            Ast_گره_Module* ثابت classp = nodep->classOrPackagep();
            اگر (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ثابت packagep = getItemPackage(classp);
                اگر (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    ایستا منطقی areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ثابت std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ثابت std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        اگر (dims0.size() != dims1.size()) بازگشت نادرست;
        برای (size_t i = 0; i < dims0.size(); ++i) {
            اگر (dims0[i]->elementsConst() != dims1[i]->elementsConst()) بازگشت نادرست;
        }
        بازگشت درست;
    }
    // Makes sure that port and pin have same size and same datatype
    تهی checkUnpackedArrayArgs(AstVar* portp, Ast_گره* pinp) {
        اگر (AstUnpackArrayDType* ثابت portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            اگر (AstUnpackArrayDType* ثابت pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                اگر (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                ثابت AstBasicDType* ثابت portBasicp = portDtypep->basicp();
                ثابت AstBasicDType* ثابت pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ثابت منطقی sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                اگر (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } وگرنه {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    تهی processFTaskRefArgs(Ast_گره_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        اگر (nodep->didWidth()) بازگشت;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        انجامبده {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ثابت V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            اگر (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            برای (ثابت auto& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.first;
                AstArg* ثابت argp = tconnect.second;
                Ast_گره_عبارت* pinp = argp->exprp();
                اگر (!pinp) ادامه;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                اگر (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_گره_عبارت* argsp = nullptr;
                    وقتی (AstArg* ثابت nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_درخت();  // Remove the call's Arg wrapper
                    }
                    string format;
                    اگر (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->رقم().toString();
                    } وگرنه {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(افزودن_Deletep(argp), argp);
                    AstSFormatF* ثابت newp
                        = جدید AstSFormatF{nodep->fileline(), format, نادرست, argsp};
                    اگر (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(جدید AstScopeName{newp->fileline(), درست});
                    }
                    handle.relink(جدید AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    برو reloop;  // so exit early; next loop will correct it
                }  //
                وگرنه اگر (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_گره_عبارت* ثابت newp = جدید AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                اگر (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } وقتی (نادرست);
        // Stage 2
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            برای (ثابت auto& tconnect : tconnects) {
                AstVar* ثابت portp = tconnect.first;
                ثابت AstArg* ثابت argp = tconnect.second;
                Ast_گره_عبارت* ثابت pinp = argp->exprp();
                اگر (!pinp) ادامه;  // Argument error we'll find later
                // Change data types based on above accept completion
                اگر (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                اگر (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            برای (ثابت auto& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.first;
                ثابت AstArg* ثابت argp = tconnect.second;
                Ast_گره_عبارت* ثابت pinp = argp->exprp();
                اگر (!pinp) ادامه;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        اگر (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            برای (ثابت auto& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.first;
                ثابت AstArg* ثابت argp = tconnect.second;
                Ast_گره_عبارت* pinp = argp->exprp();
                اگر (!pinp) ادامه;  // Argument error we'll find later
                Ast_گره_DType* ثابت portDTypep = portp->dtypep()->skipRefToEnump();
                اگر (VN_IS(pinp, EmptyQueue)) {
                    Ast_گره_عبارت* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    اگر (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = جدید AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ثابت Ast_گره_DType* ثابت pinDTypep = pinp->dtypep()->skipRefToEnump();
                ثابت AstIfaceRefDType* ثابت portIfacep
                    = VN_CAST(portDTypep->elemDTypep(درست), IfaceRefDType);
                ثابت VCastable ifaceCastable
                    = portIfacep ? Ast_گره::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ درست)
                                 : VCastable{VCastable::UNSUPPORTED};
                ثابت منطقی matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                ثابت منطقی pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->رقم().isNull();
                اگر ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    اگر (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } وگرنه {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } وگرنه اگر (portIfacep && portIfacep->isVirtual() && portp->is_درون_داد() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } وگرنه اگر (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_گره_عبارت* ثابت newp = جدید AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                اگر (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    ادامه;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                اگر (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } وگرنه اگر (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } وگرنه {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    تهی handleStdRandomizeArgs(Ast_گره_FTask_Ref* ثابت nodep) {
        AstConst* nullp = nullptr;
        برای (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_گره_عبارت* ثابت exprp = argp->exprp();
            اگر (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                اگر (constp->رقم().isNull()) {
                    nullp = constp;
                    ادامه;
                }
            }
            اگر (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                ادامه;
            }
            اگر (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                ادامه;
            }
            اگر (ثابت AstCMethodHard* ثابت methodp = VN_CAST(exprp, CMethodHard)) {
                اگر (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    ادامه;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        اگر (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        اگر (nodep->didWidth()) بازگشت;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        اگر (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(درست);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            بازگشت;  // Handled in V3Randomize
        } وگرنه اگر (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ثابت classp = VN_CAST(nodep->classOrPackagep(), Class);
            اگر (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(درست);
                Ast_گره_DType* ثابت adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, نادرست, نادرست, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                برای (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                اگر (withp) nodep->withp(withp);
                nodep->didWidth(درست);
                بازگشت;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            اگر (nodep->name() == "randomize") {
                AstClassRefDType* ثابت adtypep
                    = جدید AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, نادرست, نادرست, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                برای (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } وگرنه اگر (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } وگرنه اگر (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(درست);
                v3Global.useRandomizeMethods(درست);
                AstCExpr* ثابت newp
                    = جدید AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            } وگرنه اگر (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_گره_عبارت* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_گره_عبارت* ثابت exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(درست);
                v3Global.useRandomizeMethods(درست);
                AstCExpr* ثابت newp
                    = جدید AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            } وگرنه {
                UASSERT_OBJ(نادرست, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        اگر (nodep->didWidth()) {
            اگر (withp) nodep->withp(withp);
            بازگشت;
        }
        اگر ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            منطقی allow = نادرست;
            اگر (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                اگر (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } وگرنه اگر (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                اگر (!allow) {
                    ثابت AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    اگر (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    اگر (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            اگر (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ثابت AstClass* ثابت cgClassp = m_containingClassFinder.find(nodep);
                اگر (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    اگر (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            اگر (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        اگر (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(جدید AstScopeName{nodep->fileline(), نادرست});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        اگر (withp) nodep->withp(withp);
        nodep->didWidth(درست);
        // See steps that follow in visit(AstFuncRef*)
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    تهی visit(AstSenItem* nodep) override {
        اگر (nodep->isComboStar()) بازگشت;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        اگر (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* ثابت concatOrReplp = VN_CAST(nodep->sensp(), گره_Biop);
            auto* ثابت rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            اگر (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* ثابت lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(جدید AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(جدید AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            اگر (nodep->edgeType().anEdge()) {
                Ast_گره_DType* ثابت sensDtp = nodep->sensp()->dtypep()->skipRefp();
                اگر (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } وگرنه اگر (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    تهی visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ثابت refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    تهی visit(AstWait* nodep) override {
        اگر (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }
        اگر (nodep->fileline()->timingOn()) {
            اگر (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                اگر (Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(nodep->condp(), گره_Var_Ref)) {
                    اگر (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                بازگشت;
            } وگرنه اگر (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } وگرنه {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        اگر (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->مقدار_Arg_Refp(), nullptr);
        اگر (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } وگرنه {
            اگر (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } وگرنه {
                اگر (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } وگرنه {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            اگر (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } وگرنه {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    تهی visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        اگر (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } وگرنه {
            nodep->dtypeFrom(m_withp->مقدار_Arg_Refp());
        }
    }
    تهی visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    تهی visit(Ast_گره_عبارت* nodep) override {
        اگر (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    تهی visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        اگر (nodep->didWidthAndSet()) بازگشت;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        اگر (Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } وگرنه اگر (!nodep->dtypep()) {
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
        nodep->didWidth(درست);
    }
    تهی visitClass(AstClass* nodep) {
        اگر (nodep->didWidthAndSet()) بازگشت;

        // If the class is std::process
        اگر (nodep->name() == "process") {
            AstPackage* ثابت packagep = getItemPackage(nodep);
            اگر (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                اگر (AstVar* ثابت varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_گره_DType* ثابت dtypep = varp->getChildDTypep();
                    اگر (!varp->dtypep()) {
                        VL_DO_DANGLING(افزودن_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* ثابت newdtypep = جدید AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                اگر (Ast_گره_FTask* ثابت ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), گره_FTask)) {
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
    تهی visit(Ast_گره_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } وگرنه {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([این](ثابت AstVar* varp) { m_curModVars.درج(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    تهی visit(Ast_گره* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    تهی visit_Or_Lu64(Ast_گره_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_گره_DType* ثابت subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    تهی visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            اگر (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    جدید AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            }
        }
    }
    تهی visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    تهی visit_Os32_Lr(Ast_گره_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    تهی visit_Ou64_Lr(Ast_گره_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    تهی visit_log_not(AstLogNot* nodep) {
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
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            اگر (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ثابت newp
                    = جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }
    تهی visit_log_and_or(Ast_گره_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    تهی visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    تهی visit_red_and_or(Ast_گره_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    تهی visit_red_unknown(Ast_گره_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    منطقی isEquivalentDType(ثابت Ast_گره_DType* lhs, ثابت Ast_گره_DType* rhs) {
        // a) If two types match, they are equivalent.
        اگر (!lhs || !rhs) بازگشت نادرست;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        اگر (lhs == rhs) بازگشت درست;
        // If both are basic types, check if they are the same type
        اگر (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ثابت auto* lb = VN_CAST(lhs, BasicDType);
            ثابت auto* rb = VN_CAST(rhs, BasicDType);
            اگر (lb->isString() != rb->isString()) بازگشت نادرست;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ثابت منطقی lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ثابت منطقی rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        اگر (lhsIsUnpackArray || rhsIsUnpackArray) {
            اگر (lhsIsUnpackArray && rhsIsUnpackArray) {
                ثابت AstUnpackArrayDType* ثابت lhsp = VN_CAST(lhs, UnpackArrayDType);
                ثابت AstUnpackArrayDType* ثابت rhsp = VN_CAST(rhs, UnpackArrayDType);
                ثابت صحیح lsz = lhsp->elementsConst();
                ثابت صحیح rsz = rhsp->elementsConst();
                اگر (lsz >= 0 && rsz >= 0 && lsz != rsz) بازگشت نادرست;
                بازگشت isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            بازگشت نادرست;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ثابت منطقی lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ثابت منطقی rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ثابت منطقی lhsIsQueue = VN_IS(lhs, QueueDType);
        ثابت منطقی rhsIsQueue = VN_IS(rhs, QueueDType);
        ثابت منطقی lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ثابت منطقی rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        اگر (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            اگر (ثابت AstDynArrayDType* ثابت lhsp = VN_CAST(lhs, DynArrayDType)) {
                اگر (ثابت AstDynArrayDType* ثابت rhsp = VN_CAST(rhs, DynArrayDType)) {
                    بازگشت isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            اگر (ثابت AstQueueDType* ثابت lhsp = VN_CAST(lhs, QueueDType)) {
                اگر (ثابت AstQueueDType* ثابت rhsp = VN_CAST(rhs, QueueDType)) {
                    بازگشت isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            اگر (ثابت AstAssocArrayDType* ثابت lhsp = VN_CAST(lhs, AssocArrayDType)) {
                اگر (ثابت AstAssocArrayDType* ثابت rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    بازگشت isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->کلید_DTypep(), rhsp->کلید_DTypep());
                }
            }

            بازگشت نادرست;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        اگر (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            اگر (lhs->width() != rhs->width()) بازگشت نادرست;
            اگر (lhs->isFourstate() != rhs->isFourstate()) بازگشت نادرست;
            اگر (lhs->isSigned() != rhs->isSigned()) بازگشت نادرست;
            بازگشت درست;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        اگر (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) بازگشت نادرست;
        بازگشت !(VN_IS(lhs, گره_UOr_Struct_DType) || VN_IS(rhs, گره_UOr_Struct_DType));
    }

    ایستا منطقی isAggregateType(ثابت Ast_گره* nodep) {
        اگر (!nodep) بازگشت نادرست;
        ثابت Ast_گره_DType* dtypep = nodep->dtypep();
        اگر (!dtypep) بازگشت نادرست;
        dtypep = dtypep->skipRefp();
        اگر (!dtypep) بازگشت نادرست;
        بازگشت dtypep->isAggregateType();
    }

    تهی visit_cmp_eq_gt(Ast_گره_Biop* nodep, ثابت ComparisonType cmpType) {
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
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ثابت منطقی lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            ثابت منطقی rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            اگر (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } وگرنه اگر (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } وگرنه {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            ثابت منطقی isAggrLhs = isAggregateType(nodep->lhsp());
            ثابت منطقی isAggrRhs = isAggregateType(nodep->rhsp());

            اگر ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                ثابت Ast_گره_DType* ثابت lhsDType = nodep->lhsp()->dtypep();
                ثابت Ast_گره_DType* ثابت rhsDType = nodep->rhsp()->dtypep();

                اگر (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_گره* ثابت newp
                        = جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } وگرنه اگر (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                اگر (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_گره* ثابت newp
                        = جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
                اگر (Ast_گره_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } وگرنه اگر (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                اگر (Ast_گره_Biop* ثابت newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } وگرنه {
                ثابت منطقی signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                اگر (Ast_گره_Biop* ثابت newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ثابت صحیح width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ثابت صحیح ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                منطقی warnOn = درست;
                اگر (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    اگر ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = نادرست;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    اگر ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = نادرست;
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
    تهی visit_cmp_real(Ast_گره_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    تهی visit_cmp_string(Ast_گره_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    تهی visit_cmp_type(Ast_گره_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            ثابت AstAttrOf* ثابت lhsap = VN_AS(nodep->lhsp(), AttrOf);
            ثابت AstAttrOf* ثابت rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ثابت Ast_گره_DType* ثابت lhsDtp = lhsap->dtypep();
            ثابت Ast_گره_DType* ثابت rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ثابت منطقی invert = VN_IS(nodep, NeqT);
            ثابت منطقی identical
                = Ast_گره::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ثابت منطقی eq = invert ^ identical;
            Ast_گره* ثابت newp = جدید AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    تهی visit_negate_not(Ast_گره_Uniop* nodep, منطقی real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            اگر (!real_ok) checkCvtUS(nodep->lhsp(), نادرست);
        }
        اگر (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            اگر (Ast_گره_Uniop* ثابت newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                بازگشت;
            }
        } وگرنه {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        اگر (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        اگر (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            // Some warning suppressions
            منطقی lhsWarn = درست;
            اگر (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                اگر (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = نادرست;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    تهی visit_signed_unsigned(Ast_گره_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), درست);
            ثابت صحیح width = nodep->lhsp()->width();
            Ast_گره_DType* ثابت expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    تهی visit_shift(Ast_گره_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ثابت Ast_گره_Biop* ثابت newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (تهی)newp;  // Ununused
    }
    تهی iterate_shift_prelim(Ast_گره_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), نادرست);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            اگر (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_گره_Biop* iterate_shift_final(Ast_گره_Biop* nodep) {
        // Nodep maybe edited
        assert_At_عبارت(nodep);
        اگر (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            اگر (VN_IS(nodep, ShiftRS)) {
                اگر (Ast_گره_Biop* ثابت newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            منطقی warnOn = درست;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            اگر (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), گره_Assign)) warnOn = نادرست;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const);
            اگر (shiftp && !shiftp->رقم().isFourState() && shiftp->width() <= 32) {
                ثابت int64_t shiftVal = shiftp->رقم().toSQuad();
                اگر (VN_IS(nodep, ShiftL)) {
                    اگر (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = نادرست;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            اگر (nodep->rhsp()->width() > 32) {
                اگر (shiftp && shiftp->رقم().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_عدد رقم{shiftp, 32, shiftp->رقم()};
                    Ast_گره* ثابت shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(جدید AstConst{shiftrhsp->fileline(), رقم});
                    VL_DO_DANGLING(shiftrhsp->delete_درخت(), shiftrhsp);
                }
            }
        }
        بازگشت nodep;  // May edit
    }

    تهی visit_boolexpr_and_or(Ast_گره_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), نادرست);
            checkCvtUS(nodep->rhsp(), نادرست);
            ثابت صحیح width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            ثابت صحیح mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            ثابت منطقی expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        اگر (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    تهی visit_add_sub_replace(Ast_گره_Biop* nodep, منطقی real_ok, ثابت منطقی modOrDiv) {
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
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            اگر (!real_ok) {
                checkCvtUS(nodep->lhsp(), نادرست);
                checkCvtUS(nodep->rhsp(), نادرست);
            }
            اگر (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                اگر (Ast_گره_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                بازگشت;
            } وگرنه اگر (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            } وگرنه {
                ثابت صحیح width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ثابت صحیح mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                ثابت منطقی expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        اگر (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            اگر (Ast_گره_Biop* ثابت newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            منطقی lhsWarn = درست;
            منطقی rhsWarn = درست;
            اگر (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                اگر (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = نادرست;
                اگر (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = نادرست;
                اگر (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = نادرست;  // do_increment + ...
                اگر (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = نادرست;  // ... + do_increment
            } وگرنه اگر (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                اگر (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = نادرست;
                اگر (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = نادرست;
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
    تهی visit_real_add_sub(Ast_گره_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    تهی visit_real_neg_ceil(Ast_گره_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_عبارت(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    تهی visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->عبارت_Format_2(), nodep, "Assumes constant format");
        منطقی inPct = نادرست;
        Ast_گره_عبارت* argp = nodep->exprsp();
        string newFormat;
        برای (نویسه ch : nodep->text()) {
            اگر (!inPct && ch == '%') {
                inPct = درست;
                newFormat += ch;
            } وگرنه اگر (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } وگرنه اگر (!inPct) {  // Normal text
                newFormat += ch;
            } وگرنه {
                inPct = نادرست;
                Ast_گره_عبارت* ثابت nextp = argp ? VN_AS(argp->nextp(), گره_عبارت) : nullptr;
                AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_گره_عبارت* subargp = fargp ? fargp->exprp() : argp;
                ثابت Ast_گره_DType* ثابت dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                انتخاب (ch) {
                حالت '?':  // Unspecified by user, guess
                    اگر (dtypep->isDouble()) {
                        ch = 'g';
                    } وگرنه اگر (dtypep->isString()) {
                        ch = 's';
                    } وگرنه اگر (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } وگرنه {
                        ch = 'p';
                    }
                    argp = nextp;
                    بشکن;
                حالت '%': بشکن;  // %% - just output a %
                حالت 'm': بشکن;  // %m - auto insert "name"
                حالت 'l': بشکن;  // %m - auto insert "library"
                حالت 'd':
                    اگر (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        اگر (subargp->isDouble()) spliceCvtS(subargp, درست, 64);
                        argp = nextp;
                    }
                    بشکن;
                حالت 'b':  // FALLTHRU
                حالت 'o':  // FALLTHRU
                حالت 'x':
                    اگر (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    بشکن;
                حالت 't':  // Decimal/float time
                    اگر (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    بشکن;
                حالت 'e':  // FALLTHRU
                حالت 'f':  // FALLTHRU
                حالت 'g':
                    اگر (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        اگر (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    بشکن;
                حالت 'p':  // FALLTHRU
                حالت 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    اگر (subargp && !fargp) {
                        اگر (AstEnumDType* ثابت enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    بشکن;
                پیشفرض:  // Most operators, just move to next argument
                    argp = nextp;
                    بشکن;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    ایستا AstEnumDType* formatEnumDType(Ast_گره_عبارت* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        اگر (!enumDtp) {
            اگر (ثابت AstVarRef* ثابت varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        بازگشت enumDtp;
    }

    Ast_عبارت_Stmt* newEnumCapture(Ast_گره_عبارت* valuep, AstEnumDType* dtypep) {
        File_سطر* ثابت flp = valuep->fileline();
        AstVar* ثابت varp = جدید AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(درست);
        اگر (m_ftaskp) {
            varp->funcLocal(درست);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } وگرنه {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_عبارت_Stmt* ثابت exprp = جدید Ast_عبارت_Stmt{
            flp, جدید AstAssign{flp, جدید AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            جدید AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        بازگشت exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_گره_عبارت* valuep, AstEnumDType* dtypep) {
        اگر (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_گره_عبارت* keyp;
        اگر (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_درخت(نادرست);
        } وگرنه {
            Ast_عبارت_Stmt* ثابت capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_درخت_Pure(نادرست);
        }
        ثابت VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ثابت argp = جدید AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        بازگشت argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    منطقی widthBad(Ast_گره* nodep, Ast_گره_DType* expDTypep) {
        ثابت صحیح expWidth = expDTypep->width();
        صحیح expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        اگر (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) بازگشت نادرست;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        اگر (expWidthMin == 0) expWidthMin = expWidth;
        اگر (nodep->dtypep()->width() == expWidth) بازگشت نادرست;
        اگر (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) بازگشت درست;
        اگر (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) بازگشت درست;
        بازگشت نادرست;
    }

    تهی fixWidthExtend(Ast_گره_عبارت* nodep, Ast_گره_DType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<صحیح>(extendRule) << ") old: " << nodep);
        اگر (extendRule == EXTEND_OFF) بازگشت;
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        ثابت صحیح expWidth = expDTypep->width();
        اگر (constp && !constp->رقم().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_عدد رقم{nodep, expWidth, constp->رقم()};
            رقم.isSigned(نادرست);
            Ast_گره_عبارت* ثابت newp = جدید AstConst{nodep->fileline(), رقم};
            constp->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } وگرنه اگر (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_گره_عبارت* ثابت newp = جدید AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(درست);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } وگرنه {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            منطقی doSigned = نادرست;
            انتخاب (extendRule) {
            حالت EXTEND_ZERO: doSigned = نادرست; بشکن;
            حالت EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); بشکن;
            حالت EXTEND_LHS: doSigned = nodep->isSigned(); بشکن;
            پیشفرض: nodep->v3fatalSrc("bad case");
            }
            Ast_گره_عبارت* ثابت newp
                = (doSigned ? static_cast<Ast_گره_عبارت*>(جدید AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_گره_عبارت*>(جدید AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(درست);
            linker.relink(newp);
            nodep = newp;
        }
        اگر (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_گره_عبارت* ثابت newp = spliceCvtD(nodep);
            nodep = newp;
        }
        اگر (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } وگرنه {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    تهی fixWidthReduce(Ast_گره_عبارت* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ثابت صحیح expWidth = 1;
        ثابت صحیح expSigned = نادرست;
        اگر (nodep->width() == expWidth) بازگشت;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        اگر (constp) {
            V3_عدد رقم(nodep, expWidth);
            رقم.opRedOr(constp->رقم());
            رقم.isSigned(expSigned);
            Ast_گره_عبارت* ثابت newp = جدید AstConst{nodep->fileline(), رقم};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_درخت(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } وگرنه {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_گره_عبارت* newp;
            Ast_گره_DType* ثابت dtypeSkipp = nodep->dtypep()->skipRefp();
            اگر (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = جدید AstNeq{nodep->fileline(),
                                  جدید AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } وگرنه {
                // But, AstRedOr can be faster on wide
                newp = جدید AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    منطقی fixAutoExtend(Ast_گره_عبارت*& nodepr, صحیح expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        اگر (AstConst* ثابت constp = VN_CAST(nodepr, Const)) {
            اگر (constp->رقم().autoExtend() && !constp->رقم().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_عدد رقم(constp, expWidth);
                رقم.opRepl(constp->رقم(), expWidth);  // {width{'1}}
                Ast_گره_عبارت* ثابت newp = جدید AstConst{constp->fileline(), رقم};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_درخت(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                بازگشت درست;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            وگرنه اگر (!constp->رقم().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->رقم().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_عدد رقم(constp, expWidth);
                رقم.opExtendXZ(constp->رقم(), constp->width());
                Ast_گره_عبارت* ثابت newp = جدید AstConst{constp->fileline(), رقم};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_درخت(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                بازگشت درست;
            }
        }
        بازگشت نادرست;  // No change
    }
    منطقی isBaseClassRecurse(ثابت AstClass* ثابت cls1p, ثابت AstClass* ثابت cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        اگر (cls1p == cls2p) بازگشت درست;
        برای (ثابت AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            اگر (isBaseClassRecurse(cls1p, cextp->classp())) بازگشت درست;
        }
        بازگشت نادرست;
    }
    // Lines to end a message on nodep with the data types of its operands
    ایستا string warnDTypes(ثابت Ast_گره* nodep, ثابت Ast_گره_DType* lhsDtp,
                             ثابت Ast_گره_DType* rhsDtp, ثابت string& lhsName = "Left-hand",
                             ثابت string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        اگر (lhsState == rhsState) lhsState = rhsState = "";
        بازگشت "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    ایستا منطقی unpackedArraySizesMatch(ثابت Ast_گره_DType* lhsDtp, ثابت Ast_گره_DType* rhsDtp) {
        وقتی (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ثابت AstUnpackArrayDType* ثابت lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ثابت AstUnpackArrayDType* ثابت rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            اگر (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                بازگشت نادرست;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        بازگشت درست;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    تهی checkUnpackedArrayAssignmentCompatible(ثابت Ast_گره* nodep, ثابت T* ثابت lhsRefp,
                                                ثابت N* ثابت rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::مقدار || std::is_same<T, Ast_گره_Var_Ref>::مقدار)
                && (std::is_same<N, AstVar>::مقدار || std::is_same<N, Ast_گره_Var_Ref>::مقدار),
            "Unsupported types provided.");
        اگر (!lhsRefp || !rhsRefp) بازگشت;
        std::string lhsName;
        std::string rhsName;
        اگر (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } وگرنه {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ثابت Ast_گره_DType* ثابت lhsDtp = lhsRefp->dtypep()->skipRefp();
        ثابت Ast_گره_DType* ثابت rhsDtp = rhsRefp->dtypep()->skipRefp();
        ثابت منطقی isLhsAggregate = lhsDtp->isAggregateType();
        ثابت منطقی isRhsAggregate = rhsDtp->isAggregateType();
        اگر (!isLhsAggregate && !isRhsAggregate) بازگشت;
        اگر (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            بازگشت;
        } وگرنه اگر (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            بازگشت;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(نادرست),
                                      rhsDim = rhsDtp->dimensions(نادرست);
        // Check if unpacked array dimensions are matching
        اگر (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            بازگشت;
        }

        ثابت Ast_گره_DType* lhsDtpIterp = lhsDtp;
        ثابت Ast_گره_DType* rhsDtpIterp = rhsDtp;
        برای (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            اگر (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                بازگشت;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        اگر (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    تهی checkClassAssign(ثابت Ast_گره* nodep, ثابت نویسه* side, Ast_گره* rhsp,
                          Ast_گره_DType* ثابت lhsDTypep, منطقی isOutputArg = نادرست) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ثابت Ast_گره_DType* ثابت lhsRawDTypep = lhsDTypep->skipRefp();
        ثابت Ast_گره_DType* ثابت rhsRawDTypep = rhsp->dtypep()->skipRefp();
        اگر (ثابت AstClassRefDType* ثابت lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            اگر (ثابت AstClassRefDType* ثابت rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ثابت AstClass* ثابت dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ثابت AstClass* ثابت srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                اگر (isBaseClassRecurse(dstp, srcp)) بازگشت;
            } وگرنه اگر (rhsp->isNull()) {
                بازگشت;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } وگرنه اگر (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        اگر (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                جدید AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, گره_عبارت), lhsDTypep});
        }
    }
    ایستا منطقی similarDTypeRecurse(ثابت Ast_گره_DType* ثابت node1p,
                                    ثابت Ast_گره_DType* ثابت node2p) {
        بازگشت node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    تهی iterateCheckFileDesc(Ast_گره* parentp, Ast_گره* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_گره_DType* ثابت expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (تهی)underp;  // cppcheck
    }
    تهی iterateCheckReal(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    تهی iterateCheckSigned8(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    تهی iterateCheckSigned32(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    تهی iterateCheckUInt32(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    تهی iterateCheckDelay(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        اگر (stage & FINAL) {
            Ast_گره_DType* expDTypep;
            اگر (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } وگرنه {
                File_سطر* ثابت newFl = جدید File_سطر{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, درست);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, نادرست);
        }
        (تهی)underp;  // cppcheck
    }
    تهی iterateCheckString(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    تهی iterateCheckTyped(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp,
                           Ast_گره_DType* expDTypep, Stage stage) {
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        اگر (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (تهی)underp;  // cppcheck
    }
    تهی iterateCheckTypedSelfPrelim(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp,
                                     Ast_گره_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        اگر (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (تهی)underp;  // cppcheck
    }
    تهی iterateCheckIntegralSelf(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, درست);
    }
    تهی iterateCheckSelf(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Determ determ,
                          Stage stage, منطقی integralOnly = نادرست) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, گره_عبارت) ? checkCvtUS(VN_AS(underp, گره_عبارت), integralOnly) : underp;
        Ast_گره_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (تهی)underp;  // cppcheck
    }
    تهی iterateCheckSizedSelf(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Determ determ,
                               Stage stage, منطقی deferUntyped = نادرست) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, گره_عبارت) ? checkCvtUS(VN_AS(underp, گره_عبارت), نادرست) : underp;
        Ast_گره_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_گره_DType* ثابت checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        اگر (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (تهی)underp;  // cppcheck
    }
    تهی iterateCheckAssign(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* rhsp, Stage stage,
                            Ast_گره_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        اگر (AstConst* constp = VN_CAST(rhsp, Const)) {
            اگر (ثابت AstUnpackArrayDType* ثابت arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                اگر (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    اگر (basicp->width() == 8 && constp->رقم().isFromString()) {
                        AstConsArray* newp = جدید AstConsArray{
                            constp->fileline(), lhsDTypep,
                            جدید AstConst{constp->fileline(), AstConst::Widthed_مقدار{}, 8, 0}};
                        برای (ثابت صحیح aindex : arrayp->declRange().seqLoToHi()) {
                            صحیح cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_عدد selected{constp, 8};
                            selected.opSel(constp->رقم(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            اگر (!selected.isFourState()) {
                                اگر (ثابت uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 جدید AstConst{constp->fileline(),
                                                              AstConst::Widthed_مقدار{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        ثابت منطقی lhsStream = (VN_IS(parentp, گره_Assign)
                                && VN_IS(VN_AS(parentp, گره_Assign)->lhsp(), گره_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, درست, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (تهی)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_گره_عبارت* convertUnpackedOrder(Ast_گره_عبارت* exprp, Ast_گره_DType* dtypep) {
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        اگر (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) بازگشت exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_گره_عبارت* ثابت newp = جدید AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        بازگشت newp;
    }

    ایستا منطقی isLogicalOperator(ثابت Ast_گره* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        بازگشت VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    تهی iterateCheckBool(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Stage stage) {
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
        Ast_گره_DType* ثابت underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        اگر (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_گره* ثابت newp
                = جدید AstNeqD{parentp->fileline(), VN_AS(underp, گره_عبارت),
                              جدید AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } وگرنه اگر (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_گره* ثابت newp
                = جدید AstNeqN{parentp->fileline(), VN_AS(underp, گره_عبارت),
                              جدید AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } وگرنه اگر (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, گره_عبارت)), underp);  // Changed
        } وگرنه اگر (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(جدید AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(افزودن_Deletep(underp), underp);
        } وگرنه {
            ثابت منطقی bad = widthBad(underp, parentp->findBitDType());
            اگر (bad) {
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
            اگر (Ast_عبارت_Stmt* ثابت عبارت_Stmtp = VN_CAST(underp, عبارت_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(عبارت_Stmtp->resultp());
                عبارت_Stmtp->dtypeFrom(عبارت_Stmtp->resultp());
            } وگرنه اگر (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, گره_عبارت)), underp);  // Changed
            }
        }
    }

    ایستا تهی checkEnum(Ast_گره* nodep, Ast_گره* parentp, ثابت Ast_گره_DType* targetDTypep,
                          ثابت Ast_گره_DType* sourceDTypep, ثابت Ast_گره_عبارت* sourceExprp) {
        اگر (ثابت AstEnumDType* ثابت targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            ثابت VCastable castable
                = Ast_گره::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            اگر (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    Ast_گره* iterateCheck(Ast_گره* parentp, ثابت نویسه* side, Ast_گره* underp, Determ determ,
                          Stage stage, Ast_گره_DType* expDTypep, ExtendRule extendRule,
                          منطقی warnOn = درست, StreamUse streamUse = STREAM_USE_NONE,
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
        اگر (rhsAccess.isReadOnly()) {
            اگر (Ast_گره_عبارت* ثابت exprp = VN_CAST(underp, گره_عبارت)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        اگر (VN_IS(underp, گره_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } وگرنه اگر (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } وگرنه اگر (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } وگرنه اگر (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_گره* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, گره_عبارت));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } وگرنه اگر (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_گره* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, گره_عبارت), درست, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } وگرنه اگر (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_گره* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, گره_عبارت));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } وگرنه {
            ثابت Ast_گره_DType* ثابت underDtp = underp->dtypep()->skipRefp();
            اگر (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_گره* ثابت newp
                    = جدید AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(underp), underp);
                underp = newp;
            }
            ثابت Ast_گره_DType* ثابت expDtp = expDTypep->skipRefp();
            اگر (determ == ASSIGN && VN_IS(expDtp, گره_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            اگر (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ثابت AstBasicDType* ثابت expBasicp = expDTypep->basicp();
            ثابت AstBasicDType* ثابت underBasicp = underp->dtypep()->basicp();
            اگر (expBasicp && underBasicp) {
                اگر (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    اگر (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, گره_عبارت));
                    }
                    اگر (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_گره_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                اگر (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } وگرنه اگر (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    اگر (underp->isSigned() != subDTypep->isSigned()
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
                } وگرنه {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, گره_عبارت), expDTypep, extendRule,
                                warnOn);
            } وگرنه اگر (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } وگرنه اگر (ثابت AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ثابت AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                اگر (VN_IS(underp, Const) && VN_AS(underp, Const)->رقم().isNull()) {
                    // '= null' is ok
                } وگرنه اگر (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } وگرنه اگر (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } وگرنه اگر (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } وگرنه {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } وگرنه {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        بازگشت underp;
    }

    تهی
    widthCheckSized(Ast_گره* parentp, ثابت نویسه* side,
                    Ast_گره_عبارت* underp,  // Node to be checked or have typecast added in front of
                    Ast_گره_DType* expDTypep, ExtendRule extendRule, منطقی warnOn = درست) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ثابت AstBasicDType* ثابت expBasicp = expDTypep->basicp();
        ثابت AstBasicDType* ثابت underBasicp = underp->dtypep()->basicp();
        اگر (expDTypep == underp->dtypep()) {
            بازگشت;  // Same type must match
        } وگرنه اگر (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#اگر VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            بازگشت;
        } وگرنه {
            ثابت صحیح expWidth = expDTypep->width();
            صحیح expWidthMin = expDTypep->widthMin();
            اگر (expWidthMin == 0) expWidthMin = expWidth;
            ثابت منطقی bad = widthBad(underp, expDTypep);
            اگر ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                بازگشت;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            اگر (VN_IS(parentp->backp(), CastSize)) warnOn = نادرست;
            اگر (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = نادرست;
            اگر (VN_IS(underp, Const) && VN_AS(underp, Const)->رقم().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = نادرست;
            }
            اگر ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = نادرست;
            }
            اگر (bad && warnOn) {
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
            اگر (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ثابت Ast_گره_Assign* assignp = VN_CAST(parentp, گره_Assign);
                ثابت AstPin* pinp = VN_CAST(parentp, Pin);
                اگر (assignp && VN_IS(assignp->lhsp(), گره_Stream)) {
                } وگرنه اگر (pinp && pinp->modVarp()->direction() != VDirection::درون_داد) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } وگرنه اگر (assignp && VN_IS(underp, گره_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ثابت صحیح shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* ثابت widenedp = جدید AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(درست);
                    // Shift left so zeros fill on the right
                    Ast_گره_عبارت* ثابت shiftedp = جدید AstShiftL{
                        underp->fileline(), widenedp,
                        جدید AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } وگرنه {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_گره_عبارت* checkCvtUS(Ast_گره_عبارت* nodep, منطقی fatal) {
        اگر (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            اگر (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } وگرنه {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, نادرست, 32);
        }
        بازگشت nodep;
    }

    Ast_گره_عبارت* spliceCvtD(Ast_گره_عبارت* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        اگر (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_گره_عبارت* newp;
            اگر (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = جدید AstISToRD{nodep->fileline(), nodep};
            } وگرنه {
                newp = جدید AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            بازگشت newp;
        } وگرنه {
            بازگشت nodep;
        }
    }
    Ast_گره_عبارت* spliceCvtS(Ast_گره_عبارت* nodep, منطقی warnOn, صحیح width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        اگر (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                اگر (constp->isDouble() && V3_عدد::epsilonIntegral(constp->رقم().toDouble())) {
                    warnOn = نادرست;
                }
            }
            اگر (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_گره_عبارت* ثابت newp = جدید AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            بازگشت newp;
        } وگرنه {
            بازگشت nodep;
        }
    }
    Ast_گره_عبارت* spliceCvtString(Ast_گره_عبارت* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        اگر (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_گره_عبارت* ثابت newp = جدید AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            بازگشت newp;
        } وگرنه {
            بازگشت nodep;
        }
    }
    Ast_گره_Biop* replaceWithUOrSVersion(Ast_گره_Biop* nodep, منطقی signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        اگر (signedFlavorNeeded == nodep->signedFlavor()) بازگشت nullptr;
        اگر (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        انتخاب (nodep->type()) {
        حالت VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); بازگشت nullptr;
        حالت VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); بازگشت nullptr;
        حالت VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); بازگشت nullptr;
        حالت VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); بازگشت nullptr;
        حالت VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); بازگشت nullptr;
        حالت VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); بازگشت nullptr;
        حالت VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); بازگشت nullptr;
        حالت VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); بازگشت nullptr;
        حالت VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); بازگشت nullptr;
        پیشفرض: بشکن;
        }
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گره_Biop* newp = nullptr;
        انتخاب (nodep->type()) {
        حالت VNType::Gt: newp = جدید AstGtS{fl, lhsp, rhsp}; بشکن;
        حالت VNType::GtS: newp = جدید AstGt{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Gte: newp = جدید AstGteS{fl, lhsp, rhsp}; بشکن;
        حالت VNType::GteS: newp = جدید AstGte{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Lt: newp = جدید AstLtS{fl, lhsp, rhsp}; بشکن;
        حالت VNType::LtS: newp = جدید AstLt{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Lte: newp = جدید AstLteS{fl, lhsp, rhsp}; بشکن;
        حالت VNType::LteS: newp = جدید AstLte{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Div: newp = جدید AstDivS{fl, lhsp, rhsp}; بشکن;
        حالت VNType::DivS: newp = جدید AstDiv{fl, lhsp, rhsp}; بشکن;
        حالت VNType::ModDiv: newp = جدید AstModDivS{fl, lhsp, rhsp}; بشکن;
        حالت VNType::ModDivS: newp = جدید AstModDiv{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Mul: newp = جدید AstMulS{fl, lhsp, rhsp}; بشکن;
        حالت VNType::MulS: newp = جدید AstMul{fl, lhsp, rhsp}; بشکن;
        حالت VNType::ShiftR: newp = جدید AstShiftRS{fl, lhsp, rhsp}; بشکن;
        حالت VNType::ShiftRS: newp = جدید AstShiftR{fl, lhsp, rhsp}; بشکن;
        پیشفرض:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            بشکن;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت newp;
    }
    Ast_گره_Biop* replaceWithDVersion(Ast_گره_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        اگر (nodep->doubleFlavor()) بازگشت nullptr;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گره_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        انتخاب (nodep->type()) {
        حالت VNType::Add: newp = جدید AstAddD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Sub: newp = جدید AstSubD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Pow: newp = جدید AstPowD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Eq:
        حالت VNType::EqCase: newp = جدید AstEqD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Neq:
        حالت VNType::NeqCase: newp = جدید AstNeqD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Gt:
        حالت VNType::GtS: newp = جدید AstGtD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Gte:
        حالت VNType::GteS: newp = جدید AstGteD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Lt:
        حالت VNType::LtS: newp = جدید AstLtD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Lte:
        حالت VNType::LteS: newp = جدید AstLteD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Div:
        حالت VNType::DivS: newp = جدید AstDivD{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Mul:
        حالت VNType::MulS: newp = جدید AstMulD{fl, lhsp, rhsp}; بشکن;
        پیشفرض:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            بشکن;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت newp;
    }
    Ast_گره_Biop* replaceWithNVersion(Ast_گره_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        اگر (nodep->stringFlavor()) بازگشت nullptr;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گره_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        انتخاب (nodep->type()) {
        حالت VNType::Eq:
        حالت VNType::EqCase:
        حالت VNType::EqWild: newp = جدید AstEqN{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Neq:
        حالت VNType::NeqCase:
        حالت VNType::NeqWild: newp = جدید AstNeqN{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Gt:
        حالت VNType::GtS: newp = جدید AstGtN{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Gte:
        حالت VNType::GteS: newp = جدید AstGteN{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Lt:
        حالت VNType::LtS: newp = جدید AstLtN{fl, lhsp, rhsp}; بشکن;
        حالت VNType::Lte:
        حالت VNType::LteS: newp = جدید AstLteN{fl, lhsp, rhsp}; بشکن;
        پیشفرض:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            بشکن;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت newp;
    }
    Ast_گره_Uniop* replaceWithDVersion(Ast_گره_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        اگر (nodep->doubleFlavor()) بازگشت nullptr;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گره_Uniop* newp = nullptr;
        انتخاب (nodep->type()) {
        حالت VNType::Negate: newp = جدید AstNegateD{fl, lhsp}; بشکن;
        پیشفرض:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            بشکن;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    تهی replaceWithSFormat(AstMethodCall* nodep, ثابت string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ثابت argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_گره_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_گره* ثابت newp = جدید AstAssign{
            nodep->fileline(), fromp,
            جدید AstSFormatF{nodep->fileline(), format, نادرست, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        افزودن_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_گره_DType* iterateEditMoveDTypep(Ast_گره* parentp, Ast_گره_DType* dtnodep) {
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
        ثابت منطقی child1 = (parentp->getChildDTypep() == dtnodep);
        ثابت منطقی child2 = (parentp->getChild2DTypep() == dtnodep);
        اگر (child1 || child2) {
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
        اگر (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        بازگشت dtnodep;
    }

    AstConst* dimension_مقدار(File_سطر* fileline, Ast_گره_DType* nodep, VAttrType attrType,
                             صحیح dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_گره_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        برای (صحیح i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            اگر (ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(dtypep, گره_Array_DType)) {
                declRange = adtypep->declRange();
                اگر (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                ادامه;
            } وگرنه اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت adtypep
                       = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                بشکن;  // Sub elements don't look like arrays and can't iterate into
            } وگرنه اگر (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                اگر (adtypep->isRanged()) declRange = adtypep->declRange();
                بشکن;
            }
            بشکن;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        صحیح val = 0;
        انتخاب (attrType) {
        حالت VAttrType::DIM_BITS: {
            صحیح bits = 1;
            وقتی (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                اگر (ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(dtypep, گره_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    ادامه;
                } وگرنه اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت adtypep
                           = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    بشکن;
                } وگرنه اگر (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    بشکن;
                }
                بشکن;
            }
            اگر (dim == 0) {
                val = 0;
            } وگرنه اگر (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } وگرنه {
                val = bits;
            }
            بشکن;  // LCOV_EXCL_LINE
        }
        حالت VAttrType::DIM_بالا: val = !declRange.ranged() ? 0 : declRange.hi(); بشکن;
        حالت VAttrType::DIM_چپ: val = !declRange.ranged() ? 0 : declRange.چپ(); بشکن;
        حالت VAttrType::DIM_کم: val = !declRange.ranged() ? 0 : declRange.lo(); بشکن;
        حالت VAttrType::DIM_راست: val = !declRange.ranged() ? 0 : declRange.راست(); بشکن;
        حالت VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            بشکن;
        حالت VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); بشکن;
        پیشفرض: nodep->v3fatalSrc("Missing DIM ATTR type case"); بشکن;
        }
        اگر (!valp) valp = جدید AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        بازگشت valp;
    }
    AstVarRef* dimensionVarRefp(Ast_گره_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        اگر (cachedpr) بازگشت cachedpr->clone_درخت(نادرست);

        Ast_گره_Array_DType* ثابت vardtypep = جدید AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), جدید AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* ثابت consp = جدید AstConsArray{nodep->fileline(), vardtypep};
        برای (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_مقدار(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_درخت(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(درست);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(درست);
        افزودن_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        بازگشت cachedpr->clone_درخت(نادرست);
    }
    ایستا uint64_t enum_Max_مقدار(ثابت Ast_گره* errNodep, ثابت AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        برای (ثابت AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ثابت AstConst* ثابت vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            اگر (!vconstp->رقم().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        اگر (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            بازگشت 64;
        }
        بازگشت maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ثابت nodep, VAttrType attrType, منطقی assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        اگر (cachedpr) بازگشت cachedpr->clone_درخت(نادرست);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_DType* basep;
        اگر (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } وگرنه اگر (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } وگرنه {
            basep = nodep->dtypep();
        }
        Ast_گره_DType* vardtypep;
        اگر (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = جدید AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } وگرنه {
            vardtypep = جدید AstUnpackArrayDType{flp, basep, جدید AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        اگر (attrType == VAttrType::ENUM_NAME) {
            defaultp = جدید AstConst{flp, AstConst::String{}, ""};
        } وگرنه اگر (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = جدید AstConst{flp, V3_عدد{nodep, nodep->width(), 0}};
        } وگرنه اگر (attrType == VAttrType::ENUM_VALID) {
            defaultp = جدید AstConst{flp, AstConst::BitFalse{}};
        } وگرنه {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_گره_عبارت*> مقادیر;
        {
            uint64_t ordinal = 0;
            AstEnumItem* ثابت firstp = nodep->itemsp();
            ثابت AstEnumItem* prevp = firstp;  // Prev must start with last item
            وقتی (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            برای (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* ثابت nextp = VN_AS(itemp->nextp(), EnumItem);
                ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                اگر (!vconstp->رقم().isAnyXZ()) {  // Can 2-state runtime decode
                    ثابت uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    اگر (attrType == VAttrType::ENUM_NAME) {
                        مقادیر[i] = جدید AstConst{flp, AstConst::String{},
                                                 V3_عدد::displayedEnumName(itemp)};
                    } وگرنه اگر (attrType == VAttrType::ENUM_NEXT) {
                        مقادیر[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_درخت(نادرست);  // A const
                    } وگرنه اگر (attrType == VAttrType::ENUM_PREV) {
                        مقادیر[i] = prevp->valuep()->clone_درخت(نادرست);  // A const
                    } وگرنه اگر (attrType == VAttrType::ENUM_VALID) {
                        مقادیر[i] = جدید AstConst{flp, AstConst::BitTrue{}};
                    } وگرنه {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_گره_عبارت* valuep = nullptr;
        اگر (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ثابت consp = جدید AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            برای (ثابت auto& itr : مقادیر) {
                V3_عدد کلید{nodep, nodep->width()};
                کلید.setQuad(itr.first);
                consp->addItemsp(جدید AstConsAssocItem{flp, جدید AstConst{flp, کلید}, itr.second});
            }
            valuep = consp;
        } وگرنه {
            AstConsArray* ثابت consp = جدید AstConsArray{flp, vardtypep, defaultp};
            برای (uint64_t i = 0; i < (msbdim + 1); ++i) {
                اگر (مقادیر[i]) consp->valuep(i, مقادیر[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(درست);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(درست);
        افزودن_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        بازگشت cachedpr->clone_درخت(نادرست);
    }

    Ast_گره_عبارت* enumSelect(Ast_گره_عبارت* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        اگر (attrType == VAttrType::ENUM_NAME) {
            اگر (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                Ast_گره_عبارت* ثابت newp = جدید AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->رقم().displayedEnumName(adtypep)};
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت newp;
            }
        }
        اگر (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_عبارت_Stmt* ثابت capturep = newEnumCapture(nodep, adtypep);
            Ast_گره_عبارت* ثابت valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            بازگشت capturep;
        }
        اگر (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_گره_عبارت* indexp = جدید AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            برای (ثابت AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                اگر (constp->رقم().isAnyXZ()) ادامه;
                indexp = جدید AstCond{nodep->fileline(),
                                     جدید AstEq{nodep->fileline(), nodep->clone_درخت(نادرست),
                                               itemp->valuep()->clone_درخت(نادرست)},
                                     جدید AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, نادرست, ordinal);
            Ast_گره_عبارت* ثابت newp = جدید AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت newp;
        }
        ثابت uint64_t msbdim = enum_Max_مقدار(nodep, adtypep);
        ثابت منطقی assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_گره_عبارت* newp;
        اگر (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, درست, 0);
            newp = جدید AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } وگرنه {
            ثابت صحیح selwidth = V3_عدد::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(adtypep, attrType, نادرست, (1ULL << selwidth) - 1);
            Ast_گره_عبارت* ثابت boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? جدید AstGt{nodep->fileline(), nodep->clone_درخت(نادرست),
                                  جدید AstConst{nodep->fileline(),
                                               V3_عدد{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = جدید AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                جدید AstSel{nodep->fileline(), nodep, 0, selwidth}};
            اگر (boundp) {
                newp = جدید AstCond{nodep->fileline(), boundp,
                                   جدید AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        اگر (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } وگرنه اگر (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } وگرنه {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        بازگشت newp;
    }
    Ast_گره_عبارت* enumTestValid(Ast_گره_عبارت* valp, AstEnumDType* enumDtp) {
        ثابت uint64_t maxval = enum_Max_مقدار(valp, enumDtp);
        ثابت منطقی assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_گره_عبارت* testp = nullptr;
        File_سطر* ثابت fl_novalue = جدید File_سطر{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, درست);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, درست);
        اگر (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, درست, 0);
            testp = جدید AstAssocSel{fl_novalue, tabRefp, valp->clone_درخت_Pure(نادرست)};
        } وگرنه {
            ثابت صحیح selwidth = V3_عدد::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, نادرست, (1ULL << selwidth) - 1);
            File_سطر* ثابت fl_nowidth = جدید File_سطر{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, درست);
            testp = جدید AstCond{
                fl_novalue,
                جدید AstGt{fl_nowidth, valp->clone_درخت_Pure(نادرست),
                          جدید AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                جدید AstConst{fl_novalue, AstConst::BitFalse{}},
                جدید AstArraySel{fl_novalue, tabRefp,
                                جدید AstSel{fl_novalue, valp->clone_درخت_Pure(نادرست), 0, selwidth}}};
        }
        بازگشت testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, ثابت VNumRange& range) {
        PatVecMap patmap;
        صحیح element = range.چپ();
        برای (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            اگر (patp->keyp()) {
                اگر (ثابت AstConst* ثابت constp = patternConstKeyp(patp)) {
                    اگر (constp->رقم().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } وگرنه {
                        element = constp->toSInt();
                    }
                }
            }
            ثابت منطقی newEntry = patmap.emplace(element, patp).second;
            اگر (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.چپ_To_راست_Inc();
        }
        بازگشت patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    تهی patConcatConvertRecurse(AstPattern* patternp, Ast_گره_عبارت* nodep) {
        اگر (AstConcat* ثابت concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } وگرنه {
            AstPatMember* ثابت newp
                = جدید AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(درست);
            patternp->addItemsp(newp);
        }
    }

    تهی makeOpenArrayShell(Ast_گره_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_گره_FTask* ثابت oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_گره_FTask* ثابت newTaskp = oldTaskp->clone_درخت(نادرست);
        newTaskp->dpiOpenChild(درست);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        برای (ثابت auto& tconnect : tconnects) {
            AstVar* ثابت portp = tconnect.first;
            ثابت AstArg* ثابت argp = tconnect.second;
            ثابت Ast_گره* ثابت pinp = argp->exprp();
            اگر (!pinp) ادامه;  // Argument error we'll find later
            اگر (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_گره_DType* ثابت oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_گره* nodep) {
                    اگر (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    منطقی markHasOpenArray(Ast_گره_FTask* nodep) {
        منطقی hasOpen = نادرست;
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(درست);
                    hasOpen = درست;
                }
            }
        }
        بازگشت hasOpen;
    }
    منطقی hasOpenArrayDTypeRecurse(Ast_گره_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        اگر (VN_IS(nodep, UnsizedArrayDType)) بازگشت درست;
        اگر (nodep->subDTypep()) بازگشت hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        بازگشت نادرست;
    }
    Ast_گره_DType* dtypeNotIntAtomOrVecRecurse(Ast_گره_DType* nodep, منطقی ranged = نادرست) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        اگر (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            اگر (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                بازگشت dtp;  // Packed when already packed
            اگر (dtp->keyword().isIntNumeric()) بازگشت nullptr;
            بازگشت dtp;
        } وگرنه اگر (AstPackArrayDType* ثابت dtp = VN_CAST(nodep, PackArrayDType)) {
            اگر (ranged) بازگشت dtp;  // Packed when already packed
            بازگشت dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), درست);
        }
        بازگشت nodep;
    }
    Ast_گره_DType* dtypeNot4StateIntegralRecurse(Ast_گره_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        اگر (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            اگر (!dtp->keyword().isFourstate()) بازگشت dtp;
            بازگشت nullptr;
        } وگرنه اگر (Ast_گره_Array_DType* ثابت dtp = VN_CAST(nodep, گره_Array_DType)) {
            بازگشت dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } وگرنه اگر (Ast_گره_UOr_Struct_DType* ثابت dtp = VN_CAST(nodep, گره_UOr_Struct_DType)) {
            برای (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_گره_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                اگر (badDtp) بازگشت badDtp;
            }
            بازگشت nullptr;
        }
        بازگشت nodep;
    }
    تهی checkForceReleaseLhs(Ast_گره* nodep, Ast_گره* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        اگر (Ast_گره* ثابت selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    تهی assert_At_عبارت(Ast_گره* nodep) {
        اگر (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    تهی assertAtStatement(Ast_گره* nodep) {
        اگر (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    تهی checkConstantOrReplace(Ast_گره* nodep, منطقی noFourState, ثابت string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        اگر (!constp || (noFourState && constp->رقم().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(افزودن_Deletep(nodep), nodep, constp);
            بازگشت;
        }
    }
    Ast_گره* گره_For_Unsized_Warning(Ast_گره* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        اگر (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            بازگشت nodep->op1p();
        } وگرنه اگر (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            بازگشت nodep->op2p();
        }
        بازگشت nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_گره* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        اگر (VL_LIKELY(!nodep)) بازگشت nullptr;
        اگر (auto* ثابت refp = VN_CAST(nodep, RefDType)) {
            اگر (refp->typedefp() == typedefp) بازگشت refp;
        }
        اگر (auto* ثابت refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) بازگشت refp;
        اگر (auto* ثابت refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) بازگشت refp;
        اگر (auto* ثابت refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) بازگشت refp;
        اگر (auto* ثابت refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) بازگشت refp;
        بازگشت nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    ایستا منطقی isIfaceArrayDType(ثابت Ast_گره_DType* dtypep) {
        اگر (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) بازگشت نادرست;
        ثابت AstIfaceRefDType* ثابت irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        بازگشت irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    ایستا std::string unpackedArraySize(ثابت Ast_گره_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        وقتی (ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            اگر (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        بازگشت size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    ایستا Ast_گره_عبارت* newIfaceArrayElement(Ast_گره_عبارت* fromp, صحیح bit) {
        AstConsArray* ثابت consp = VN_CAST(fromp, ConsArray);
        اگر (!consp || !isIfaceArrayDType(consp->dtypep())) بازگشت nullptr;
        ثابت AstUnpackArrayDType* ثابت arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (bit < 0 || bit >= arrp->elementsConst()) بازگشت nullptr;
        بازگشت consp->valuep(bit)->clone_درخت(نادرست);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_گره_عبارت* new_Iface_Array_مقدار(ثابت Ast_گره_Var_Ref* refp, Ast_گره_DType* dtypep,
                                    ثابت std::string& name, ثابت std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_سطر* ثابت flp = refp->fileline();
        AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        اگر (!arrp) {
            AstVar* ثابت varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_گره_Var_Ref* newp;
            اگر (ثابت AstVarXRef* ثابت xrefp = VN_CAST(refp, VarXRef)) {
                newp = جدید AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } وگرنه {
                newp = جدید AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            بازگشت newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ثابت consp = جدید AstConsArray{flp, arrp};
        ثابت VNumRange range = arrp->declRange();
        برای (ثابت صحیح i : range.seq_چپ_To_راست()) {
            ثابت std::string s = name + "__BRA__" + Ast_گره::encode_عدد(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_مقدار(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        بازگشت consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    تهی replaceSplitIfaceArrayRef(Ast_گره_Var_Ref* nodep) {
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        اگر (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_مقدار(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    تهی expandIfaceArrayPinDimensions(AstPin* nodep, ثابت Ast_گره_DType* dtypep,
                                       Ast_گره_عبارت* conp, ثابت std::string& suffix,
                                       AstVar*& elemVarpr) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        اگر (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ثابت newp = nodep->clone_درخت(نادرست);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            اگر (conp) newp->exprp(conp->clone_درخت(نادرست));
            nodep->addNextHere(newp);
            بازگشت;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ثابت VNumRange range = arrp->declRange();
        برای (صحیح n = range.elements() - 1; n >= 0; --n) {
            ثابت صحیح i = range.چپ() + n * range.چپ_To_راست_Inc();
            ثابت std::string s = suffix + "__BRA__" + Ast_گره::encode_عدد(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_گره_عبارت* ثابت elemConnp = [&]() -> Ast_گره_عبارت* {
                اگر (!conp) بازگشت nullptr;
                ثابت AstUnpackArrayDType* ثابت conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ثابت صحیح elements = conArrp->elementsConst();
                // Counted from lo
                Ast_گره_عبارت* ثابت resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                بازگشت resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    تهی expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ثابت portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        برای (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_گره_عبارت* ثابت exprp = VN_AS(nodep->exprp(), گره_عبارت);
        اگر (exprp) افزودن_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ثابت منطقی connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_گره* userIterateSubtreeReturnEdits(Ast_گره* nodep, WidthVP* vup) {
        اگر (!nodep) بازگشت nullptr;
        Ast_گره* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        بازگشت ret;
    }
    تهی userIterate(Ast_گره* nodep, WidthVP* vup) {
        اگر (!nodep) بازگشت;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    تهی userIterateAndNext(Ast_گره* nodep, WidthVP* vup) {
        اگر (!nodep) بازگشت;
        اگر (nodep->didWidth()) بازگشت;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    تهی userIterateChildren(Ast_گره* nodep, WidthVP* vup) {
        اگر (!nodep) بازگشت;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    تهی userIterateChildrenBackwardsConst(Ast_گره* nodep, WidthVP* vup) {
        اگر (!nodep) بازگشت;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

عمومی:
    // CONSTRUCTORS
    WidthVisitor(منطقی paramsOnly,  // [in] TRUE if we are considering parameters only.
                 منطقی doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_گره* mainAcceptEdit(Ast_گره* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        برای (AstVar* ثابت varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(افزودن_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        بازگشت nodep;
    }
    ایستا منطقی lowerAsFixedAggregate(ثابت Ast_گره_DType* ثابت dtypep) {
        بازگشت dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    تهی streamAssignLower(Ast_گره_Assign* nodep) {
        Ast_گره_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        اگر (Ast_گره_Stream* ثابت streamp = VN_CAST(nodep->rhsp(), گره_Stream)) {
            Ast_گره_DType* ثابت lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ثابت صحیح lwidth = lhsDTypeSkippedRefp->widthStream();
            ثابت صحیح rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            اگر (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            اگر (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                ثابت صحیح queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            اگر (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    جدید AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        اگر (Ast_گره_Stream* ثابت streamp = VN_CAST(nodep->lhsp(), گره_Stream)) {
            ثابت Ast_گره_DType* ثابت rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_گره_DType* ثابت lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            ثابت صحیح lwidth = lhsStreamDTypep->widthStream();
            ثابت صحیح rwidth = rhsDTypep->widthStream();
            اگر (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            اگر (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_گره_عبارت* ثابت streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_گره_عبارت* ثابت dstp = streamp->lhsp()->unlinkFrBack();
                Ast_گره_عبارت* srcp = nodep->rhsp()->unlinkFrBack();
                اگر (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_گره_عبارت::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } وگرنه {
                    اگر (srcp->width() > lwidth) {
                        srcp = جدید AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(افزودن_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(جدید AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } وگرنه اگر (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(جدید AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = پیشفرض;
};

//######################################################################
// Width class functions

تهی V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ثابت WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ثابت VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{نادرست, نادرست};
        (تهی)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (تهی)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("width", 0, dump_درخت_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_گره* V3Width::widthParamsEdit(Ast_گره* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{درست, نادرست};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    بازگشت nodep;
}

تهی V3Width::streamAssignLowerEdit(Ast_گره_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{نادرست, نادرست};
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
Ast_گره* V3Width::widthGenerateParamsEdit(
    Ast_گره* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{درست, درست};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    بازگشت nodep;
}

Ast_گره* V3Width::selectNonConstantRecurse(Ast_گره* nodep, منطقی inSel) {
    // If node has a non-constant select, return that select
    Ast_گره* resultp = nullptr;
    اگر (Ast_گره_Sel* ثابت anodep = VN_CAST(nodep, گره_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        اگر (resultp) بازگشت resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), درست);
    } وگرنه اگر (AstSel* ثابت anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        اگر (resultp) بازگشت resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), درست);
    } وگرنه اگر (Ast_گره_Var_Ref* ثابت anodep = VN_CAST(nodep, گره_Var_Ref)) {
        اگر (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) بازگشت anodep;
    } وگرنه {
        اگر (Ast_گره* ثابت refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        اگر (resultp) بازگشت resultp;
        اگر (Ast_گره* ثابت refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        اگر (resultp) بازگشت resultp;
        اگر (Ast_گره* ثابت refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        اگر (resultp) بازگشت resultp;
        اگر (Ast_گره* ثابت refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    بازگشت resultp;
}
