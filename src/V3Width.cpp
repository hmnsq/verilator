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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Width.h"

#অন্তর্ভাব "V3Assert.h"
#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Begin.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3ConstPool.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3LinkLValue.h"
#অন্তর্ভাব "V3MemberMap.h"
#অন্তর্ভাব "V3Number.h"
#অন্তর্ভাব "V3Randomize.h"
#অন্তর্ভাব "V3String.h"
#অন্তর্ভাব "V3Task.h"
#অন্তর্ভাব "V3UniqueNames.h"
#অন্তর্ভাব "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#অন্তর্ভাব "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্রেণী ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ধ্রুবক Ast_গ্রন্থি*, AstClass*> m_cache;

    AstClass* findCached(Ast_গ্রন্থি* nodep) {
        যদি (!nodep) ফেরত nullptr;
        ধ্রুবক auto it = m_cache.find(nodep);
        যদি (it != m_cache.end()) ফেরত it->second;

        AstClass* classp = nullptr;
        যদি (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } অন্যথা যদি (AstClass* ধ্রুবক parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } অন্যথা যদি (AstClassPackage* ধ্রুবক packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } অন্যথা {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        ফেরত classp;
    }

সর্বজনীন:
    AstClass* find(Ast_গ্রন্থি* nodep) {
        যদি (AstClass* ধ্রুবক classp = VN_CAST(nodep, Class)) ফেরত classp;
        যদি (AstClassPackage* ধ্রুবক packagep = VN_CAST(nodep, ClassPackage)) {
            ফেরত packagep->classp();
        }
        ফেরত findCached(nodep);
    }
};

//######################################################################

গণনা Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, ধ্রুবক Stage& rhs) {
    ফেরত str << ("-PFB"[static_cast<পূর্ণসংখ্যা>(rhs)]);
}

গণনা Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, ধ্রুবক Determ& rhs) {
    স্থির constexpr ধ্রুবক অক্ষর* ধ্রুবক s_det[] = {"SELF", "CNTX", "ASSN"};
    ফেরত str << s_det[rhs];
}

গণনা StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, বার্তা) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               বার্তা)

//######################################################################
// Width state, as a visitor of each AstNode

শ্রেণী WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_গ্রন্থি_DType* ধ্রুবক m_dtypep;  // Parent's data type to resolve to
    ধ্রুবক Stage m_stage;  // If true, report errors
    ধ্রুবক StreamUse m_streamUse;  // Current expression may be a stream concat

সর্বজনীন:
    WidthVP(Ast_গ্রন্থি_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        যদি (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { ফেরত এই; }
    যৌক্তিক selfDtm() ধ্রুবক { ফেরত m_dtypep == nullptr; }
    StreamUse streamUse() ধ্রুবক { ফেরত m_streamUse; }
    Ast_গ্রন্থি_DType* dtypep() ধ্রুবক {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        ফেরত m_dtypep;
    }
    Ast_গ্রন্থি_DType* dtypeNullp() ধ্রুবক { ফেরত m_dtypep; }
    Ast_গ্রন্থি_DType* dtypeNullSkipRefp() ধ্রুবক {
        Ast_গ্রন্থি_DType* dtp = dtypeNullp();
        যদি (dtp) dtp = dtp->skipRefp();
        ফেরত dtp;
    }
    Ast_গ্রন্থি_DType* dtypeOverridep(Ast_গ্রন্থি_DType* ধ্রুবক defaultp) ধ্রুবক {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        যদি (!m_dtypep) ফেরত defaultp;
        যদি (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            ফেরত defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        ফেরত m_dtypep;
    }
    পূর্ণসংখ্যা width() ধ্রুবক {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ফেরত m_dtypep->width();
    }
    পূর্ণসংখ্যা widthMin() ধ্রুবক {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ফেরত m_dtypep->widthMin();
    }
    যৌক্তিক prelim() ধ্রুবক { ফেরত m_stage & PRELIM; }
    যৌক্তিক final() ধ্রুবক { ফেরত m_stage & FINAL; }
    শূন্য dump(std::ostream& str) ধ্রুবক {
        যদি (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } অন্যথা {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, ধ্রুবক WidthVP* vup) {
    যদি (vup) vup->dump(str);
    ফেরত str;
}

//######################################################################

শ্রেণী WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    শূন্য clearWidthRecurse(Ast_গ্রন্থি* nodep) {
        জন্য (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(মিথ্যা);
            যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op1p()) clearWidthRecurse(refp);
            যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op2p()) clearWidthRecurse(refp);
            যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op3p()) clearWidthRecurse(refp);
            যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

সর্বজনীন:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    আভাসী ~WidthClearVisitor() = পূর্বনির্ধারিত;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_গ্রন্থি_iterate_instead_of_Ast_গ্রন্থি_accept

//######################################################################

শ্রেণী WidthVisitor final : সর্বজনীন VNVisitor {
    // TYPES
    using PatVecMap = std::map<পূর্ণসংখ্যা, AstPatMember*>;
    using DTypeMap = std::map<ধ্রুবক std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enum_অস্থায়ী_Names;  // Captured enum expressions
    V3UniqueNames m_inside_অস্থায়ী_Names;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ধ্রুবক AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    যৌক্তিক m_underFork = মিথ্যা;  // Visiting under a fork
    যৌক্তিক m_underSExpr = মিথ্যা;  // Visiting under a sequence expression
    যৌক্তিক m_underPackedArray = মিথ্যা;  // Visiting under a AstPackArrayDType
    যৌক্তিক m_underMemberSel = মিথ্যা;  // Viting under a MemberSel
    Ast_গ্রন্থি* m_seqUnsupp = nullptr;  // Property has unsupported node
    যৌক্তিক m_hasSExpr = মিথ্যা;  // Property has a sequence expression
    ধ্রুবক AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    ধ্রুবক AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_গ্রন্থি_Module* m_modep = nullptr;  // Current module
    ধ্রুবক AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_গ্রন্থি_Procedure* m_procedurep = nullptr;  // Current final/always
    ধ্রুবক AstWith* m_withp = nullptr;  // Current 'with' statement
    ধ্রুবক AstFunc* m_funcp = nullptr;  // Current function
    ধ্রুবক AstAttrOf* m_attrp = nullptr;  // Current attribute
    ধ্রুবক Ast_গ্রন্থি_রাশিমালা* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    ধ্রুবক যৌক্তিক m_paramsOnly;  // Computing parameter value; limit operation
    ধ্রুবক যৌক্তিক m_doGenerate;  // Do errors later inside generate statement
    যৌক্তিক m_streamConcat = মিথ্যা;  // True if visiting arguments of stream concatenation
    যৌক্তিক m_inParamOverride = মিথ্যা;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ধ্রুবক Ast_গ্রন্থি_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<ধ্রুবক Ast_গ্রন্থি_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<ধ্রুবক AstVar*> m_curModVars;  // Variables declared in current module

    স্থির constexpr পূর্ণসংখ্যা ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    গণনা ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    গণনা শ্রেণী ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    স্থির StreamUse streamUseForDeterm(Determ determ) {
        ফেরত determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() ধ্রুবক { ফেরত m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    স্থির শূন্য packIfUnpacked(Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nodep) {
        যদি (AstUnpackArrayDType* ধ্রুবক unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ধ্রুবক পূর্ণসংখ্যা elements_অঙ্ক = unpackDTypep->arrayUnpackedElements();
            ধ্রুবক পূর্ণসংখ্যা unpackMinBits = elements_অঙ্ক * unpackDTypep->subDTypep()->widthMin();
            ধ্রুবক পূর্ণসংখ্যা unpackBits = elements_অঙ্ক * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(নতুন AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    স্থির Ast_গ্রন্থি_DType* fromDTypep(Ast_গ্রন্থি* fromp) {
        যদি (Ast_গ্রন্থি_DType* ধ্রুবক dtypep = VN_CAST(fromp, গ্রন্থি_DType))
            ফেরত dtypep->skipRefOrNullp();
        ফেরত fromp ? fromp->dtypep() : nullptr;
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
    শূন্য visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    শূন্য visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    শূন্য visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    শূন্য visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    শূন্য visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    শূন্য visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    শূন্য visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    শূন্য visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    শূন্য visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    শূন্য visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    শূন্য visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    শূন্য visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    শূন্য visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    শূন্য visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    শূন্য visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    শূন্য visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    শূন্য visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    শূন্য visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    শূন্য visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    শূন্য visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, সত্য, মিথ্যা); }
    শূন্য visit(AstSub* nodep) override { visit_add_sub_replace(nodep, সত্য, মিথ্যা); }
    শূন্য visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, সত্য, সত্য); }
    শূন্য visit(AstMul* nodep) override { visit_add_sub_replace(nodep, সত্য, মিথ্যা); }
    // These can't promote to real
    শূন্য visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, মিথ্যা, সত্য); }
    শূন্য visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, মিথ্যা, সত্য); }
    শূন্য visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, মিথ্যা, মিথ্যা); }
    শূন্য visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, মিথ্যা, সত্য); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    শূন্য visit(AstNegate* nodep) override { visit_negate_not(nodep, সত্য); }
    // Unary never real
    শূন্য visit(AstNot* nodep) override { visit_negate_not(nodep, মিথ্যা); }

    // Real: inputs and output real
    শূন্য visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(Ast_গ্রন্থি_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    শূন্য visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    শূন্য visit(Ast_গ্রন্থি_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    শূন্য visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    শূন্য visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    শূন্য visit(AstShiftL* nodep) override { visit_shift(nodep); }
    শূন্য visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    শূন্য visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    শূন্য visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    শূন্য visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    শূন্য visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    শূন্য visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    শূন্য visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    শূন্য visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        যদি (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    শূন্য visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        যদি (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    শূন্য visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        যদি (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    শূন্য visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        যদি (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    শূন্য visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    শূন্য visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            যদি (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } অন্যথা {
                nodep->dtypeSetInteger();
            }
        }
    }
    শূন্য visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    শূন্য visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    শূন্য visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    শূন্য visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    শূন্য visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    শূন্য visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            নতুন AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    স্থির যৌক্তিক isOverridePattern(ধ্রুবক Ast_গ্রন্থি* nodep) {
        যদি (ধ্রুবক AstPattern* ধ্রুবক patternp = VN_CAST(nodep, Pattern)) {
            ফেরত !patternp->childDTypep();
        }
        ধ্রুবক AstCond* ধ্রুবক condp = VN_CAST(nodep, Cond);
        ফেরত condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    শূন্য visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_রাশিমালা(nodep);
        যদি (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ধ্রুবক পূর্ণসংখ্যা errors = V3Error::error_গুনতি();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = মিথ্যা;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->condp(), Const);
            ধ্রুবক যৌক্তিক known = constp && !constp->অঙ্ক().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            যদি (!known && V3Error::error_গুনতি() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            ফেরত;
        }
        যদি (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            user_Iterate_And_পরবর্তী(nodep->thenp(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            user_Iterate_And_পরবর্তী(nodep->elsep(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_গ্রন্থি_DType* ধ্রুবক thenDTypep = nodep->thenp()->dtypep();
            Ast_গ্রন্থি_DType* ধ্রুবক elseDTypep = nodep->elsep()->dtypep();
            Ast_গ্রন্থি_DType* ধ্রুবক thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_গ্রন্থি_DType* ধ্রুবক elseDTypeSkippedp = elseDTypep->skipRefp();
            যদি (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } অন্যথা যদি (nodep->thenp()->is_Class_Handle_মান()
                       || nodep->elsep()->is_Class_Handle_মান()) {
                Ast_গ্রন্থি_DType* commonClassTypep = nullptr;
                যদি (nodep->thenp()->is_Class_Handle_মান() && nodep->elsep()->is_Class_Handle_মান()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_গ্রন্থি::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                যদি (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } অন্যথা {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } অন্যথা যদি (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                যদি (Ast_গ্রন্থি::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } অন্যথা {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    যদি (!Ast_গ্রন্থি::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } অন্যথা যদি (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } অন্যথা যদি (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } অন্যথা {
                ধ্রুবক যৌক্তিক is_Any_ফলাফল_Fourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                যৌক্তিক handled = মিথ্যা;
                ধ্রুবক যৌক্তিক isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_গ্রন্থি_DType* ধ্রুবক thenEnumDTypep = thenDTypep->skipRefToEnump();
                যদি (VN_IS(thenDTypeSkippedp, গ্রন্থি_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_বৃক্ষ(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = সত্য;
                } অন্যথা যদি (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_বৃক্ষ(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    যদি (is_Any_ফলাফল_Fourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = সত্য;
                    } অন্যথা {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(নতুন File_সারি{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, সত্য);
                    }
                }
                যদি (!handled) {
                    ধ্রুবক পূর্ণসংখ্যা width = std::max(thenDTypep->width(), elseDTypep->width());
                    ধ্রুবক পূর্ণসংখ্যা widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    ধ্রুবক VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ধ্রুবক যৌক্তিক isFourstate = is_Any_ফলাফল_Fourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    যদি ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } অন্যথা যদি ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } অন্যথা {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        যদি (m_vup->final()) {
            Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    শূন্য visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_গ্রন্থি_DType* ধ্রুবক vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }
        যদি (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* ধ্রুবক newp = নতুন AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ফেরত;
        }

        // Concat handling
        যদি (m_vup->prelim()) {
            যদি (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            যদি (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } অন্যথা {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                যদি (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_গ্রন্থি_রাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            যদি (ধ্রুবক AstReplicate* ধ্রুবক repp = VN_CAST(nodep->lhsp(), Replicate)) {
                যদি (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত;
                }
            }
            যদি (ধ্রুবক AstReplicate* ধ্রুবক repp = VN_CAST(nodep->rhsp(), Replicate)) {
                যদি (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত;
                }
            }
        }
        যদি (m_vup->final()) {
            যদি (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_গ্রন্থি_রাশিমালা* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* rhsp = nodep->rhsp()->unlinkFrBack();
                যদি (!lhsp->isString()) lhsp = নতুন AstCvtPackString{lhsp->fileline(), lhsp};
                যদি (!rhsp->isString()) rhsp = নতুন AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            }
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্রন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    শূন্য visit(AstConcatN* nodep) override {
        যদি (nodep->didWidth()) ফেরত;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        যদি (m_vup->final()) {
            nodep->didWidth(সত্য);
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্রন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    শূন্য visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    শূন্য visit(AstDelay* nodep) override {
        যদি (nodep->isCycleDelay() && m_underSExpr) {
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            যদি (nodep->rhsp() && !nodep->isUnbounded()) {
                user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            যদি (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                ফেরত;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            ফেরত;
        }
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            ফেরত;
        }
        যদি (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        }
        যদি (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        }
        যদি (nodep->fileline()->timingOn()) {
            যদি (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterate_And_পরবর্তী_Null(nodep->stmtsp());
                ফেরত;
            } অন্যথা যদি (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } অন্যথা {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        যদি (nodep->stmtsp()) nodep->add_পরবর্তী_Here(nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        }
        যদি (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ধ্রুবক newp = নতুন AstBegin{nodep->fileline(), nodep->name(), nullptr, মিথ্যা};
            nodep->replaceWith(newp);
            যদি (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlink_Fr_Back_With_পরবর্তী());
            যদি (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
            যদি (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlink_Fr_Back_With_পরবর্তী());
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = সত্য;
            iterateChildren(nodep);
        } অন্যথা যদি (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        } অন্যথা {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    শূন্য visitWaitOrDisableFork(Ast_গ্রন্থি* nodep) {
        যদি (nodep->fileline()->timingOn()) {
            যদি (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                ফেরত;
            } অন্যথা যদি (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    শূন্য visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    শূন্য visit(AstToLowerN* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    শূন্য visit(AstToUpperN* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    শূন্য visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->countp(), Const);
            যদি (constp) {
                যদি (constp->অঙ্ক().isFourState()
                    || (constp->dtypep()->isSigned() && constp->অঙ্ক().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } অন্যথা {
                    times = constp->toSInt();
                }
            }

            Ast_গ্রন্থি_DType* ধ্রুবক vdtypep = m_vup->dtypeNullSkipRefp();
            যদি (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                যদি (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                যদি (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    user_Iterate_And_পরবর্তী(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত;
                } অন্যথা {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* ধ্রুবক newp = নতুন AstPattern{
                        nodep->fileline(),
                        নতুন AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ফেরত;
                }
            }
            যদি (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            যদি (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } অন্যথা {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            যদি ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_গ্রন্থি* ধ্রুবক newp
                    = নতুন AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            } অন্যথা {
                যদি (!constp) nodep->v3error("Replication value isn't a constant.");
                যদি (times == 0
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
        যদি (m_vup->final()) {
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্রন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    শূন্য visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        যদি (m_vup->final()) {
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্রন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Dist_Biop* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Dist_Triop* nodep) override {
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    যৌক্তিক streamImplicitUseAllowed(ধ্রুবক Ast_গ্রন্থি_Stream* nodep) ধ্রুবক {
        যদি (m_streamConcat) ফেরত সত্য;
        যদি (m_vup && m_vup->streamUse() != STREAM_USE_NONE) ফেরত সত্য;

        ধ্রুবক Ast_গ্রন্থি* ধ্রুবক backp = nodep->backp();
        যদি (!backp) ফেরত সত্য;  // Error elsewhere

        ফেরত VN_IS(backp, গ্রন্থি_Assign);
    }
    শূন্য visit(Ast_গ্রন্থি_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            m_streamConcat = সত্য;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = মিথ্যা;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->rhsp(), Const)) {
                যদি (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } অন্যথা {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            যদি (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, গ্রন্থি_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, গ্রন্থি_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } অন্যথা যদি (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } অন্যথা {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_গ্রন্থি_রাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        যদি (m_vup->final()) {
            যদি (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্রন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    শূন্য visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), সত্য,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), সত্য,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        যদি (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ধ্রুবক পূর্ণসংখ্যা width = nodep->elementsConst();
            যদি (nodep->fromBracket() && nodep->বাম_Const() > nodep->ডান_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } অন্যথা যদি (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ধ্রুবক যৌক্তিক inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ধ্রুবক যৌক্তিক inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ধ্রুবক যৌক্তিক inTypeTable = !m_modep;
            ধ্রুবক Ast_গ্রন্থি* basep = nodep->backp();
            যখন (VN_IS(basep, Range)) basep = basep->backp();
            যদি (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->বাম_Const() << ":" << nodep->ডান_Const()
                                            << "]");
            }
        }
    }

    শূন্য visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        যদি (nodep->didWidth()) ফেরত;
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_পরবর্তী(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), মিথ্যা);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            পূর্ণসংখ্যা width = nodep->widthConst();
            যদি (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                ফেরত;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ধ্রুবক যৌক্তিক isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ধ্রুবক যৌক্তিক inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            যদি (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                ঠেলো_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(নতুন AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            যদি (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                যদি (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    মিথ্যা /*noerror*/);
                } অন্যথা {
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
            পূর্ণসংখ্যা frommsb = nodep->fromp()->width() - 1;
            পূর্ণসংখ্যা fromlsb = 0;
            ধ্রুবক পূর্ণসংখ্যা elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            যদি (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } অন্যথা {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ধ্রুবক পূর্ণসংখ্যা selwidth = V3_সংখ্যা::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_গ্রন্থি_DType* ধ্রুবক selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            user_Iterate_And_পরবর্তী(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            যদি (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                যদি (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            যদি (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                যদি (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } অন্যথা যদি (!inParameterizedTemplate) {
                    যদি (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } অন্যথা {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                যদি (!isWriteSelect) {
                    // Extend it
                    ধ্রুবক পূর্ণসংখ্যা extendTo = nodep->msbConst() + 1;
                    Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    মিথ্যা /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            যদি (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                মিথ্যা /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    শূন্য visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            যদি (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->bitp(), Const)) {
                    ধ্রুবক পূর্ণসংখ্যা bit = constp->toSInt();
                    Ast_গ্রন্থি_রাশিমালা* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    যদি (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত;
                }
            }
            //
            পূর্ণসংখ্যা frommsb;
            পূর্ণসংখ্যা fromlsb;
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক fromDtp = nodep->fromp()->dtypep()->skipRefp();
            যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                যদি (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                যদি (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } অন্যথা {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ধ্রুবক পূর্ণসংখ্যা selwidth = V3_সংখ্যা::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_গ্রন্থি_DType* ধ্রুবক selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            যদি (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                যদি (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            যদি (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                যদি (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    যদি (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                মিথ্যা /*NOWARN*/);
            }
        }
    }

    শূন্য visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ধ্রুবক AstAssocArrayDType* ধ্রুবক adtypep = VN_CAST(fromDtp, AssocArrayDType);
            যদি (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->চাবি_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    শূন্য visit(AstAlias* nodep) override {
        যদি (!nodep->didWidthAndSet()) {
            user_Iterate_And_পরবর্তী(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        ধ্রুবক auto check_If_রাশিমালা_Ok = [এই](ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp) {
            যদি (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                ফেরত মিথ্যা;
            }

            ধ্রুবক AstVarRef* ধ্রুবক varRefp = VN_CAST(exprp, VarRef);
            যদি (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                ফেরত মিথ্যা;
            }
            AstVar* ধ্রুবক varp = varRefp->varp();
            যদি (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                ফেরত মিথ্যা;
            }
            যদি (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                ফেরত মিথ্যা;
            } অন্যথা {
                m_aliasedVars.ঢোকাও(varp);
            }
            ফেরত সত্য;
        };

        check_If_রাশিমালা_Ok(nodep->itemsp());
        Ast_গ্রন্থি_DType* ধ্রুবক firstItemDtypep = nodep->itemsp()->dtypep();
        জন্য (Ast_গ্রন্থি* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_রাশিমালা_Ok(VN_AS(itemp, গ্রন্থি_রাশিমালা));
            যদি (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    শূন্য visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ধ্রুবক AstWildcardArrayDType* ধ্রুবক adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            যদি (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ধ্রুবক AstBasicDType* ধ্রুবক basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            যদি (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    শূন্য visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            যদি (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_গ্রন্থি_DType* ধ্রুবক newDtp
                = নতুন AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          নতুন AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            যদি (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ধ্রুবক পূর্ণসংখ্যা subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ধ্রুবক VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                যদি ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(নতুন AstConsArray{
                        nodep->fileline(), newDtp,
                        নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত;
                } অন্যথা যদি ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            যদি (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ধ্রুবক fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ধ্রুবক newp = নতুন AstConsArray{nodep->fileline(), newDtp};
                ধ্রুবক VNumRange range = nodep->declRange();
                জন্য (পূর্ণসংখ্যা n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_বৃক্ষ(মিথ্যা));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        }
    }

    শূন্য visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্রন্থি* ধ্রুবক selp = V3Width::widthSelNoIterEdit(nodep);
        যদি (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            ফেরত;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    শূন্য visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্রন্থি* ধ্রুবক selp = V3Width::widthSelNoIterEdit(nodep);
        যদি (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ফেরত;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    শূন্য visit(AstSelPlus* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্রন্থি* ধ্রুবক selp = V3Width::widthSelNoIterEdit(nodep);
        যদি (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ফেরত;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    শূন্য visit(AstSelMinus* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_পরবর্তী(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্রন্থি* ধ্রুবক selp = V3Width::widthSelNoIterEdit(nodep);
        যদি (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ফেরত;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    শূন্য visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        যদি (nodep->didWidthAndSet()) ফেরত;
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        যদি (nodep->didWidthAndSet()) ফেরত;
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        যদি (nodep->didWidthAndSet()) ফেরত;
        যদি (m_vup && m_vup->prelim()) {
            যদি (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } অন্যথা যদি (nodep->অঙ্ক().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } অন্যথা যদি (nodep->অঙ্ক().isString()) {
                nodep->dtypeSetString();
            } অন্যথা যদি (nodep->অঙ্ক().isDouble()) {
                nodep->dtypeChgWidth(nodep->অঙ্ক().width(), nodep->অঙ্ক().sized()
                                                               ? nodep->অঙ্ক().width()
                                                               : nodep->অঙ্ক().widthToFit());
            } অন্যথা {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->অঙ্ক().width(),
                    nodep->অঙ্ক().sized() ? nodep->অঙ্ক().width() : nodep->অঙ্ক().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->অঙ্ক().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    শূন্য visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        যদি (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    শূন্য visit(AstFell* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সত্য);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstFalling* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সত্য);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstFuture* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সত্য);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    শূন্য visit(AstPast* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সত্য);
            nodep->dtypeFrom(nodep->exprp());
            যদি (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ সত্য);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->ticksp(), Const);
                যদি (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_বৃক্ষ();
                } অন্যথা যদি (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_বৃক্ষ();
                } অন্যথা যদি (V3AssertCommon::check_Sva_মান_Limit(nodep, constp, "$past tick value")) {
                    যদি (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    শূন্য visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_পরবর্তী(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ধ্রুবক AstConst* ধ্রুবক minConstp = VN_CAST(nodep->countp(), Const);
            যদি (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } অন্যথা যদি (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } অন্যথা যদি (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            যদি (nodep->maxCountp()) {
                user_Iterate_And_পরবর্তী(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ধ্রুবক AstConst* ধ্রুবক maxConstp = VN_CAST(nodep->maxCountp(), Const);
                যদি (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } অন্যথা যদি (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } অন্যথা যদি (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_গ্রন্থি_রাশিমালা* propp = nodep->propp();
            যদি (!VN_IS(nodep->loBoundp(), Unbounded)) {
                user_Iterate_And_পরবর্তী(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            যদি (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                user_Iterate_And_পরবর্তী(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ধ্রুবক যৌক্তিক loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ধ্রুবক যৌক্তিক hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            যদি (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_গ্রন্থি* ধ্রুবক boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ফেরত;
            }
            যদি (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ফেরত;
            }
            ধ্রুবক AstConst* ধ্রুবক loConstp = VN_CAST(nodep->loBoundp(), Const);
            ধ্রুবক AstConst* ধ্রুবক hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            যদি (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            যদি (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            যদি (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } অন্যথা যদি (loConstp) {
                V3AssertCommon::check_Sva_মান_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            যদি (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_মান_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            যদি (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            যৌক্তিক hasPropertyOp = propp->isMultiCycleSva();
            যদি (!hasPropertyOp) {
                propp->foreach([&](ধ্রুবক Ast_গ্রন্থি* np) {
                    যদি (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = সত্য;
                    }
                });
            }
            যদি (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstRising* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সত্য);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstRose* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সত্য);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    শূন্য visit(AstSampled* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সত্য);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    শূন্য visit(AstSetuphold* nodep) override {
        File_সারি* ধ্রুবক flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        যদি (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        যদি (nodep->deldatap()) {
            যদি (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } অন্যথা {
                newp->add_পরবর্তী_Here(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        যদি (!newp) {
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        }
        nodep->replaceWith(নতুন AstAlways{newp});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_সারি* ধ্রুবক flp, Ast_গ্রন্থি_রাশিমালা* ধ্রুবক evp,
                                         Ast_গ্রন্থি_রাশিমালা* ধ্রুবক delp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = delp->clone_বৃক্ষ_Pure(মিথ্যা);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = evp->clone_বৃক্ষ_Pure(মিথ্যা);
        UASSERT_OBJ(VN_IS(lhsp, গ্রন্থি_Var_Ref) || VN_IS(lhsp, গ্রন্থি_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        যদি (Ast_গ্রন্থি_Var_Ref* varRefp = VN_CAST(lhsp, গ্রন্থি_Var_Ref)) {
            যদি (varRefp->varp()->direction() == VDirection::প্রবেশ) {
                VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
                ফেরত nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        যদি (Ast_গ্রন্থি_Pre_Sel* selp = VN_CAST(lhsp, গ্রন্থি_Pre_Sel)) {
            যদি (Ast_গ্রন্থি_Var_Ref* varRefp = VN_CAST(selp->fromp(), গ্রন্থি_Var_Ref)) {
                যদি (varRefp->varp()->direction() == VDirection::প্রবেশ) {
                    VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
                    ফেরত nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        ফেরত নতুন AstAssignW{flp, lhsp, rhsp};
    }

    শূন্য visit(AstStable* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সত্য);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSteady* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সত্য);
            nodep->dtypeSetBit();
        }
    }

    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    শূন্য visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    শূন্য visit(AstUntil* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            যদি (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            যদি (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    শূন্য visit(AstRand* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            যদি (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } অন্যথা {
                nodep->dtypeSetInt();  // Says the spec
            }
            যদি (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    শূন্য visit(AstSEventually* nodep) override {
        যদি (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::Widthed_মান{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ফেরত;
        }
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSGotoRep* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_পরবর্তী(nodep->countp(), WidthVP{SELF, BOTH}.p());
            যদি (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } অন্যথা {
                width_Check_Rep_গুনতি(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSNonConsRep* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_পরবর্তী(nodep->countp(), WidthVP{SELF, BOTH}.p());
            যদি (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } অন্যথা {
                width_Check_Rep_গুনতি(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    শূন্য width_Check_Rep_গুনতি(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->countp(), Const);
        যদি (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } অন্যথা যদি (constp->dtypep()->isSigned() && constp->অঙ্ক().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } অন্যথা যদি (constp->অঙ্ক().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } অন্যথা যদি (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    শূন্য widthCheckGotoRepRange(T_Rep* nodep, ধ্রুবক অক্ষর* kind) {
        user_Iterate_And_পরবর্তী(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ধ্রুবক AstConst* ধ্রুবক minConstp = VN_CAST(nodep->countp(), Const);
        ধ্রুবক AstConst* ধ্রুবক maxConstp = VN_CAST(nodep->maxCountp(), Const);
        যদি (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            ফেরত;
        }
        যদি (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            ফেরত;
        }
        যদি (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            ফেরত;
        }
        যদি (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    শূন্য visit(AstSThroughout* nodep) override {
        m_hasSExpr = সত্য;
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = মিথ্যা;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = সত্য;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = সত্য;
        m_hasSExpr = সত্য;
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            যদি (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = সত্য;
        m_hasSExpr = সত্য;
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            জন্য (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                user_Iterate_And_পরবর্তী(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstURandomRange* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = nodep->findUInt32DType();
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    শূন্য visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        যদি (VN_IS(nodep->backp(), IsUnbounded)) ফেরত;  // Ok, leave
        যদি (VN_IS(nodep->backp(), BracketArrayDType)) ফেরত;  // Ok, leave
        যদি (VN_IS(nodep->backp(), InsideRange)) ফেরত;  // Ok, leave
        যদি (ধ্রুবক auto* ধ্রুবক varp = VN_CAST(nodep->backp(), Var)) {
            যদি (varp->isParam()) ফেরত;  // Ok, leave
        }
        Ast_গ্রন্থি* backp = nodep->backp();
        যদি (VN_IS(backp, Sub)) backp = backp->backp();
        যদি (ধ্রুবক auto* ধ্রুবক selp = VN_CAST(backp, SelExtract)) {
            যদি (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ফেরত;
        }
        যদি (ধ্রুবক auto* ধ্রুবক selp = VN_CAST(backp, SelBit)) {
            যদি (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ফেরত;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    শূন্য visit(AstInferredDisable* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) nodep->dtypeSetBit();
    }
    শূন্য visit(AstIsUnbounded* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        যদি (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        যদি (m_vup->final()) {
            Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            যদি (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    শূন্য visit(AstCLog2* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    শূন্য widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = nodep->valuep();
        ধ্রুবক std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(valuep, Const)) {
            যদি (!constp->অঙ্ক().isNegative()) ফেরত;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } অন্যথা যদি (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } অন্যথা {
            ফেরত;
        }
        // Continue with the default weight
        valuep->replaceWith(নতুন AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    শূন্য widthCoverInt(T_Option* nodep) {
        user_Iterate_And_পরবর্তী(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    শূন্য visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ধ্রুবক cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        যদি (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ফেরত;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        যদি (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            ফেরত;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        যদি (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            যদি (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } অন্যথা {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } অন্যথা যদি (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = nodep->valuep();
            যদি (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(নতুন AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            }
            ফেরত;
        } অন্যথা যদি (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            ফেরত;
        } অন্যথা যদি (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->valuep(), Const);
            যদি (!constp || !constp->অঙ্ক().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstCoverOption* nodep) override {
        যদি (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ফেরত;
        }
        যদি (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            ফেরত;
        }
        user_Iterate_And_পরবর্তী(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstCoverCross* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->itemsp(), nullptr);
        যদি (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_পরবর্তী(nodep->optionsp(), nullptr);
        user_Iterate_And_পরবর্তী(nodep->binsp(), nullptr);
    }
    শূন্য visit(AstCoverCrossBin* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->selectp(), nullptr);
        যদি (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    শূন্য visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        user_Iterate_And_পরবর্তী(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        যদি (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_পরবর্তী(nodep->optionsp(), nullptr);
    }
    শূন্য widthCovergroupRanges(Ast_গ্রন্থি* rangesp, পূর্ণসংখ্যা fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ধ্রুবক auto fill = [&](Ast_গ্রন্থি* nodep) {
            যদি (!fillWidth) ফেরত;
            Ast_গ্রন্থি_রাশিমালা* exprp = VN_AS(nodep, গ্রন্থি_রাশিমালা);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        জন্য (Ast_গ্রন্থি *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (AstInsideRange* ধ্রুবক rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } অন্যথা যদি (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } অন্যথা {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    শূন্য visit(AstCoverBinsof* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    শূন্য visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        যদি (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        যদি (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            user_Iterate_And_পরবর্তী(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        user_Iterate_And_পরবর্তী(nodep->transp(), m_vup);
    }
    শূন্য visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        user_Iterate_And_পরবর্তী(nodep->itemp(), nullptr);
        যদি (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        যদি (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    শূন্য visit(AstCoverCrossWith* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        user_Iterate_And_পরবর্তী(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক countp = nodep->countp()) {
            যদি (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } অন্যথা {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                user_Iterate_And_পরবর্তী(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    শূন্য widthCoverFilter(Ast_গ্রন্থি_রাশিমালা* filterp, ধ্রুবক অক্ষর* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), গ্রন্থি_রাশিমালা);
        File_সারি* ধ্রুবক fl = filterp->fileline();
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক resultp = filterp->dtypep()->skipRefp();
        যদি (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                নতুন AstNeqD{fl, filterp, নতুন AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } অন্যথা যদি (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(নতুন AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(filterp), filterp);
        } অন্যথা {
            fixWidthReduce(filterp);
        }
    }
    শূন্য visit(AstCoverTransSet* nodep) override { user_Iterate_And_পরবর্তী(nodep->itemsp(), m_vup); }
    শূন্য visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    শূন্য visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            যদি (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                ফেরত;
            }
            checkCvtUS(nodep->lhsp(), মিথ্যা);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            যদি (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->অঙ্ক().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        যদি (m_vup->final()) {
            Ast_গ্রন্থি_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্রন্থি_DType* ধ্রুবক lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_গ্রন্থি* newp = nullptr;  // No change
            যদি (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = নতুন AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } অন্যথা যদি (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = নতুন AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } অন্যথা যদি (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = নতুন AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            যদি (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        }
    }
    শূন্য visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(Ast_গুনতি_Bits* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ধ্রুবক পূর্ণসংখ্যা widthMin = V3_সংখ্যা::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    শূন্য visit(Ast_গুনতি_Ones* nodep) override {
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ধ্রুবক পূর্ণসংখ্যা widthMin = V3_সংখ্যা::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    শূন্য visit(AstCvtPackString* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        // Opaque returns, so arbitrary
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        যদি (Ast_গ্রন্থি_DType* ধ্রুবক dt = nodep->lhsp()->dtypep()) {
            যদি (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    শূন্য visit(AstCvtPackedToArray* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        // Opaque returns, so arbitrary
        user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    শূন্য visit(AstCvtArrayToArray* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        // Opaque returns, so arbitrary
        user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    শূন্য visit(AstCvtArrayToPacked* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        // Opaque returns, so arbitrary
        user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    শূন্য visit(AstCvtUnpackToQueue* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        // Opaque returns, so arbitrary
        user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    শূন্য visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        যদি (m_vup->final()) {
            user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    শূন্য visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ধ্রুবক দ্বিগুণ timePrescale = constp->অঙ্ক().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ধ্রুবক দ্বিগুণ time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ধ্রুবক newp = নতুন AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstEventControl* nodep) override {
        যদি (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }
        যদি (nodep->fileline()->timingOn()) {
            যদি (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                ফেরত;
            } অন্যথা যদি (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } অন্যথা {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        যদি (nodep->stmtsp()) nodep->add_পরবর্তী_Here(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    স্থির string typenameExtends(ধ্রুবক AstClass* classp) {
        // An interface class may extend several, so those are not named
        যদি (!classp || classp->isInterfaceClass()) ফেরত "";
        জন্য (ধ্রুবক AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            যদি (extendsp->isImplements()) চালু;
            ধ্রুবক AstClass* ধ্রুবক basep = extendsp->classOrNullp();
            ফেরত basep ? " extends class " + basep->dtypeName(সত্য) : "";
        }
        ফেরত "";
    }
    শূন্য visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        যদি (nodep->dimp()) user_Iterate_And_পরবর্তী(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        নির্বাচন (nodep->attrType()) {
        ক্ষেত্র VAttrType::DIM_DIMENSIONS:
        ক্ষেত্র VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_গ্রন্থি_DType* ধ্রুবক dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ধ্রুবক std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(সত্য);
            ধ্রুবক পূর্ণসংখ্যা val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(নতুন AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            বিরতি;
        }
        ক্ষেত্র VAttrType::DIM_BITS_OR_সংখ্যা: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            যদি (VN_IS(nodep->fromp(), গ্রন্থি_রাশিমালা)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা {
                Ast_গ্রন্থি* newp = নতুন AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                user_Iterate_And_পরবর্তী(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            ফেরত;
        }
        ক্ষেত্র VAttrType::DIM_BITS:
        ক্ষেত্র VAttrType::DIM_উপরের:
        ক্ষেত্র VAttrType::DIM_INCREMENT:
        ক্ষেত্র VAttrType::DIM_বাম:
        ক্ষেত্র VAttrType::DIM_নিচের:
        ক্ষেত্র VAttrType::DIM_ডান:
        ক্ষেত্র VAttrType::DIM_আকার: {
            Ast_গ্রন্থি_DType* ধ্রুবক dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            যদি (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক ownModp = v3Global.rootp()->containingModule(dtypep);
                যদি (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    ফেরত;
                }
            }

            যদি (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                নির্বাচন (nodep->attrType()) {
                ক্ষেত্র VAttrType::DIM_আকার: {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = VN_AS(nodep->fromp()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
                    Ast_গ্রন্থি* ধ্রুবক newp
                        = নতুন AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_আকার};
                    newp->dtypeSetInt();
                    newp->didWidth(সত্য);
                    newp->protect(মিথ্যা);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                    বিরতি;
                }
                ক্ষেত্র VAttrType::DIM_বাম:
                ক্ষেত্র VAttrType::DIM_নিচের: {
                    Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    বিরতি;
                }
                ক্ষেত্র VAttrType::DIM_ডান:
                ক্ষেত্র VAttrType::DIM_উপরের: {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = VN_AS(nodep->fromp()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sizep
                        = নতুন AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_আকার};
                    sizep->dtypeSetInt();
                    sizep->didWidth(সত্য);
                    sizep->protect(মিথ্যা);
                    Ast_গ্রন্থি* ধ্রুবক newp
                        = নতুন AstSub{nodep->fileline(), sizep,
                                     নতুন AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                    বিরতি;
                }
                ক্ষেত্র VAttrType::DIM_INCREMENT: {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp
                        = নতুন AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    বিরতি;
                }
                ক্ষেত্র VAttrType::DIM_BITS: {
                    যদি (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } অন্যথা {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    বিরতি;
                }
                পূর্বনির্ধারিত: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } অন্যথা {
                ধ্রুবক std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(সত্য);
                ধ্রুবক uint32_t msbdim = dimpair.first + dimpair.second;
                যদি (!nodep->dimp() || msbdim < 1) {
                    যদি (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = VN_AS(nodep->fromp()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
                        Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstShiftL{
                            nodep->fileline(), নতুন AstLenN{nodep->fileline(), fromp},
                            নতুন AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    } অন্যথা {
                        Ast_গ্রন্থি_DType* ধ্রুবক baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        ধ্রুবক পূর্ণসংখ্যা dim = 1;
                        AstConst* ধ্রুবক newp = dimension_মান(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    }
                } অন্যথা যদি (VN_IS(nodep->dimp(), Const)) {
                    ধ্রুবক পূর্ণসংখ্যা dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* ধ্রুবক newp
                        = dimension_মান(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                } অন্যথা {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ধ্রুবক tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক dimp = nodep->dimp()->unlinkFrBack();
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                }
            }
            বিরতি;
        }
        ক্ষেত্র VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            বিরতি;
        ক্ষেত্র VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_গ্রন্থি_DType* ধ্রুবক protoDtp = nodep->fromp()->dtypep();
            Ast_গ্রন্থি_DType* ধ্রুবক declDtp = [&]() {
                যদি (m_ftaskp->fvarp()) ফেরত m_ftaskp->fvarp()->dtypep();
                Ast_গ্রন্থি_DType* ধ্রুবক voidp = নতুন AstVoidDType{m_ftaskp->fileline()};
                ঠেলো_Deletep(voidp);  // Note voidp used past here
                ফেরত voidp;
            }();
            যদি (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            বিরতি;
        }
        ক্ষেত্র VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->fromp()->dtypep();
            string ফলাফল = dtypep->prettyDTypeName(সত্য);
            যদি (ধ্রুবক AstClassRefDType* ধ্রুবক classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                ফলাফল += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << ফলাফল << "' from " << dtypep);
            Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstConst{nodep->fileline(), AstConst::String{}, ফলাফল};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            বিরতি;
        }
        ক্ষেত্র VAttrType::TYPEID:
            যদি (Ast_গ্রন্থি_DType* dtypep = VN_CAST(nodep->fromp(), গ্রন্থি_DType)) {
                nodep->dtypep(dtypep);
            } অন্যথা {
                nodep->dtypep(VN_AS(nodep->fromp(), গ্রন্থি_রাশিমালা)->dtypep());
            }
            বিরতি;
        ক্ষেত্র VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            বিরতি;
        পূর্বনির্ধারিত: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            বিরতি;
        }
        }
    }
    শূন্য visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    শূন্য visit(AstCellArrayRef* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        user_Iterate_And_পরবর্তী(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        user_Iterate_And_পরবর্তী(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(সত্য);
    }

    শূন্য visit(AstCellRef* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;

        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক cellExprp = VN_CAST(nodep->cellp(), গ্রন্থি_রাশিমালা)) {
            user_Iterate_And_পরবর্তী(cellExprp, WidthVP{SELF, PRELIM}.p());
            user_Iterate_And_পরবর্তী(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = VN_CAST(nodep->exprp(), গ্রন্থি_রাশিমালা)) {
            user_Iterate_And_পরবর্তী(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            user_Iterate_And_পরবর্তী(exprp, WidthVP{SELF, FINAL}.p());
        } অন্যথা {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(সত্য);
    }

    // DTYPES
    শূন্য visit(Ast_গ্রন্থি_Array_DType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed

        যদি (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ধ্রুবক basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            যদি (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* ধ্রুবক newp = নতুন AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        যদি (VN_IS(nodep, PackArrayDType)) m_underPackedArray = সত্য;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        user_Iterate_And_পরবর্তী(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        যদি (AstUnpackArrayDType* ধ্রুবক adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            যদি (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(সত্য);
        } অন্যথা {
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক elemDTypep = nodep->subDTypep()->skipRefp();
            যদি (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ধ্রুবক পূর্ণসংখ্যা width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            যদি (!VL_RESTORER_পূর্ব(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ধ্রুবক Ast_গ্রন্থি_DType* baseDTypep = nodep->subDTypep();
                যখন (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                ধ্রুবক AstBasicDType* ধ্রুবক basicp = nodep->basicp();
                যদি (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(সত্য));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstAssocArrayDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->চাবি_DTypep(iterateEditMoveDTypep(nodep, nodep->চাবি_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_গ্রন্থি_DType* ধ্রুবক childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_গ্রন্থি* ধ্রুবক elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_গ্রন্থি* newp;
        যদি (VN_IS(elementsp, Unbounded)) {
            newp = নতুন AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_বৃক্ষ(), elementsp);
        } অন্যথা যদি (Ast_গ্রন্থি_DType* ধ্রুবক keyp = VN_CAST(elementsp, গ্রন্থি_DType)) {
            newp = নতুন AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } অন্যথা {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_সারি* ধ্রুবক elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, সত্য);
            // Must be expression that is constant, but we'll determine that later
            newp = নতুন AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                নতুন AstRange{nodep->fileline(), নতুন AstConst(elementsp->fileline(), 0),
                             নতুন AstSub{elementsNewFl, VN_AS(elementsp, গ্রন্থি_রাশিমালা),
                                        নতুন AstConst(elementsNewFl, 1)},
                             সত্য}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    শূন্য visit(AstConstraintRefDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    শূন্য visit(AstDynArrayDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstQueueDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        user_Iterate_And_পরবর্তী(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        যদি (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_বৃক্ষ();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstVoidDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstUnsizedArrayDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        যদি (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstWildcardArrayDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstBasicDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        যদি (nodep->generic()) ফেরত;  // Already perfect
        যদি (nodep->rangep()) {
            user_Iterate_And_পরবর্তী(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } অন্যথা যদি (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } অন্যথা যদি (nodep->implicit()) {
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
    শূন্য visit(AstConstDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstRefDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(সত্য);
        যদি (nodep->typeofp()) {  // type(typeofp_expression)
            যদি (Ast_গ্রন্থি_DType* typeofDtp = VN_CAST(nodep->typeofp(), গ্রন্থি_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } অন্যথা {
                // Type comes from expression's type, e.g. "type(variable)"
                user_Iterate_And_পরবর্তী(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_গ্রন্থি* ধ্রুবক typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_বৃক্ষ(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        যদি (nodep->subDTypep()) {
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
        যদি (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ধ্রুবক যৌক্তিক inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            যদি (inTemplateModule) {
                nodep->doingWidth(মিথ্যা);
                ফেরত;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(মিথ্যা);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    শূন্য widthDTypeNameParams(ধ্রুবক AstTypedef* nodep) {
        ধ্রুবক VDTypeNameScopes* ধ্রুবক scopesp = VDTypeNameScopes::currentp();
        যদি (!scopesp) ফেরত;  // Computing parameters, as by V3Param, so naming as found
        Ast_গ্রন্থি* scopep = nullptr;
        জন্য (ধ্রুবক Ast_গ্রন্থি* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            যদি (ধ্রুবক std::vector<Ast_গ্রন্থি*>* ধ্রুবক paramsp = scopesp->paramsp(scopep)) {
                জন্য (Ast_গ্রন্থি* ধ্রুবক paramp : *paramsp) {
                    যদি (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    শূন্য visit(AstTypedef* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        যদি (auto* ধ্রুবক refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ধ্রুবক যৌক্তিক declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        যদি (declares) {
            Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->dtypep();
            যদি (Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক sdtypep = VN_CAST(dtypep, গ্রন্থি_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } অন্যথা যদি (AstEnumDType* ধ্রুবক edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    শূন্য visit(AstParamTypeDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        যদি (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    শূন্য visit(AstRequireDType* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        যদি (Ast_গ্রন্থি_DType* ধ্রুবক dtp = VN_CAST(nodep->lhsp(), গ্রন্থি_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } অন্যথা {
            যদি (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(নতুন AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_গ্রন্থি_DType* ধ্রুবক toDtp = nodep->শীর্ষ()->dtypep()->skipRefToEnump();
        Ast_গ্রন্থি_DType* ধ্রুবক fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_সারি* ধ্রুবক fl = nodep->fileline();
        ধ্রুবক auto castable = Ast_গ্রন্থি::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_গ্রন্থি* newp;
        যদি (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            ফেরত;
        } অন্যথা যদি (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ধ্রুবক enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক testp = enumTestValid(nodep->fromp(), enumDtp);
            File_সারি* ধ্রুবক fl_novalue = নতুন File_সারি{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, সত্য);
            newp = নতুন AstCond{
                fl, testp,
                নতুন Ast_রাশিমালা_Stmt{fl,
                                নতুন AstAssign{fl_novalue, nodep->শীর্ষ()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                নতুন AstConst{fl, AstConst::Signed32{}, 1}},
                নতুন AstConst{fl, AstConst::Signed32{}, 0}};
        } অন্যথা যদি ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = নতুন Ast_রাশিমালা_Stmt{
                fl,
                নতুন AstAssign{fl, nodep->শীর্ষ()->unlinkFrBack(),
                              নতুন AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                নতুন AstConst{fl, AstConst::Signed32{}, 1}};
        } অন্যথা যদি (castable == VCastable::INCOMPATIBLE) {
            newp = নতুন AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } অন্যথা {
            newp = নতুন AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    শূন্য visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        যদি (AstConst* ধ্রুবক constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_গ্রন্থি* ধ্রুবক newp
                = নতুন Ast_Cast_আকার{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } অন্যথা যদি (Ast_গ্রন্থি_DType* ধ্রুবক refp = VN_CAST(nodep->dtp(), গ্রন্থি_DType)) {
            refp->unlinkFrBack();
            Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } অন্যথা {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(AstCast* nodep) override {
        যদি (nodep->didWidth()) ফেরত;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        যদি (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            যদি (AstSigned* ধ্রুবক fromp = VN_CAST(nodep->fromp(), Signed)) {
                যদি (VN_IS(fromp->lhsp(), গ্রন্থি_Stream)) {
                    Ast_গ্রন্থি* ধ্রুবক lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_বৃক্ষ(), fromp);
                }
            }
            user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_গ্রন্থি_DType* ধ্রুবক toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_গ্রন্থি_DType* ধ্রুবক fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            ধ্রুবক auto castable = Ast_গ্রন্থি::computeCastable(toDtp, fromDtp, nodep->fromp());
            যৌক্তিক bad = মিথ্যা;
            যদি (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = সত্য;
            } অন্যথা যদি (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } অন্যথা যদি (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = সত্য;
            } অন্যথা যদি (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = সত্য;
            } অন্যথা {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
            যদি (bad) {
            } অন্যথা যদি (AstBasicDType* ধ্রুবক basicp = toDtp->basicp()) {
                যদি (!basicp->isString() && fromDtp->isString()) {
                    newp = নতুন AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } অন্যথা যদি (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_গ্রন্থি_DType* ধ্রুবক origDTypep = nodep->dtypep();
                    যদি (!VN_IS(fromDtp, StreamDType)) {
                        ধ্রুবক পূর্ণসংখ্যা width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } অন্যথা {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 মিথ্যা);
                }
                যদি (newp) {
                } অন্যথা যদি (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    যদি (nodep->fromp()->isSigned()) {
                        newp = নতুন AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } অন্যথা {
                        newp = নতুন AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } অন্যথা যদি (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = নতুন AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } অন্যথা যদি (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = নতুন AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } অন্যথা যদি (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = নতুন AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } অন্যথা {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } অন্যথা যদি (VN_IS(toDtp, QueueDType)) {
                যদি (VN_IS(fromDtp, BasicDType)) {
                    newp = নতুন AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } অন্যথা যদি (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    পূর্ণসংখ্যা srcElementBits = 1;
                    পূর্ণসংখ্যা dstElementBits = 1;
                    যদি (Ast_গ্রন্থি_DType* ধ্রুবক elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ধ্রুবক AstQueueDType* ধ্রুবক dstQueueDtp = VN_AS(toDtp, QueueDType);
                    যদি (Ast_গ্রন্থি_DType* ধ্রুবক elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = নতুন AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  মিথ্যা,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } অন্যথা যদি (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } অন্যথা {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            যদি (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        যদি (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, মিথ্যা, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(সত্য);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(সত্য);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(Ast_Cast_আকার* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            পূর্ণসংখ্যা width = nodep->rhsp()->toSInt();
            যদি (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        যদি (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_গ্রন্থি* ধ্রুবক underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    শূন্য visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    শূন্য castSized(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* underp, পূর্ণসংখ্যা width) {
        ধ্রুবক AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        যদি (!underDtp) underDtp = underp->dtypep()->basicp();
        যদি (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ধ্রুবক পূর্ণসংখ্যা calcWidth = std::max(width, underDtp->width());
            Ast_গ্রন্থি_DType* ধ্রুবক calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         মিথ্যা, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_গ্রন্থি_DType* ধ্রুবক outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, গ্রন্থি_রাশিমালা), outDtp, EXTEND_EXP,
                            মিথ্যা);
            VL_DANGLING(underp);
        }
    }
    শূন্য visit(AstConstraintBefore* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_পরবর্তী(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(Ast_Constraint_রাশিমালা* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstConstraintUnique* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    শূন্য visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        যদি (nodep->didWidth()) ফেরত;
        যদি (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ঠেলো_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(সত্য);
            ফেরত;
        }
        nodep->doingWidth(সত্য);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        যদি (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.ঠেলো_back(nodep);
        যদি (nodep->attrsp()) {
            nodep->attrsp()->foreach([এই, nodep](AstAttrOf* attrp) {
                যদি (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    যদি (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), গ্রন্থি_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ঠেলো_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        যদি (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_গ্রন্থি_DType* dtp = nodep->dtypep();
            Ast_গ্রন্থি_DType* np = nullptr;
            যখন (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                যদি (ধ্রুবক AstDynArrayDType* ধ্রুবক unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    যদি (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } অন্যথা {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ধ্রুবক newp
                        = নতুন AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    যদি (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } অন্যথা {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } অন্যথা {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        যদি (AstWildcardArrayDType* ধ্রুবক wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        যদি (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(সত্য);
        // Parameters if implicit untyped inherit from what they are assigned to
        ধ্রুবক AstBasicDType* ধ্রুবক bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        যৌক্তিক didchk = মিথ্যা;
        ধ্রুবক যৌক্তিক implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        যদি (implicitParam) {
            যদি (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                user_Iterate_And_পরবর্তী(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                যদি (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } অন্যথা যদি (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } অন্যথা {
                    পূর্ণসংখ্যা width = 0;
                    Ast_গ্রন্থি_DType* ধ্রুবক মান_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(মান_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ধ্রুবক AstBasicDType* ধ্রুবক মান_Bdtypep = মান_DTypep->basicp();
                    যৌক্তিক issigned = মিথ্যা;
                    যদি (bdtypep->isNosign()) {
                        যদি (মান_Bdtypep && মান_Bdtypep->isSigned()) issigned = সত্য;
                    } অন্যথা {
                        issigned = bdtypep->isSigned();
                    }
                    যদি (মান_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        যদি (ধ্রুবক auto* ধ্রুবক constp = VN_CAST(nodep->valuep(), Const)) {
                            যদি (constp->অঙ্ক().isString()) {
                                width = constp->অঙ্ক().toString().দৈর্ঘ্য() * 8;
                            }
                        }
                        যদি (width < 8) width = 8;
                    } অন্যথা যদি (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } অন্যথা {
                        যদি (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ধ্রুবক যৌক্তিক is_মান_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    যদি (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_মান_Fourstate));
                    } অন্যথা {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_মান_Fourstate);
                    }
                    didchk = সত্য;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } অন্যথা {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } অন্যথা যদি (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        যদি (nodep->isNet()) {
            Ast_গ্রন্থি_DType* ধ্রুবক badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            যদি (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        যদি (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            user_Iterate_And_পরবর্তী(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        user_Iterate_And_পরবর্তী(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(সত্য);
        nodep->doingWidth(মিথ্যা);
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (nodep->didWidth()) ফেরত;
        যদি (!nodep->varp()) {
            যদি (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, মিথ্যা,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                ফেরত;
            } অন্যথা {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        যদি (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        যদি (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            ফেরত;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        যদি (VN_IS(nodep->backp(), গ্রন্থি_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        যদি (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(সত্য);
        যদি (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } অন্যথা যদি (nodep->access().isWriteOrRW() && nodep->varp()->is_প্রবেশ()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ধ্রুবক যৌক্তিক hierRef = !m_curModVars.গুনতি(nodep->varp());
            ধ্রুবক যৌক্তিক netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            ধ্রুবক যৌক্তিক contAssign = VN_IS(nodep->abovep(), AssignW);
            ধ্রুবক যৌক্তিক pinConn = VN_IS(nodep->abovep(), Pin);
            যদি (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } অন্যথা যদি (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        যদি (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(সত্য);
    }

    শূন্য visit(AstEnumDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_গ্রন্থি_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_গ্রন্থি_DType* ধ্রুবক badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        যদি (badDtp) {
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
        user_Iterate_And_পরবর্তী(nodep->itemsp(), WidthVP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_সংখ্যা অঙ্ক(nodep, nodep->width(), 0);
        ধ্রুবক V3_সংখ্যা one{nodep, nodep->width(), 1};
        যৌক্তিক wrapAround = মিথ্যা;
        std::map<ধ্রুবক V3_সংখ্যা, AstEnumItem*> inits;
        জন্য (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            যদি (itemp->valuep()) {
                যদি (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_বৃক্ষ("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                যদি (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_বৃক্ষ();
                    চালু;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = মিথ্যা;  // Explicit value resets wrap-around tracking
            }
            যদি (!itemp->valuep()) {
                যদি (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                যদি (অঙ্ক.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(নতুন AstConst{itemp->fileline(), অঙ্ক});
            }

            ধ্রুবক AstConst* ধ্রুবক constp = VN_AS(itemp->valuep(), Const);
            যদি (constp->অঙ্ক().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            অঙ্ক.opAssign(constp->অঙ্ক());
            // Look for duplicates
            ধ্রুবক auto pair = inits.emplace(অঙ্ক, itemp);
            যদি (!pair.second) {  // IEEE says illegal
                ধ্রুবক Ast_গ্রন্থি* ধ্রুবক otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ধ্রুবক যৌক্তিক পূর্ব_Neg = constp->অঙ্ক().isNegative();
            অঙ্ক.opAdd(one, constp->অঙ্ক());
            যদি (basicp->isSigned()) {
                wrapAround = !পূর্ব_Neg && অঙ্ক.isNegative();
            } অন্যথা {
                wrapAround = অঙ্ক.isEqZero();
            }
        }
    }
    শূন্য visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_গ্রন্থি_DType* ধ্রুবক vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        যদি (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            user_Iterate_And_পরবর্তী(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            যৌক্তিক warnOn = সত্য;
            Ast_গ্রন্থি_রাশিমালা* valuep = nodep->valuep();
            যদি (ধ্রুবক AstAdd* ধ্রুবক anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                যদি (anodep->fileline()->equal_First_সারি_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(valuep, Const)) {
                যদি (static_cast<পূর্ণসংখ্যা>(constp->অঙ্ক().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = মিথ্যা;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(নতুন AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    শূন্য visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        যদি (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_গ্রন্থি* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            জন্য (; enump; enump = enump->backp()) {
                যদি (VN_IS(enump, EnumDType)) বিরতি;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    শূন্য visit(Ast_গ্রন্থি_Cons* nodep) override {}  // Widthed when created
    শূন্য visit(Ast_গ্রন্থি_Cons_Item* nodep) override {}  // Widthed when created
    শূন্য visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        জন্য (Ast_গ্রন্থি *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_গ্রন্থি_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        যদি (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } অন্যথা যদি (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } অন্যথা {
            // Take width as maximum across all items
            পূর্ণসংখ্যা width = nodep->exprp()->width();
            পূর্ণসংখ্যা mwidth = nodep->exprp()->widthMin();
            যৌক্তিক isFourstate = nodep->exprp()->dtypep()->isFourstate();
            জন্য (ধ্রুবক Ast_গ্রন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        জন্য (Ast_গ্রন্থি *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            যদি (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } অন্যথা {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        যদি (m_constraintp) {
            যৌক্তিক canLower = সত্য;
            জন্য (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                যদি (VN_IS(ditemp->rangep(), InsideRange)) চালু;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                যদি (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = মিথ্যা;
                    বিরতি;
                }
            }
            যদি (canLower) ফেরত;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
        জন্য (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_গ্রন্থি_রাশিমালা* inewp
                = insideItem(nodep, nodep->exprp()->clone_বৃক্ষ_Pure(মিথ্যা), itemp->rangep());
            যদি (!inewp) চালু;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক cmpp
                = নতুন AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            নতুন AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, সত্য);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, সত্য);
            inewp = নতুন AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? নতুন AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        যদি (!newp) newp = নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstInside* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        জন্য (Ast_গ্রন্থি *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_গ্রন্থি_DType* expDTypep = nullptr;

        যদি (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } অন্যথা যদি (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } অন্যথা {
            // Take width as maximum across all items
            পূর্ণসংখ্যা width = nodep->exprp()->width();
            পূর্ণসংখ্যা mwidth = nodep->exprp()->widthMin();
            যৌক্তিক isFourstate = nodep->exprp()->dtypep()->isFourstate();
            জন্য (ধ্রুবক Ast_গ্রন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            ধ্রুবক VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        জন্য (Ast_গ্রন্থি *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            যদি (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } অন্যথা যদি (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_গ্রন্থি_রাশিমালা* exprp;
        Ast_রাশিমালা_Stmt* রাশিমালা_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        যদি (!m_constraintp && !nodep->exprp()->isPure()) {
            File_সারি* ধ্রুবক fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_গ্রন্থি_DType* ধ্রুবক রাশিমালা_Dtp = nodep->exprp()->dtypep();
            ধ্রুবক পূর্ণসংখ্যা w = রাশিমালা_Dtp->width();
            Ast_গ্রন্থি_DType* ধ্রুবক অস্থায়ী_DTypep
                = রাশিমালা_Dtp->widthSized() ? রাশিমালা_Dtp
                                        : nodep->findBitOrLogicDType(w, w, রাশিমালা_Dtp->numeric(),
                                                                     রাশিমালা_Dtp->isFourstate());
            AstVar* ধ্রুবক varp
                = নতুন AstVar{fl, VVarType::XTEMP, m_inside_অস্থায়ী_Names.get(nodep), অস্থায়ী_DTypep};
            exprp = নতুন AstVarRef{fl, varp, VAccess::READ};
            রাশিমালা_Stmtp = নতুন Ast_রাশিমালা_Stmt{fl,
                                        নতুন AstAssign{fl, নতুন AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            যদি (m_ftaskp) {
                varp->funcLocal(সত্য);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } অন্যথা {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(রাশিমালা_Stmtp);
        } অন্যথা {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
        জন্য (Ast_গ্রন্থি_রাশিমালা *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), গ্রন্থি_রাশিমালা);  // Will be unlinking
            Ast_গ্রন্থি_রাশিমালা* inewp;
            যদি (রাশিমালা_Stmtp) {
                inewp = insideItem(nodep, রাশিমালা_Stmtp, itemp);
                রাশিমালা_Stmtp = nullptr;
            } অন্যথা {
                inewp = insideItem(nodep, exprp->clone_বৃক্ষ_Pure(মিথ্যা), itemp);
            }
            যদি (!inewp) চালু;
            newp = newp ? নতুন AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        যদি (রাশিমালা_Stmtp) VL_DO_DANGLING(রাশিমালা_Stmtp->delete_বৃক্ষ(), রাশিমালা_Stmtp);
        যদি (!newp) newp = নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    Ast_গ্রন্থি_রাশিমালা* insideItem(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি_রাশিমালা* itemp) {
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক itemDtp = itemp->dtypep()->skipRefp();
        যদি (AstInsideRange* ধ্রুবক irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            ফেরত irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } অন্যথা যদি (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক inewp = নতুন AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(সত্য);
            ফেরত inewp;
        } অন্যথা যদি (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_বৃক্ষ(), exprp);
            ফেরত nullptr;
        }
        ফেরত AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    শূন্য visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        যদি (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } অন্যথা {
            যদি (m_vup->prelim()) {
                user_Iterate_And_পরবর্তী(nodep->lhsp(), m_vup);
                user_Iterate_And_পরবর্তী(nodep->rhsp(), m_vup);
            }
            যদি (m_vup->final()) {
                Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ধ্রুবক পূর্ণসংখ্যা expWidth = expDTypep->width();
                ধ্রুবক যৌক্তিক waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                ধ্রুবক যৌক্তিক waiveRhs = expWidth == 32
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

    শূন্য visit(AstIfaceRefDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        যদি (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_UOr_Struct_DType* nodep) override {
        যদি (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(মিথ্যা);
            ফেরত;
        }
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(সত্য);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        যদি (ধ্রুবক AstUnionDType* ধ্রুবক unionp = VN_CAST(nodep, UnionDType)) {
            যদি (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(মিথ্যা);
        // Error checks
        জন্য (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_গ্রন্থি_DType* ধ্রুবক dtp = itemp->subDTypep()->skipRefp();
            যদি (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            যদি ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ঠেলো_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        ধ্রুবক যৌক্তিক isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ধ্রুবক যৌক্তিক inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        যদি (VN_IS(nodep, UnionDType) || nodep->packed()) {
            পূর্ণসংখ্যা lsb = 0;
            পূর্ণসংখ্যা width = 0;
            যৌক্তিক first = সত্য;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            জন্য (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            জন্য (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                যদি (itemp->isFourstate()) nodep->isFourstate(সত্য);
                itemp->lsb(lsb);
                যদি (VN_IS(nodep, UnionDType)) {
                    ধ্রুবক পূর্ণসংখ্যা itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    যদি (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } অন্যথা {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = মিথ্যা;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } অন্যথা {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(মিথ্যা);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    শূন্য visit(AstThisRef* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        যদি (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    শূন্য visit(AstClassRefDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    শূন্য visit(AstClassOrPackageRef* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        userIterateChildren(nodep, nullptr);
    }
    শূন্য visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstClassExtends* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        যদি (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    শূন্য visit(AstMemberDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        যদি (nodep->valuep()) {
            user_Iterate_And_পরবর্তী(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    শূন্য visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        যদি (nodep->didWidth()) ফেরত;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = সত্য;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_গ্রন্থি_DType* ধ্রুবক fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        ধ্রুবক AstMemberSel* ধ্রুবক fromSel = VN_CAST(nodep->fromp(), MemberSel);
        যদি (AstClocking* ধ্রুবক clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            যদি (AstVar* ধ্রুবক varp = memberSelClocking(nodep, clockingp)) {
                যদি (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_বৃক্ষ(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                যদি (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                যদি (AstIfaceRefDType* ধ্রুবক adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(সত্য);  // Must not visit again: will confuse scopes
                ফেরত;
            }
        } অন্যথা যদি (Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক adtypep = VN_CAST(fromDtp, গ্রন্থি_UOr_Struct_DType)) {
            যদি (memberSelStruct(nodep, adtypep)) ফেরত;
        } অন্যথা যদি (AstClassRefDType* ধ্রুবক adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            যদি (memberSelClass(nodep, adtypep)) ফেরত;
        } অন্যথা যদি (AstIfaceRefDType* ধ্রুবক adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            যদি (Ast_গ্রন্থি* foundp = memberSelIface(nodep, adtypep)) {
                যদি (AstClocking* ধ্রুবক clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                যদি (AstVar* ধ্রুবক varp = VN_CAST(foundp, Var)) {
                    যদি (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* ধ্রুবক ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ধ্রুবক AstVarRef* ধ্রুবক refp = VN_CAST(nodep->fromp(), VarRef);
                    যদি (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ধ্রুবক newp = নতুন AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(সত্য);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                        ফেরত;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(সত্য);
                    ফেরত;
                }
                যদি (AstModport* ধ্রুবক modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ধ্রুবক newDtypep = নতুন AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত;
                }
                যদি (AstCell* ধ্রুবক cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    যদি (VN_IS(cellp->modp(), Iface)) {
                        ধ্রুবক string viftopName = cellp->name() + "__Viftop";
                        Ast_গ্রন্থি_Module* ধ্রুবক parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        জন্য (Ast_গ্রন্থি* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            যদি (AstVar* ধ্রুবক vp = VN_CAST(itemp, Var)) {
                                যদি (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    বিরতি;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        যদি (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_গ্রন্থি_DType* subDtypep = viftopVarp->dtypep();
                        যদি (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            যদি (AstIfaceRefDType* ধ্রুবক subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ধ্রুবক newDtypep = নতুন AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(সত্য);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(সত্য);
                        ফেরত;
                    }
                }
                যদি (Ast_গ্রন্থি_FTask* ধ্রুবক ftaskp = VN_CAST(foundp, গ্রন্থি_FTask)) {
                    AstMethodCall* ধ্রুবক newp = নতুন AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ফেরত;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } অন্যথা যদি (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ফেরত;
        } অন্যথা {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    যৌক্তিক memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        যদি (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* ধ্রুবক newp = নতুন AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            visit(newp);
            ফেরত সত্য;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ধ্রুবক first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        জন্য (AstClass* classp = first_classp; classp;) {
            যদি (Ast_গ্রন্থি* ধ্রুবক foundp = m_memberMap.findMember(classp, nodep->name())) {
                যদি (AstVar* ধ্রুবক varp = VN_CAST(foundp, Var)) {
                    যদি (!varp->didWidth()) userIterate(varp, nullptr);
                    যদি (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ধ্রুবক varRefp
                            = নতুন AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                        ফেরত সত্য;
                    }
                    যদি (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(সত্য);
                    যদি (nodep->fromp()->same_বৃক্ষ(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    ফেরত সত্য;
                }
                যদি (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(নতুন AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত সত্য;
                }
                যদি (AstEnumItemRef* ধ্রুবক adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_বৃক্ষ(মিথ্যা));
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত সত্য;
                }
                যদি (Ast_গ্রন্থি_FTask* ftaskp = VN_CAST(foundp, গ্রন্থি_FTask)) {
                    AstMethodCall* newp = নতুন AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত সত্য;
                }
                যদি (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    ফেরত সত্য;
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
        জন্য (AstClass* classp = first_classp; classp;) {
            জন্য (Ast_গ্রন্থি* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                যদি (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.ঠেলো_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        ধ্রুবক string suggest = speller.best_Candidate_বার্তা(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ফেরত মিথ্যা;  // Caller handles error
    }
    যৌক্তিক modportAllowsItem(Ast_গ্রন্থি* nodep, ধ্রুবক AstIfaceRefDType* adtypep, ধ্রুবক Ast_গ্রন্থি* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ধ্রুবক AstModport* ধ্রুবক modportp = adtypep->modportp();
        যদি (!modportp) ফেরত সত্য;
        যদি (ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(itemp, Var)) {
            যদি (varp->isParam()) ফেরত সত্য;
        } অন্যথা যদি (!VN_IS(itemp, গ্রন্থি_FTask) && !VN_IS(itemp, Clocking)) {
            ফেরত সত্য;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        যদি (pair.second) {
            জন্য (ধ্রুবক Ast_গ্রন্থি* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.ঢোকাও(mpItemp->name());
            }
        }
        যদি (pair.first->second.গুনতি(itemp->name())) ফেরত সত্য;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        ফেরত মিথ্যা;
    }
    Ast_গ্রন্থি* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_গ্রন্থি_Module* ধ্রুবক ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        জন্য (Ast_গ্রন্থি* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            যদি (itemp->name() == nodep->name()) {
                যদি (!modportAllowsItem(nodep, adtypep, itemp)) {
                    ফেরত nullptr;  // Caller handles error
                }
                ফেরত itemp;
            }
            যদি (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ঠেলো_Candidate(itemp->prettyName());
            }
        }
        ধ্রুবক string suggest = speller.best_Candidate_বার্তা(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ফেরত nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        জন্য (Ast_গ্রন্থি* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            যদি (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                যদি (citemp->varp()->name() == nodep->name()) ফেরত citemp->varp();
                speller.ঠেলো_Candidate(citemp->varp()->prettyName());
            }
        }
        ধ্রুবক string suggest = speller.best_Candidate_বার্তা(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ফেরত nullptr;  // Caller handles error
    }
    যৌক্তিক memberSelStruct(AstMemberSel* nodep, Ast_গ্রন্থি_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        যদি (AstMemberDType* ধ্রুবক memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            যদি (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } অন্যথা যদি (adtypep->packed()) {
                AstSel* ধ্রুবক newp = নতুন AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(সত্য);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } অন্যথা {
                AstStructSel* ধ্রুবক newp = নতুন AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(সত্য);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            ফেরত সত্য;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        ফেরত মিথ্যা;
    }

    শূন্য visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    শূন্য visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        যদি (nodep->didWidth()) ফেরত;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_গ্রন্থি_DType* ধ্রুবক fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* ধ্রুবক basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        যদি (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } অন্যথা যদি (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } অন্যথা যদি (AstEnumDType* ধ্রুবক adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } অন্যথা যদি (AstAssocArrayDType* ধ্রুবক adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } অন্যথা যদি (AstWildcardArrayDType* ধ্রুবক adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } অন্যথা যদি (AstDynArrayDType* ধ্রুবক adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } অন্যথা যদি (AstQueueDType* ধ্রুবক adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } অন্যথা যদি (AstClassRefDType* ধ্রুবক adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } অন্যথা যদি (AstIfaceRefDType* ধ্রুবক adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } অন্যথা যদি (AstUnpackArrayDType* ধ্রুবক adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } অন্যথা যদি (AstConstraintRefDType* ধ্রুবক adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } অন্যথা যদি (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } অন্যথা যদি (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } অন্যথা {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_গ্রন্থি_FTask_Ref* nodep, যৌক্তিক required, যৌক্তিক arbReturn,
                              Ast_গ্রন্থি_DType* returnDtp, Ast_গ্রন্থি_DType* সূচক_Dtp,
                              Ast_গ্রন্থি_DType* মান_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        যদি (AstWith* ধ্রুবক withp = nodep->withp()) {
            withp->সূচক_Arg_Refp()->dtypep(সূচক_Dtp);
            withp->মান_Arg_Refp()->dtypep(মান_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            ফেরত withp;
        }
        যদি (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        ফেরত nullptr;
    }
    শূন্য methodOkArguments(Ast_গ্রন্থি_FTask_Ref* nodep, পূর্ণসংখ্যা minArg, পূর্ণসংখ্যা maxArg) {
        পূর্ণসংখ্যা narg = 0;
        যদি (AstWith* ধ্রুবক withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ঠেলো_Deletep(withp->unlinkFrBack()), withp);
        }
        জন্য (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ধ্রুবক যৌক্তিক ok = (narg >= minArg) && (narg <= maxArg);
        যদি (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            জন্য (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    নতুন AstArg{nodep->fileline(), "",
                               নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            জন্য (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                যখন (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_বৃক্ষ(), argp);
            }
        }
    }

    Ast_গ্রন্থি_রাশিমালা* methodArg(AstMethodCall* nodep, পূর্ণসংখ্যা arg) {
        AstArg* argp = nodep->argsp();
        জন্য (পূর্ণসংখ্যা narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        ফেরত argp->exprp();
    }

    শূন্য methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        যদি (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            যদি (nodep->name() == "num") {
                পূর্ণসংখ্যা বস্তুসমূহ = 0;
                জন্য (Ast_গ্রন্থি* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++বস্তুসমূহ;
                newp = নতুন AstConst(nodep->fileline(), AstConst::Signed32{}, বস্তুসমূহ);
            } অন্যথা যদি (nodep->name() == "first") {
                ধ্রুবক AstEnumItem* itemp = adtypep->itemsp();
                যদি (!itemp) {
                    newp = নতুন AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } অন্যথা {
                    newp = VN_AS(itemp->valuep()->clone_বৃক্ষ(মিথ্যা), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } অন্যথা যদি (nodep->name() == "last") {
                ধ্রুবক AstEnumItem* itemp = adtypep->itemsp();
                যখন (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                যদি (!itemp) {
                    newp = নতুন AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } অন্যথা {
                    newp = VN_AS(itemp->valuep()->clone_বৃক্ষ(মিথ্যা), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            যদি (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } অন্যথা যদি (nodep->name() == "next") {
                attrType = VAttrType::ENUM_পরবর্তী;
                methodOkArguments(nodep, 0, 1);
            } অন্যথা যদি (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_পূর্ব;
                methodOkArguments(nodep, 0, 1);
            } অন্যথা {
                nodep->v3fatalSrc("Bad case");
            }

            যদি (nodep->name() != "name" && nodep->argsp()) {
                Ast_গ্রন্থি_রাশিমালা* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, সত্য);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                যদি (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ধ্রুবক uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                যদি (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* ধ্রুবক newp = নতুন AstMethodCall{
                        nodep->fileline(),
                        নতুন AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                    ফেরত;
                } অন্যথা যদি (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ঠেলো_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* ধ্রুবক clonep = nodep->clone_বৃক্ষ(মিথ্যা);
                    VN_AS(stepp, Const)->অঙ্ক().setLong(1);
                    AstConst* ধ্রুবক constp = নতুন AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* ধ্রুবক argp = নতুন AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* ধ্রুবক newp
                        = নতুন AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                    ফেরত;
                }
            }
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    শূন্য methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        যদি (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_আকার};  // So don't need num()
            newp->dtypeSetInt();
        } অন্যথা যদি (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } অন্যথা যদি (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক সূচক_exprp = method_Call_Wildcard_সূচক_রাশিমালা(nodep, adtypep);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, সূচক_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } অন্যথা যদি (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            যদি (!nodep->argsp()) {
                newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } অন্যথা {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক সূচক_exprp = method_Call_Wildcard_সূচক_রাশিমালা(nodep, adtypep);
                newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, সূচক_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } অন্যথা যদি (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } অন্যথা যদি (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ধ্রুবক withp
                = methodWithClause(nodep, মিথ্যা, মিথ্যা, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ধ্রুবক withp
                = methodWithClause(nodep, সত্য, মিথ্যা, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "map") {
            AstWith* ধ্রুবক withp
                = methodWithClause(nodep, সত্য, মিথ্যা, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        যদি (newp) {
            newp->protect(মিথ্যা);
            newp->didWidth(সত্য);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    শূন্য methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        যদি (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        যদি (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_আকার};  // So don't need num()
            newp->dtypeSetInt();
        } অন্যথা যদি (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->চাবি_DTypep(), BOTH);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক সূচক_exprp = method_Call_Assoc_সূচক_রাশিমালা(nodep, adtypep);
            methodCallLValueRecurse(nodep, সূচক_exprp, VAccess::READWRITE);
            newp
                = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     সূচক_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->চাবি_DTypep(), BOTH);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক সূচক_exprp = method_Call_Assoc_সূচক_রাশিমালা(nodep, adtypep);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, সূচক_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } অন্যথা যদি (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            যদি (!nodep->argsp()) {
                newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } অন্যথা {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->চাবি_DTypep(),
                                  BOTH);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক সূচক_exprp = method_Call_Assoc_সূচক_রাশিমালা(nodep, adtypep);
                newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, সূচক_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } অন্যথা যদি (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } অন্যথা যদি (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ধ্রুবক withp = methodWithClause(nodep, মিথ্যা, মিথ্যা, adtypep->subDTypep(),
                                                    adtypep->চাবি_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ধ্রুবক withp = methodWithClause(
                nodep, মিথ্যা, সত্য, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            যদি (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->চাবি_DTypep()));
            } অন্যথা {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ধ্রুবক withp = methodWithClause(nodep, সত্য, মিথ্যা, nodep->findBitDType(),
                                                    adtypep->চাবি_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ধ্রুবক withp = methodWithClause(nodep, সত্য, মিথ্যা, nodep->findBitDType(),
                                                    adtypep->চাবি_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->চাবি_DTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "map") {
            AstWith* ধ্রুবক withp = methodWithClause(nodep, সত্য, মিথ্যা, adtypep->subDTypep(),
                                                    adtypep->চাবি_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        যদি (newp) {
            newp->protect(মিথ্যা);
            newp->didWidth(সত্য);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    Ast_গ্রন্থি_রাশিমালা* method_Call_Assoc_সূচক_রাশিমালা(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_গ্রন্থি* ধ্রুবক সূচক_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", সূচক_exprp, CONTEXT_DET, FINAL, adtypep->চাবি_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(সূচক_exprp);  // May have been edited
        ফেরত nodep->argsp()->exprp();
    }
    Ast_গ্রন্থি_রাশিমালা* method_Call_Wildcard_সূচক_রাশিমালা(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_গ্রন্থি* ধ্রুবক সূচক_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", সূচক_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(সূচক_exprp);  // May have been edited
        ফেরত nodep->argsp()->exprp();
    }
    শূন্য methodCallLValueRecurse(AstMethodCall* nodep, Ast_গ্রন্থি* childp, ধ্রুবক VAccess& access) {
        যদি (AstCMethodHard* ধ্রুবক ichildp = VN_CAST(childp, CMethodHard)) {
            যৌক্তিক methAt = মিথ্যা;
            যৌক্তিক methAtBack = মিথ্যা;
            যৌক্তিক methAtWrite = মিথ্যা;
            নির্বাচন (ichildp->method()) {
            ক্ষেত্র VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = সত্য;
                বিরতি;
            ক্ষেত্র VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = সত্য;
                বিরতি;
            ক্ষেত্র VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            ক্ষেত্র VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            ক্ষেত্র VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = সত্য;
                বিরতি;
            পূর্বনির্ধারিত: বিরতি;
            }
            যদি (methAt || methAtBack || methAtWrite) {
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                যদি (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    যদি (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } অন্যথা যদি (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                ফেরত;
            }
        }
        যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুবক varrefp = VN_CAST(childp, গ্রন্থি_Var_Ref)) {
            varrefp->access(access);
        } অন্যথা যদি (ধ্রুবক AstMemberSel* ধ্রুবক ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } অন্যথা যদি (ধ্রুবক AstStructSel* ধ্রুবক ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_Sel* ধ্রুবক ichildp = VN_CAST(childp, গ্রন্থি_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } অন্যথা যদি (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } অন্যথা {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_গ্রন্থি_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        যদি (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            যদি (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, মিথ্যা, সত্য, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ধ্রুবক withp = methodWithClause(
                nodep, মিথ্যা, সত্য, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            যদি (nodep->name() == "unique_index") {
                newp->dtypep(newp->find_Queue_সূচক_DType());
            } অন্যথা {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ধ্রুবক withp
                = methodWithClause(nodep, সত্য, মিথ্যা, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ধ্রুবক withp
                = methodWithClause(nodep, সত্য, মিথ্যা, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->find_Queue_সূচক_DType());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ধ্রুবক withp
                = methodWithClause(nodep, সত্য, মিথ্যা, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        ফেরত newp;
    }
    শূন্য methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        যদি (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_আকার};
            newp->dtypeSetInt();
        } অন্যথা যদি (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ধ্রুবক withp
                = methodWithClause(nodep, মিথ্যা, মিথ্যা, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি ((newp = methodCallArray(nodep, adtypep))) {
        } অন্যথা {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        যদি (newp) {
            newp->protect(মিথ্যা);
            newp->didWidth(সত্য);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    শূন্য methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        যদি (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_আকার};
            newp->dtypeSetInt();
        } অন্যথা যদি (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            যদি (!nodep->argsp()) {
                newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } অন্যথা {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক সূচক_exprp = method_Call_Queue_সূচক_রাশিমালা(nodep);
                যদি (সূচক_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_বের_করো_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } অন্যথা {
                    newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, সূচক_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } অন্যথা যদি (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক সূচক_exprp = method_Call_Queue_সূচক_রাশিমালা(nodep);
            AstArg* ধ্রুবক argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            যদি (সূচক_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ঠেলো_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } অন্যথা {
                newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ঢোকাও, সূচক_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } অন্যথা যদি (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ধ্রুবক argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ধ্রুবক withp
                = methodWithClause(nodep, মিথ্যা, মিথ্যা, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি ((newp = methodCallArray(nodep, adtypep))) {
        } অন্যথা {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        যদি (newp) {
            newp->protect(মিথ্যা);
            newp->didWidth(সত্য);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    Ast_গ্রন্থি_রাশিমালা* method_Call_Queue_সূচক_রাশিমালা(AstMethodCall* nodep) {
        Ast_গ্রন্থি* ধ্রুবক সূচক_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", সূচক_exprp, BOTH);
        VL_DANGLING(সূচক_exprp);  // May have been edited
        ফেরত nodep->argsp()->exprp();
    }
    শূন্য methodCallWarnTiming(Ast_গ্রন্থি_FTask_Ref* ধ্রুবক nodep, ধ্রুবক std::string& className) {
        যদি (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } অন্যথা যদি (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    শূন্য methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ধ্রুবক ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        যদি (Ast_গ্রন্থি_FTask* ধ্রুবক ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), গ্রন্থি_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            যদি (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                ফেরত;
            }
            যদি (adtypep->isVirtual()) {
                জন্য (Ast_গ্রন্থি* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    যদি (AstVar* ধ্রুবক mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            যদি (ftaskp->isStatic()) {
                AstArg* ধ্রুবক argsp = nodep->argsp();
                যদি (argsp) argsp->unlink_Fr_Back_With_পরবর্তী();
                Ast_গ্রন্থি_FTask_Ref* newp = nullptr;
                যদি (VN_IS(ftaskp, Task)) {
                    newp = নতুন AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } অন্যথা {
                    newp = নতুন AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            } অন্যথা {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                যদি (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            ফেরত;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    শূন্য handleRandomizeArgs(Ast_গ্রন্থি_FTask_Ref* ধ্রুবক nodep, AstClass* ধ্রুবক classp) {
        যৌক্তিক hasNonNullArgs = মিথ্যা;
        AstConst* nullp = nullptr;
        জন্য (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_গ্রন্থি_রাশিমালা* exprp = argp->exprp();
            যদি (AstConst* ধ্রুবক constp = VN_CAST(exprp, Const)) {
                যদি (constp->অঙ্ক().isNull()) {
                    nullp = constp;
                    চালু;
                }
            }
            hasNonNullArgs = সত্য;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            যদি (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_গ্রন্থি_রাশিমালা* fromp = methodCallp->fromp();
                যখন (AstMemberSel* ধ্রুবক memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ধ্রুবক varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            যদি (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
                চালু;
            }
            যখন (exprp) {
                যদি (AstMemberSel* ধ্রুবক memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } অন্যথা {
                    যদি (AstVarRef* ধ্রুবক varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } অন্যথা {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                যদি (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            যদি (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } অন্যথা যদি (classp->existsMember([&](ধ্রুবক AstClass*, ধ্রুবক AstVar* memberVarp) {
                           ফেরত memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } অন্যথা {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
            }
        }
        যদি (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    শূন্য methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ধ্রুবক first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        যদি (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, মিথ্যা, মিথ্যা, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            জন্য (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                যদি (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } অন্যথা যদি (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        জন্য (AstClass* classp = first_classp; classp;) {
            যদি (nodep->fileline()->timingOn()) {
                যদি (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ধ্রুবক packagep = getItemPackage(classp);
                    // Check if it's std
                    যদি (packagep && packagep->name() == "std") {
                        যদি (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } অন্যথা যদি (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } অন্যথা যদি (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            যদি (Ast_গ্রন্থি_FTask* ধ্রুবক ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), গ্রন্থি_FTask)) {
                userIterate(ftaskp, nullptr);
                যদি (ftaskp->isStatic()) {
                    AstArg* ধ্রুবক argsp = nodep->argsp();
                    যদি (argsp) argsp->unlink_Fr_Back_With_পরবর্তী();
                    Ast_গ্রন্থি_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    যদি (!m_vup) {  // Called as task
                        newp = নতুন AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } অন্যথা {
                        newp = নতুন AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                } অন্যথা {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    যদি (VN_IS(ftaskp, Task)) {
                        যদি (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_রাশিমালা)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    যদি (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                ফেরত;
            } অন্যথা যদি (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(সত্য);
                v3Global.useRandomizeMethods(সত্য);
                AstCMethodHard* ধ্রুবক newp
                    = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(সত্য);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            } অন্যথা যদি (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(সত্য);
                v3Global.useRandomizeMethods(সত্য);
                AstCMethodHard* ধ্রুবক newp
                    = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(সত্য);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            } অন্যথা যদি (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(সত্য);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            জন্য (AstClass* classp = first_classp; classp;) {
                জন্য (Ast_গ্রন্থি* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    যদি (VN_IS(itemp, গ্রন্থি_FTask)) speller.ঠেলো_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            ধ্রুবক string suggest = speller.best_Candidate_বার্তা(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    শূন্য methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        যদি (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            যদি (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } অন্যথা {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(সত্য);
        } অন্যথা {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        যদি (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } অন্যথা {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(সত্য);
    }
    শূন্য methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        গণনা : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_যোগ,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        যদি (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } অন্যথা যদি (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } অন্যথা যদি (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } অন্যথা যদি (nodep->name() == "sum") {
            methodId = ARRAY_যোগ;
        } অন্যথা যদি (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        যদি (methodId) {
            AstWith* ধ্রুবক withp
                = methodWithClause(nodep, মিথ্যা, মিথ্যা, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            যদি (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ধ্রুবক newp
                    = নতুন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(মিথ্যা);
                newp->didWidth(সত্য);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                ফেরত;
            } অন্যথা {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_সারি* ধ্রুবক fl = nodep->fileline();
                Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
                জন্য (পূর্ণসংখ্যা i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক arrayRef = nodep->fromp()->clone_বৃক্ষ_Pure(মিথ্যা);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক selector = নতুন AstArraySel{fl, arrayRef, i};
                    যদি (!newp) {
                        newp = selector;
                    } অন্যথা {
                        নির্বাচন (methodId) {
                        ক্ষেত্র ARRAY_OR: newp = নতুন AstOr{fl, newp, selector}; বিরতি;
                        ক্ষেত্র ARRAY_AND: newp = নতুন AstAnd{fl, newp, selector}; বিরতি;
                        ক্ষেত্র ARRAY_XOR: newp = নতুন AstXor{fl, newp, selector}; বিরতি;
                        ক্ষেত্র ARRAY_যোগ: newp = নতুন AstAdd{fl, newp, selector}; বিরতি;
                        ক্ষেত্র ARRAY_PRODUCT: newp = নতুন AstMul{fl, newp, selector}; বিরতি;
                        পূর্বনির্ধারিত: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            }
        } অন্যথা যদি (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(মিথ্যা);
            newp->didWidth(সত্য);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    শূন্য methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        যদি (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* ধ্রুবক callp = নতুন AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    শূন্য methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        যদি (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } অন্যথা যদি (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } অন্যথা যদি (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } অন্যথা যদি (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } অন্যথা যদি (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } অন্যথা যদি (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_গ্রন্থি* ধ্রুবক newp
                = নতুন AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_গ্রন্থি* ধ্রুবক newp
                = নতুন AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "compare" || nodep->name() == "icompare") {
            ধ্রুবক যৌক্তিক ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ধ্রুবক argp = nodep->argsp();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhs = nodep->fromp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhs = argp->exprp()->unlinkFrBack();
            Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ধ্রুবক arg0p = nodep->argsp();
            AstArg* ধ্রুবক arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_গ্রন্থি_Var_Ref* ধ্রুবক fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* ধ্রুবক varrefp
                = নতুন AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstAssign{
                nodep->fileline(), fromp, নতুন AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ঠেলো_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } অন্যথা যদি (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ধ্রুবক arg0p = nodep->argsp();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->fromp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ধ্রুবক arg0p = nodep->argsp();
            AstArg* ধ্রুবক arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->fromp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক thsp = arg1p->exprp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            যদি (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } অন্যথা যদি (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } অন্যথা যদি (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } অন্যথা যদি (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } অন্যথা যদি (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } অন্যথা {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_গ্রন্থি* ধ্রুবক newp
                = নতুন AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_গ্রন্থি_DType* সূচক_DTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        যদি (AstQueueDType* ধ্রুবক queuep = m_queueDTypeIndexed[সূচক_DTypep]) {
            ফেরত queuep;
        } অন্যথা {
            AstQueueDType* ধ্রুবক newp
                = নতুন AstQueueDType{সূচক_DTypep->fileline(), সূচক_DTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[সূচক_DTypep] = newp;
            ফেরত newp;
        }
    }

    শূন্য visit(AstNew* nodep) override {
        যদি (nodep->didWidth()) ফেরত;
        AstClass* classp = nullptr;
        যৌক্তিক assign = মিথ্যা;
        যদি (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = সত্য;
            Ast_গ্রন্থি* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            যদি (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                যদি (AstClass* ধ্রুবক clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ধ্রুবক adtypep
                        = নতুন AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } অন্যথা {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            যদি (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, গ্রন্থি_DType)
                           ? ("data type "s + VN_AS(warnp, গ্রন্থি_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                ফেরত;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            যদি (Ast_গ্রন্থি_FTask* ধ্রুবক ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } অন্যথা {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } অন্যথা {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        যদি (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    শূন্য visit(AstNewCopy* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        AstClassRefDType* ধ্রুবক refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        যদি (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            ফেরত;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        যদি (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    শূন্য visit(AstNewDynamic* nodep) override {
        যদি (nodep->didWidthAndSet()) ফেরত;
        AstDynArrayDType* ধ্রুবক adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        যদি (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            ফেরত;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        যদি (!VN_IS(nodep->backp(), গ্রন্থি_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            ফেরত;
        }
        nodep->dtypep(adtypep);
        যদি (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        যদি (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ধ্রুবক consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            যদি (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        }
    }

    শূন্য visit(Ast_Tagged_রাশিমালা* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    শূন্য visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    শূন্য visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    শূন্য visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    শূন্য visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    শূন্য visit(AstPattern* nodep) override {
        যদি (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            জন্য (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = মিথ্যা;
                user_Iterate_And_পরবর্তী(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                যদি (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), গ্রন্থি_DType)) {
                    user_Iterate_And_পরবর্তী(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                জন্য (Ast_গ্রন্থি *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            ফেরত;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = মিথ্যা;
        যদি (nodep->didWidthAndSet()) ফেরত;
        UINFO(9, "PATTERN " << nodep);
        যদি (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        যদি (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_গ্রন্থি_DType* dtypep = nodep->dtypep();
        যদি (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            যদি (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                ফেরত;
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            যদি (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                ফেরত;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            জন্য (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ধ্রুবক পূর্ণসংখ্যা times = visitPatMemberRep(patp);
                জন্য (পূর্ণসংখ্যা i = 1; i < times; i++) {
                    AstPatMember* ধ্রুবক newp = patp->clone_বৃক্ষ(মিথ্যা);
                    patp->add_পরবর্তী_Here(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            জন্য (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                যদি (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    যখন (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক movep = VN_AS(patp->lhssp()->nextp(), গ্রন্থি_রাশিমালা)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_গ্রন্থি* newkeyp = nullptr;
                        যদি (patp->keyp()) newkeyp = patp->keyp()->clone_বৃক্ষ(সত্য);
                        AstPatMember* ধ্রুবক newp
                            = নতুন AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->add_পরবর্তী(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            জন্য (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ধ্রুবক nextp = VN_AS(patp->nextp(), PatMember);
                যদি (patp->isDefault()) {
                    যদি (defaultp) {
                        VL_DO_DANGLING(ঠেলো_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            যখন (ধ্রুবক AstConstDType* ধ্রুবক vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            যদি (auto* ধ্রুবক vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } অন্যথা যদি (auto* ধ্রুবক vdtypep = VN_CAST(dtypep, গ্রন্থি_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } অন্যথা যদি (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ধ্রুবক consp = নতুন AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } অন্যথা যদি (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ধ্রুবক consp = নতুন AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } অন্যথা যদি (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } অন্যথা যদি (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } অন্যথা {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            যদি (defaultp) VL_DO_DANGLING(ঠেলো_Deletep(defaultp), defaultp);
        }
    }
    শূন্য patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<ধ্রুবক AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ধ্রুবক AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            যখন (patp) {
                করো {
                    যদি (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        যদি (ধ্রুবক AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            যদি (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                বিরতি;
                            } অন্যথা {
                                ধ্রুবক std::pair<PatMap::iterator, যৌক্তিক> ret
                                    = patmap.emplace(memp, patp);
                                যদি (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } অন্যথা যদি (Ast_গ্রন্থি_DType* nodedtypep = VN_CAST(patp->keyp(), গ্রন্থি_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            ধ্রুবক string dtype = nodedtypep->dtypep()->prettyDTypeName(সত্য);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } অন্যথা {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } অন্যথা যদি (memp) {
                        // constant expr
                        ধ্রুবক std::pair<PatMap::iterator, যৌক্তিক> ret = patmap.emplace(memp, patp);
                        যদি (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } অন্যথা {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } যখন (মিথ্যা);

                //পরবর্তী
                যদি (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
        যদি (vdtypep->packed()) {
            জন্য (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ধ্রুবক auto it = patmap.find(memp);
                যদি (it == patmap.end()) {  // default or default_type assignment
                    যদি (AstStructDType* ধ্রুবক memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } অন্যথা {
                        AstPatMember* ধ্রুবক patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = মান_Concat_pattern_Struct(patp, newp, memp, nodep);
                        যদি (patp) VL_DO_DANGLING(ঠেলো_Deletep(patp), patp);
                    }
                } অন্যথা {  // member assignment
                    newp = মান_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } অন্যথা {  // Unpacked
            AstConsStruct* ধ্রুবক consp = নতুন AstConsStruct{nodep->fileline(), vdtypep};
            জন্য (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ধ্রুবক auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                যদি (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    ঠেলো_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ঠেলো_Deletep(patp);  // patp used below
                } অন্যথা {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = pattern_Member_মান_Iterate(patp);
                consp->addItemsp(নতুন AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        যদি (newp) {
            nodep->replaceWith(newp);
        } অন্যথা {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_গ্রন্থি_রাশিমালা* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_গ্রন্থি_রাশিমালা* newp,
                                                 AstPattern* nodep, ধ্রুবক DTypeMap& dtypemap) {
        জন্য (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            যদি (AstStructDType* ধ্রুবক memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } অন্যথা {
                AstPatMember* ধ্রুবক patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = মান_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                যদি (patp) VL_DO_DANGLING(ঠেলো_Deletep(patp), patp);
            }
        }
        ফেরত newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ধ্রুবক DTypeMap& dtypemap) {
        ধ্রুবক string memp_DType = memp->virtRefDTypep()->prettyDTypeName(সত্য);
        ধ্রুবক auto it = dtypemap.find(memp_DType);
        যদি (it != dtypemap.end()) {
            // default_value for data_type
            ফেরত it->second->clone_বৃক্ষ(মিথ্যা);
        }
        যদি (defaultp) {
            // default_value for any unmatched member yet
            ফেরত defaultp->clone_বৃক্ষ(মিথ্যা);
        }
        যদি (memp->valuep()) {
            ফেরত নতুন AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_বৃক্ষ(মিথ্যা), গ্রন্থি_রাশিমালা),
                                    নতুন AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        ফেরত nullptr;
    }

    Ast_গ্রন্থি_রাশিমালা* মান_Concat_pattern_Struct(AstPatMember* patp, Ast_গ্রন্থি_রাশিমালা* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        যদি (patp) {
            patp->dtypep(memp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = pattern_Member_মান_Iterate(patp);
            যদি (!newp) {
                newp = valuep;
            } অন্যথা {
                AstConcat* ধ্রুবক concatp = নতুন AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, সত্য);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, সত্য);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        ফেরত newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_গ্রন্থি_DType* elemDTypep,
                                       ধ্রুবক DTypeMap& dtypemap = DTypeMap{}) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = defaultp->lhssp()->clone_বৃক্ষ(মিথ্যা);
        Ast_গ্রন্থি_DType* ধ্রুবক elemDTypeSkipRefp = elemDTypep->skipRefp();
        ধ্রুবক AstStructDType* ধ্রুবক structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ধ্রুবক যৌক্তিক unpackedStruct = structp && !structp->packed();

        যদি (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            ফেরত defaultp->clone_বৃক্ষ(মিথ্যা);
        }
        যদি (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            ফেরত defaultp->clone_বৃক্ষ(মিথ্যা);
        }
        যদি (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        যৌক্তিক wholeElement = unpackedStruct;
        যদি (Ast_গ্রন্থি_DType* ধ্রুবক মান_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !মান_DTypep->skipRefp()->isIntegralOrPacked()
                                          : মান_DTypep->similarDType(elemDTypeSkipRefp);
        }
        যদি (wholeElement) {
            VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            ফেরত defaultp->clone_বৃক্ষ(মিথ্যা);
        }

        AstPatMember* ধ্রুবক nestedDefaultp
            = নতুন AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(সত্য);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        জন্য (ধ্রুবক auto& entry : dtypemap) nestedDefaultp->add_পরবর্তী(entry.second->clone_বৃক্ষ(মিথ্যা));
        AstPattern* ধ্রুবক recursivePatternp = নতুন AstPattern{defaultp->fileline(), nestedDefaultp};
        ফেরত নতুন AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_গ্রন্থি_রাশিমালা* patternDefaultValuep(AstPatMember* defaultp, Ast_গ্রন্থি_DType* elemDTypep) {
        যদি (!defaultp) ফেরত nullptr;
        AstPatMember* ধ্রুবক elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = pattern_Member_মান_Iterate(elemDefaultp);
        VL_DO_DANGLING(ঠেলো_Deletep(elemDefaultp), elemDefaultp);
        ফেরত valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    শূন্য patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        ধ্রুবক VNumRange range = arrayDtp->declRange();
        ধ্রুবক পূর্ণসংখ্যা elements = range.elements();
        AstConsArray* ধ্রুবক newp = নতুন AstConsArray{nodep->fileline(), arrayDtp};
        পূর্ণসংখ্যা entn = 0;  // Number of elements given so far, from the left
        যৌক্তিক error = মিথ্যা;  // Reported an error, so the value is not complete
        জন্য (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = pattern_Member_মান_Iterate(patp);
            ধ্রুবক uint64_t সূচক = range.ascending() ? entn : elements - 1 - entn;
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            যদি (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_মানসমূহ(সূচক, valuep);
                চালু;
            }
            ধ্রুবক VCastable castable = Ast_গ্রন্থি::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            যদি (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = সত্য;
            }
            newp->valuep(সূচক, valuep);
            ++entn;
        }
        যদি (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = সত্য;
        }
        যদি (error) {
            VL_DO_DANGLING(ঠেলো_Deletep(newp), newp);
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } অন্যথা {
            nodep->replaceWith(newp);
        }
    }
    শূন্য patternArray(AstPattern* nodep, Ast_গ্রন্থি_Array_DType* arrayDtp, AstPatMember* defaultp) {
        যদি (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            ফেরত;
        }
        ধ্রুবক VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.বাম() << " to " << range.ডান());
        Ast_গ্রন্থি* newp = nullptr;
        যৌক্তিক missed = মিথ্যা;  // Some element missed, so not a complete value
        জন্য (পূর্ণসংখ্যা entn = 0, ent = range.বাম(); entn < range.elements();
             ++entn, ent += range.বাম_To_ডান_Inc()) {
            Ast_গ্রন্থি_রাশিমালা* valuep = nullptr;
            ধ্রুবক auto it = patmap.find(ent);
            যদি (it == patmap.end()) {
                যদি (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } অন্যথা {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = সত্য;
                }
            } অন্যথা {
                AstPatMember* ধ্রুবক patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_মান_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ধ্রুবক AstUnpackArrayDType* ধ্রুবক memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                যদি (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                    valuep = নতুন AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            যদি (valuep) {
                যদি (VN_IS(arrayDtp, UnpackArrayDType)) {
                    যদি (!newp) newp = নতুন AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } অন্যথা {  // Packed. Convert to concat for now.
                    যদি (!newp) {
                        newp = valuep;
                    } অন্যথা {
                        AstConcat* ধ্রুবক concatp
                            = নতুন AstConcat{valuep->fileline(), VN_AS(newp, গ্রন্থি_রাশিমালা), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        যদি (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        যদি (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ঠেলো_Deletep(newp), newp);
            newp = নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        যদি (newp) {
            nodep->replaceWith(newp);
        } অন্যথা {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        যদি (VN_IS(patp->keyp(), গ্রন্থি_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            ফেরত nullptr;
        }
        যদি (patp->চাবি_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->চাবি_Exprp());  // May change
        AstConst* ধ্রুবক constp = VN_CAST(patp->চাবি_Exprp(), Const);
        যদি (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        ফেরত constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_গ্রন্থি_DType* চাবি_DTypep) {
        যদি (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            ফেরত nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        যদি (চাবি_DTypep->skipRefp()->isIntegralOrPacked() && patp->চাবি_Exprp()) {
            userIterate(patp->চাবি_Exprp(), WidthVP{চাবি_DTypep, PRELIM}.p());
            ধ্রুবক পূর্ণসংখ্যা width = std::max(patp->চাবি_Exprp()->width(), চাবি_DTypep->width());
            Ast_গ্রন্থি_DType* ধ্রুবক contextDTypep
                = চাবি_DTypep->findBitOrLogicDType(width, width, চাবি_DTypep->numeric(), সত্য);
            iterateCheck(patp, "key", patp->চাবি_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, মিথ্যা);
        }
        AstConst* ধ্রুবক constp = patternConstKeyp(patp);
        যদি (!constp) ফেরত nullptr;
        যদি (constp->অঙ্ক().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            ফেরত nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, চাবি_DTypep, EXTEND_EXP, মিথ্যা);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক convp = patp->চাবি_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        ফেরত VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_গ্রন্থি_রাশিমালা* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_গ্রন্থি_DType* arrayDtp) {
        Ast_গ্রন্থি_DType* ধ্রুবক চাবি_DTypep = VN_AS(arrayDtp, AssocArrayDType)->চাবি_DTypep();
        যদি (!চাবি_DTypep->skipRefp()->isAggregateType() || !patp->চাবি_Exprp()) {
            ফেরত patternScalarKeyp(patp, চাবি_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->চাবি_Exprp(), চাবি_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->চাবি_Exprp());
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক foldedp = patp->চাবি_Exprp();
        যদি (!foldedp->is_Constant_বৃক্ষ()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            ফেরত nullptr;
        }
        যদি (foldedp->exists([](ধ্রুবক AstConst* constp) { ফেরত constp->অঙ্ক().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            ফেরত nullptr;
        }
        ফেরত foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_গ্রন্থি_DType* arrayDtp) {
        ফেরত patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_গ্রন্থি_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        ফেরত patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    শূন্য sort_Cons_বস্তুসমূহ(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        যখন (T_Item* ধ্রুবক itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.ঠেলো_back(itemp);
        }
        ধ্রুবক auto চাবি_Of = [](ধ্রুবক T_Item* itemp) -> ধ্রুবক V3_সংখ্যা& {
            ধ্রুবক Ast_গ্রন্থি* ধ্রুবক keyp = itemp->keyp();
            ফেরত VN_AS(keyp, Const)->অঙ্ক();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](ধ্রুবক T_Item* ap, ধ্রুবক T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            ফেরত চাবি_Of(ap).isLtXZ(চাবি_Of(bp));
        });
        ধ্রুবক T_Item* firstp = nullptr;  // First item with the current key
        জন্য (T_Item* ধ্রুবক itemp : itemps) {
            যদি (firstp && চাবি_Of(firstp).isCaseEq(চাবি_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } অন্যথা {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    শূন্য patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_গ্রন্থি_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        জন্য (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ধ্রুবক auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = pattern_Member_মান_Iterate(patp);
            যদি (keyp) {
                consp->addItemsp(নতুন T_Item{patp->fileline(), keyp, valuep});
            } অন্যথা {
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ধ্রুবক AstAssocArrayDType* ধ্রুবক assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        যদি (!assocDtp || !assocDtp->চাবি_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_বস্তুসমূহ<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_গ্রন্থি_রাশিমালা* newConsDynArrayOrQueue(File_সারি* fileline, ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        যদি (VN_IS(dtypep, DynArrayDType)) ফেরত নতুন AstConsDynArray{fileline};
        যদি (VN_IS(dtypep, QueueDType)) ফেরত নতুন AstConsQueue{fileline};
        ফেরত nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    শূন্য addConsItem(T_Cons* consp, Ast_গ্রন্থি_রাশিমালা* valuep) {
        Ast_গ্রন্থি_DType* ধ্রুবক arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ধ্রুবক যৌক্তিক isElement
            = Ast_গ্রন্থি::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ধ্রুবক AstQueueDType* ধ্রুবক subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        যদি (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* ধ্রুবক subp = Ast_গ্রন্থি::cast<T_Cons>(valuep);
            যদি (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                যদি (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlink_Fr_Back_With_পরবর্তী());
                VL_DO_DANGLING(ঠেলো_Deletep(subp), subp);
                ফেরত;
            }
            যদি (AstConsQueue* ধ্রুবক queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_বস্তুসমূহ(consp, queuep);
                ফেরত;
            }
            যদি (AstConsDynArray* ধ্রুবক dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_বস্তুসমূহ(consp, dynp);
                ফেরত;
            }
        }
        T_Item* ধ্রুবক itemp = নতুন T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        যদি (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } অন্যথা {
            user_Iterate_And_পরবর্তী(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    শূন্য addConsItem(Ast_গ্রন্থি_রাশিমালা* consp, Ast_গ্রন্থি_রাশিমালা* valuep) {
        যদি (AstConsDynArray* ধ্রুবক dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } অন্যথা {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    শূন্য add_Cons_বস্তুসমূহ(Ast_গ্রন্থি_রাশিমালা* consp, T_Sub* subp) {
        যখন (auto* ধ্রুবক itemp = subp->itemsp()) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(ঠেলো_Deletep(subp), subp);
    }
    শূন্য patternDynArrayOrQueue(AstPattern* nodep, Ast_গ্রন্থি_DType* arrayp, AstPatMember* defaultp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ধ্রুবক AstPatMember* ধ্রুবক firstp = VN_AS(nodep->itemsp(), PatMember);
        ধ্রুবক যৌক্তিক keyed = defaultp || (firstp && firstp->keyp());
        যদি (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            ফেরত;
        }
        ধ্রুবক অক্ষর* ধ্রুবক kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        যদি (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        জন্য (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            যদি (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_মান_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    শূন্য patternBasic(AstPattern* nodep, Ast_গ্রন্থি_DType* vdtypep, AstPatMember* defaultp) {
        ধ্রুবক AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ধ্রুবক VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
        জন্য (ধ্রুবক পূর্ণসংখ্যা ent : range.seqHiToLo()) {
            Ast_গ্রন্থি_রাশিমালা* valuep = nullptr;
            ধ্রুবক auto it = patmap.find(ent);
            যদি (it == patmap.end()) {
                যদি (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } অন্যথা {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } অন্যথা {
                AstPatMember* ধ্রুবক patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_মান_Iterate(patp);
            }
            যদি (valuep) {  // Packed. Convert to concat for now.
                যদি (!newp) {
                    newp = valuep;
                } অন্যথা {
                    AstConcat* ধ্রুবক concatp = নতুন AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        যদি (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        যদি (newp) {
            nodep->replaceWith(newp);
        } অন্যথা {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_গ্রন্থি_রাশিমালা* pattern_Member_মান_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_গ্রন্থি_রাশিমালা* valuep = patp->lhssp()->unlinkFrBack();
        যদি (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            যদি (AstConst* ধ্রুবক newp = V3WidthCommit::new_If_Const_Commit_আকার(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        ফেরত valuep;
    }

    স্থির শূন্য checkEventAssignment(ধ্রুবক Ast_গ্রন্থি_Assign* ধ্রুবক asgnp) {
        string unsupEvtAsgn;
        যদি (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        যদি (VN_IS(asgnp->rhsp(), CReset)) ফেরত;
        যদি (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        যদি (!unsupEvtAsgn.empty()) {
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

    স্থির যৌক্তিক usesDynamicScheduler(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        যখন (সত্য) {
            AstVarRef* ধ্রুবক vrefp = VN_CAST(nodep, VarRef);
            যদি (vrefp) ফেরত usesDynamicScheduler(vrefp);
            যদি (VN_IS(nodep, MemberSel)) {
                ফেরত সত্য;
            } অন্যথা যদি (Ast_গ্রন্থি_Sel* selp = VN_CAST(nodep, গ্রন্থি_Sel)) {
                nodep = selp->fromp();
            } অন্যথা {
                ফেরত মিথ্যা;
            }
        }
    }

    স্থির যৌক্তিক usesDynamicScheduler(AstVarRef* vrefp) {
        ফেরত VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    শূন্য visit(AstPatMember* nodep) override {
        Ast_গ্রন্থি_DType* ধ্রুবক vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    পূর্ণসংখ্যা visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        যদি (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->repp(), Const);
            যদি (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } অন্যথা {
                times = constp->toUInt();
            }
            যদি (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlink_Fr_Back_With_পরবর্তী()
                ->delete_বৃক্ষ();  // Done with replicate before cloning
        }
        ফেরত times;
    }

    শূন্য visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_গ্রন্থি* propp = nodep->propp();
            যখন (AstLogNot* ধ্রুবক notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ধ্রুবক untilp = VN_CAST(propp, Until);
            যদি (untilp && VN_IS(nodep->backp(), গ্রন্থি_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(নতুন AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ঠেলো_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            user_Iterate_And_পরবর্তী(nodep->sensesp(), nullptr);
            যদি (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    যৌক্তিক firstNewStatementOkRecurse(Ast_গ্রন্থি* nodep) {
        যদি (AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
            যদি (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) ফেরত সত্য;
        }
        যদি (AstAssign* ধ্রুবক aitemp = VN_CAST(nodep, Assign)) {
            যদি (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) ফেরত সত্য;
        }
        যদি (AstInitialStaticStmt* ধ্রুবক aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            জন্য (Ast_গ্রন্থি* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                যদি (!firstNewStatementOkRecurse(stmtp)) ফেরত মিথ্যা;
            }
            ফেরত সত্য;
        }
        যদি (AstInitialAutomaticStmt* ধ্রুবক aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            জন্য (Ast_গ্রন্থি* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                যদি (!firstNewStatementOkRecurse(stmtp)) ফেরত মিথ্যা;
            }
            ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    শূন্য handleCaseType(Ast_গ্রন্থি* casep, Ast_গ্রন্থি_রাশিমালা* exprp, CaseItem* itemsp) {
        AstAttrOf* ধ্রুবক exprap = VN_CAST(exprp, AttrOf);
        যদি (!exprap) ফেরত;
        যদি (exprap->attrType() != VAttrType::TYPEID) ফেরত;

        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক রাশিমালা_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << রাশিমালা_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_সারি* ধ্রুবক newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, সত্য);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, সত্য);  // Side effect of transform
        casep->fileline(newfl);
        জন্য (CaseItem* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<CaseItem>(itemp->nextp())) {
            যদি (itemp->isDefault()) চালু;
            যৌক্তিক hit = মিথ্যা;
            জন্য (Ast_গ্রন্থি* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ধ্রুবক AstAttrOf* ধ্রুবক condAttrp = VN_CAST(condp, AttrOf);
                যদি (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } অন্যথা {
                    Ast_গ্রন্থি_DType* ধ্রুবক condDtp = condAttrp->dtypep();
                    যদি (Ast_গ্রন্থি::computeCastable(রাশিমালা_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = সত্য;
                        বিরতি;
                    }
                }
            }
            ঠেলো_Deletep(itemp->condsp()->unlink_Fr_Back_With_পরবর্তী());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(নতুন AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(নতুন AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(ঠেলো_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    শূন্য handleCase(Ast_গ্রন্থি* casep, Ast_গ্রন্থি_রাশিমালা* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_গ্রন্থি_DType* subDTypep = exprp->dtypep();
        জন্য (CaseItem* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<CaseItem>(itemp->nextp())) {
            জন্য (Ast_গ্রন্থি* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                যদি (condp->dtypep() == subDTypep) চালু;

                যদি (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } অন্যথা যদি (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } অন্যথা {
                    ধ্রুবক পূর্ণসংখ্যা width = std::max(subDTypep->width(), condp->width());
                    ধ্রুবক পূর্ণসংখ্যা mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    ধ্রুবক যৌক্তিক issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        জন্য (CaseItem* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<CaseItem>(itemp->nextp())) {
            জন্য (Ast_গ্রন্থি *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    শূন্য visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        জন্য (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            জন্য (Ast_গ্রন্থি *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    শূন্য visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_পরবর্তী(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        user_Iterate_And_পরবর্তী(nodep->incsp(), nullptr);
    }
    শূন্য visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    শূন্য visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        জন্য (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            user_Iterate_And_পরবর্তী(itemp->stmtsp(), nullptr);
            জন্য (Ast_গ্রন্থি *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    শূন্য visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_গ্রন্থি_DType* ধ্রুবক itemDTypep = nodep->findUInt32DType();
        জন্য (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            user_Iterate_And_পরবর্তী(itemp->stmtsp(), nullptr);
            জন্য (Ast_গ্রন্থি *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    শূন্য visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        user_Iterate_And_পরবর্তী(nodep->stmtsp(), nullptr);
    }
    শূন্য visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    শূন্য visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        user_Iterate_And_পরবর্তী(nodep->thensp(), nullptr);
        user_Iterate_And_পরবর্তী(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        user_Iterate_And_পরবর্তী(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    শূন্য visit(Ast_গ্রন্থি_Foreach* nodep) override {
        যদি (nodep->didWidth()) ফেরত;
        nodep->didWidth(সত্য);
        ধ্রুবক AstForeachHeader* ধ্রুবক headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        user_Iterate_And_পরবর্তী(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_গ্রন্থি_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        জন্য (Ast_গ্রন্থি *argsp = headerp->elementsp(), *পরবর্তী_argsp; argsp; argsp = পরবর্তী_argsp) {
            পরবর্তী_argsp = argsp->nextp();
            ধ্রুবক যৌক্তিক empty = VN_IS(argsp, Empty);
            AstVar* ধ্রুবক varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            যদি (varp) varp->usedLoopIdx(সত্য);
            যদি (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                ফেরত;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            যদি (VN_IS(fromDtp, গ্রন্থি_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } অন্যথা যদি (AstBasicDType* ধ্রুবক adtypep = VN_CAST(fromDtp, BasicDType)) {
                যদি (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                    ফেরত;
                }
            } অন্যথা যদি (ধ্রুবক AstAssocArrayDType* ধ্রুবক adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->চাবি_DTypep());
            } অন্যথা {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                ফেরত;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_গ্রন্থি* ধ্রুবক bodyp = nodep->bodyp();
        user_Iterate_And_পরবর্তী(bodyp, nullptr);
        যদি (AstForeach* ধ্রুবক loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            ফেরত;
        }
    }

    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
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
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            যদি (ধ্রুবক AstIfaceRefDType* ধ্রুবক irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                যদি (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                    ফেরত;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            streamAssignLower(nodep);
            Ast_গ্রন্থি_DType* ধ্রুবক lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            যদি (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক lhsDtp = lhsDTypep->skipRefp();
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ধ্রুবক যৌক্তিক lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ধ্রুবক যৌক্তিক rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                যদি (lhsIsUnpackArray && rhsIsUnpackArray) {
                    যদি (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_গ্রন্থি_Var_Ref, Ast_গ্রন্থি_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), গ্রন্থি_Var_Ref), VN_CAST(nodep->rhsp(), গ্রন্থি_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                যদি (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), গ্রন্থি_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        যদি (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        যদি (Ast_গ্রন্থি* ধ্রুবক controlp = nodep->timingControlp()) {
            যদি (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlink_Fr_Back_With_পরবর্তী()->delete_বৃক্ষ(), controlp);
            } অন্যথা যদি (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } অন্যথা {
                যদি (nodep->fileline()->timingOn()) {
                    যদি (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } অন্যথা {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlink_Fr_Back_With_পরবর্তী()->delete_বৃক্ষ(), controlp);
            }
        }
        যদি (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            যদি (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                ফেরত;
            }
            AstMethodCall* ধ্রুবক newp
                = নতুন AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            ফেরত;
        }
        যদি (ধ্রুবক AstNewDynamic* ধ্রুবক dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            যদি (!dynp->rhsp()) {
                newp = নতুন AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } অন্যথা {
                newp = নতুন AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(সত্য);
            newp->protect(মিথ্যা);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        যদি (AstCMethodHard* ধ্রুবক slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            যদি (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } অন্যথা যদি (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } অন্যথা যদি (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } অন্যথা {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            যদি (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক msbp = slicep->pinsp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* ধ্রুবক newp
                    = নতুন AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(সত্য);
                newp->protect(মিথ্যা);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            }
        }

        যদি (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    শূন্য visit(AstRSRule* nodep) override {
        যদি (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        user_Iterate_And_পরবর্তী(nodep->prodlistsp(), nullptr);
        user_Iterate_And_পরবর্তী(nodep->weightStmtsp(), nullptr);
    }

    শূন্য visit(AstRelease* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    শূন্য visit(AstDeassign* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    স্থির যৌক্তিক isFormatNonNumericArg(ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ফেরত dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, গ্রন্থি_UOr_Struct_DType)
                   && !VN_AS(dtypep, গ্রন্থি_UOr_Struct_DType)->packed());
    }

    শূন্য checkFormatNumericArg(Ast_গ্রন্থি* argp, অক্ষর ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        যদি (std::tolower(ch) == 'd') ফেরত;
        // In contrast, %x of a class reference, causes errors on several simulators.
        যদি (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    শূন্য visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    শূন্য visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_পরবর্তী(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    শূন্য visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        যদি (m_vup && !m_vup->prelim()) ফেরত;  // Can be called as statement or function
        যদি (nodep->didWidthAndSet()) ফেরত;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        যদি (!nodep->রাশিমালা_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_গ্রন্থি_রাশিমালা* oldExprsp = nodep->exprsp();
        যদি (nodep->রাশিমালা_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), গ্রন্থি_রাশিমালা);
        }
        যদি (oldExprsp) oldExprsp->unlink_Fr_Back_With_পরবর্তী();
        যখন (Ast_গ্রন্থি_রাশিমালা* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), গ্রন্থি_রাশিমালা);
            যদি (oldExprsp) oldExprsp->unlink_Fr_Back_With_পরবর্তী();
            যদি (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                চালু;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_গ্রন্থি_DType* ধ্রুবক dtypep = argp->dtypep()->skipRefp();
            যদি (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } অন্যথা যদি (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } অন্যথা যদি (isFormatNonNumericArg(dtypep)) {
                যদি (AstClassRefDType* ধ্রুবক classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    যদি (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(সত্য);
                    }
                } অন্যথা {
                    // Class handles inside containers print type names, not their members.
                    Ast_গ্রন্থি_DType* গ্রন্থি_Dtypep = dtypep;
                    যখন (গ্রন্থি_Dtypep) {
                        গ্রন্থি_Dtypep = গ্রন্থি_Dtypep->skipRefp();
                        যদি (Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক uOrStructDTypep
                            = VN_CAST(গ্রন্থি_Dtypep, গ্রন্থি_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(সত্য);
                        }
                        গ্রন্থি_Dtypep = গ্রন্থি_Dtypep->subDTypep();
                    }
                }
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } অন্যথা যদি (nodep->রাশিমালা_Format_2()) {
                যদি (AstEnumDType* ধ্রুবক enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    চালু;
                }
            }
            যদি (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            যদি (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } অন্যথা {
                nodep->addExprsp(নতুন AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    শূন্য visit(AstCReset* nodep) override {
        assert_At_রাশিমালা(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    শূন্য visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    শূন্য visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    শূন্য visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        যদি (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        যদি (!m_paramsOnly) {
            nodep->foreach([এই](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    নতুন AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            যদি (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            নির্বাচন (nodep->displayType()) {
            ক্ষেত্র VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); বিরতি;
            ক্ষেত্র VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); বিরতি;
            ক্ষেত্র VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); বিরতি;
            ক্ষেত্র VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); বিরতি;
            পূর্বনির্ধারিত: UASSERT_OBJ(মিথ্যা, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        }
    }
    শূন্য visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        যদি (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    শূন্য visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    শূন্য visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    শূন্য visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    শূন্য visit(AstFError* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            user_Iterate_And_পরবর্তী(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(AstFEof* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    শূন্য visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        যদি (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    শূন্য visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    শূন্য visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    শূন্য visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    শূন্য visit(AstFGetC* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    শূন্য visit(AstFGetS* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_পরবর্তী(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(AstFUngetC* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    শূন্য visit(AstFRead* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            যদি (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            যদি (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    শূন্য visit(AstFScanF* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_পরবর্তী(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(AstSScanF* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_পরবর্তী(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(Ast_স্তূপ_Trace_F* nodep) override { nodep->dtypeSetString(); }
    শূন্য visit(AstSysIgnore* nodep) override {
        user_Iterate_And_পরবর্তী(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstSystemF* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(Ast_গ্রন্থি_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        user_Iterate_And_পরবর্তী(nodep->memp(), WidthVP{SELF, BOTH}.p());
        ধ্রুবক Ast_গ্রন্থি_DType* subp = nullptr;
        যদি (ধ্রুবক AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            যদি (!adtypep->চাবি_DTypep()->skipRefp()->basicp()
                || !adtypep->চাবি_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } অন্যথা যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } অন্যথা যদি (ধ্রুবক AstDynArrayDType* ধ্রুবক adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } অন্যথা {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        যদি (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        user_Iterate_And_পরবর্তী(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_পরবর্তী(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstTestPlusArgs* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    শূন্য visit(Ast_মান_Plus_Args* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            user_Iterate_And_পরবর্তী(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        যদি (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        যদি (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        যদি (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        যদি (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    শূন্য visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_পরবর্তী(nodep->passsp(), nullptr);
        user_Iterate_And_পরবর্তী(nodep->failsp(), nullptr);
    }
    শূন্য visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_পরবর্তী(nodep->passsp(), nullptr);
        user_Iterate_And_পরবর্তী(nodep->failsp(), nullptr);
    }
    শূন্য visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_পরবর্তী(nodep->passsp(), nullptr);
    }
    শূন্য visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    যৌক্তিক pinInstArrayElement(AstPin* nodep, ধ্রুবক AstRange* rangep, পূর্ণসংখ্যা idx) {
        Ast_গ্রন্থি_DType* ধ্রুবক modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_গ্রন্থি_DType* ধ্রুবক conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        যদি (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) ফেরত সত্য;
        // Types don't match, but all dimensions done, still need the usual checks
        যদি (!rangep) ফেরত মিথ্যা;

        ধ্রুবক AstRange* ধ্রুবক subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        পূর্ণসংখ্যা subElems = 1;
        জন্য (ধ্রুবক AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ধ্রুবক পূর্ণসংখ্যা elements = rangep->elementsConst();
        ধ্রুবক পূর্ণসংখ্যা from_বাম = idx / subElems;

        ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ধ্রুবক uint32_t conDims = conDTypep->dimensions(মিথ্যা).second;
        ধ্রুবক uint32_t modDims = modDTypep->dimensions(মিথ্যা).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        যদি (arrp && conDims != modDims) {
            যদি (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                user_Iterate_And_পরবর্তী(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                ফেরত সত্য;
            }
            // ArraySel index is counted from lo
            ধ্রুবক পূর্ণসংখ্যা bit = arrp->declRange().ascending() ? from_বাম : elements - 1 - from_বাম;
            // Connection is self-determined
            user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = VN_AS(nodep->exprp(), গ্রন্থি_রাশিমালা)->unlinkFrBack();
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ঠেলো_Deletep(fromp), fromp);
            } অন্যথা {
                AstArraySel* ধ্রুবক selp = নতুন AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(সত্য);
                nodep->exprp(selp);
            }
            ফেরত pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        যদি (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) ফেরত মিথ্যা;

        // Packed connection
        ধ্রুবক পূর্ণসংখ্যা modwidth = modDTypep->width();
        ধ্রুবক পূর্ণসংখ্যা conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        যদি (conwidth == modwidth) ফেরত মিথ্যা;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        যদি (conwidth == modwidth * elements * subElems) {
            যদি (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->বাম_Const() << ":"
                                                     << rangep->ডান_Const() << "]");
            }
            ধ্রুবক পূর্ণসংখ্যা selwidth = modwidth * subElems;
            // Connection is self-determined
            user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = VN_AS(nodep->exprp(), গ্রন্থি_রাশিমালা)->unlinkFrBack();
            AstSel* ধ্রুবক selp = নতুন AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_বাম), selwidth};
            selp->didWidth(সত্য);
            nodep->exprp(selp);
            ফেরত pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        ফেরত সত্য;
    }

    শূন্য visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        যদি (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ধ্রুবক concatp = VN_CAST(nodep->exprp(), Replicate);
            ধ্রুবক AstConst* ধ্রুবক countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            যদি (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* ধ্রুবক patternp = নতুন AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ঠেলো_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } অন্যথা যদি (!m_paramsOnly) {
            যদি (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            যদি (!nodep->exprp()) {  // No-connect
                যদি (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                ফেরত;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ধ্রুবক VDirection pinDirection = nodep->modVarp()->direction();
            যদি (VN_IS(nodep->exprp(), গ্রন্থি_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_গ্রন্থি_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_গ্রন্থি_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            যদি (ধ্রুবক AstIfaceRefDType* ধ্রুবক modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ধ্রুবক AstIfaceRefDType* ধ্রুবক conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                যদি (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_গ্রন্থি_DType* subDTypep = modDTypep;
            ধ্রুবক পূর্ণসংখ্যা modwidth = modDTypep->width();
            পূর্ণসংখ্যা conwidth = conDTypep->width();
            যদি (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                user_Iterate_And_পরবর্তী(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } অন্যথা {
                যদি (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    যদি (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        যদি (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        ফেরত;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                যদি (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } অন্যথা যদি (nodep->modVarp()->isTristate()) {
                    যদি (modwidth != conwidth) {
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
                } অন্যথা যদি (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                যদি ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক রাশিমালা_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                যদি (রাশিমালা_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpacked_Array_আকার(conDTypep) != unpacked_Array_আকার(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpacked_Array_আকার(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpacked_Array_আকার(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } অন্যথা যদি ((রাশিমালা_Arrayp && !modArrayp) || (!রাশিমালা_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (রাশিমালা_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } অন্যথা {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_গ্রন্থি_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), গ্রন্থি_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            যদি (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    শূন্য visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        যদি (!m_paramsOnly) {
            যদি (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_বার্তা(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            যদি (nodep->rangep()) user_Iterate_And_পরবর্তী(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_পরবর্তী(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            যদি (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ঠেলো_Deletep(nodep->rangep()->unlink_Fr_Back_With_পরবর্তী());
            }
        }
        user_Iterate_And_পরবর্তী(nodep->paramsp(), nullptr);
    }
    শূন্য visit(AstGatePin* nodep) override {
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক pinDTypep = nodep->exprp()->dtypep();
            ধ্রুবক পূর্ণসংখ্যা অঙ্ক_Insts = nodep->rangep()->elementsConst();
            ধ্রুবক পূর্ণসংখ্যা modwidth = অঙ্ক_Insts;
            ধ্রুবক পূর্ণসংখ্যা pinwidth = pinDTypep->width();
            যদি (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_গ্রন্থি* ধ্রুবক newp
                    = নতুন AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(অঙ্ক_Insts)};
                nodep->replaceWith(newp);
            } অন্যথা {
                যদি (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                user_Iterate_And_পরবর্তী(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            }
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        যদি (nodep->didWidth()) ফেরত;
        যদি (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(সত্য);
            nodep->didWidth(সত্য);
            ফেরত;
        }
        যদি (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } অন্যথা যদি (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        যদি (nodep->classMethod() && nodep->name() == "new") {
            যদি (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            যদি (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_গ্রন্থি* firstp = nullptr;
            জন্য (Ast_গ্রন্থি* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                যদি (Ast_Stmt_রাশিমালা* ধ্রুবক sep = VN_CAST(itemp, Stmt_রাশিমালা)) {
                    যদি (AstNew* ধ্রুবক newp = VN_CAST(sep->exprp(), New)) {
                        যদি (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            বিরতি;
                        }
                    }
                    চালু;
                }
                যদি (firstNewStatementOkRecurse(itemp)) চালু;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(সত্য);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        যদি (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        যদি (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } অন্যথা যদি (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } অন্যথা যদি (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        যদি (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(সত্য);
        nodep->doingWidth(মিথ্যা);
        যদি (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        যদি (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->exists_And_পরবর্তী([&](ধ্রুবক AstVarRef* varrefp) {
                   ফেরত varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        জন্য (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুবক portp = VN_CAST(stmtp, Var)) {
                যদি (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } অন্যথা যদি (AstAttrOf* ধ্রুবক attrp = VN_CAST(stmtp, AttrOf)) {
                যদি (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                যদি (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        যদি (protop) {
            জন্য (আকার_t i = 0; i < std::max(ports.আকার_2(), protos.আকার_2()); ++i) {
                যদি (i >= ports.আকার_2() || i >= protos.আকার_2()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    বিরতি;
                } অন্যথা {
                    AstVar* ধ্রুবক portp = ports[i];
                    AstAttrOf* ধ্রুবক rprotop = protos[i];
                    Ast_গ্রন্থি_DType* ধ্রুবক declDtp = portp->dtypep();
                    Ast_গ্রন্থি_DType* ধ্রুবক protoDtp = rprotop->fromp()->dtypep();
                    যদি (portp->name() != rprotop->name()) {
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
                    } অন্যথা যদি (!similarDTypeRecurse(protoDtp, declDtp)) {
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
    শূন্য visit(AstConstraint* nodep) override {
        যদি (nodep->didWidth()) ফেরত;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(সত্য);
    }
    শূন্য visit(AstProperty* nodep) override {
        যদি (nodep->didWidth()) ফেরত;
        যদি (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(সত্য);
            nodep->didWidth(সত্য);
            ফেরত;
        }
        nodep->doingWidth(সত্য);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        জন্য (Ast_গ্রন্থি* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            যদি (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } অন্যথা যদি (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } অন্যথা যদি (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } অন্যথা {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(সত্য);
        nodep->doingWidth(মিথ্যা);
    }
    শূন্য visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    শূন্য visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        যদি (nodep->didWidth()) ফেরত;
        nodep->dtypeSetBit();
        জন্য (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } অন্যথা যদি (VN_IS(stmtp, গ্রন্থি_রাশিমালা)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } অন্যথা {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(সত্য);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_গ্রন্থি* pkgItemp) {
        যখন (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        ফেরত VN_CAST(pkgItemp->backp(), Package);
    }
    শূন্য visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_গ্রন্থি_FTask_Ref*>(nodep));
        যদি (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            যদি (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_রাশিমালা))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } অন্যথা {
            nodep->dtypeFrom(nodep->taskp());
        }
        যদি (nodep->fileline()->timingOn()) {
            Ast_গ্রন্থি_Module* ধ্রুবক classp = nodep->classOrPackagep();
            যদি (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ধ্রুবক packagep = getItemPackage(classp);
                যদি (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    স্থির যৌক্তিক are_Same_আকার(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ধ্রুবক std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ধ্রুবক std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        যদি (dims0.আকার_2() != dims1.আকার_2()) ফেরত মিথ্যা;
        জন্য (আকার_t i = 0; i < dims0.আকার_2(); ++i) {
            যদি (dims0[i]->elementsConst() != dims1[i]->elementsConst()) ফেরত মিথ্যা;
        }
        ফেরত সত্য;
    }
    // Makes sure that port and pin have same size and same datatype
    শূন্য checkUnpackedArrayArgs(AstVar* portp, Ast_গ্রন্থি* pinp) {
        যদি (AstUnpackArrayDType* ধ্রুবক portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            যদি (AstUnpackArrayDType* ধ্রুবক pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                যদি (!are_Same_আকার(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                ধ্রুবক AstBasicDType* ধ্রুবক portBasicp = portDtypep->basicp();
                ধ্রুবক AstBasicDType* ধ্রুবক pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ধ্রুবক যৌক্তিক sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                যদি (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } অন্যথা {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    শূন্য processFTaskRefArgs(Ast_গ্রন্থি_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        যদি (nodep->didWidth()) ফেরত;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        করো {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ধ্রুবক V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            যদি (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            জন্য (ধ্রুবক auto& tconnect : tconnects) {
                ধ্রুবক AstVar* ধ্রুবক portp = tconnect.first;
                AstArg* ধ্রুবক argp = tconnect.second;
                Ast_গ্রন্থি_রাশিমালা* pinp = argp->exprp();
                যদি (!pinp) চালু;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                যদি (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlink_Fr_Back_With_পরবর্তী(&handle);  // Format + additional args, if any
                    Ast_গ্রন্থি_রাশিমালা* argsp = nullptr;
                    যখন (AstArg* ধ্রুবক nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->add_পরবর্তী(nextargp->exprp()->unlink_Fr_Back_With_পরবর্তী());
                        nextargp->unlinkFrBack()->delete_বৃক্ষ();  // Remove the call's Arg wrapper
                    }
                    string format;
                    যদি (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->অঙ্ক().toString();
                    } অন্যথা {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ঠেলো_Deletep(argp), argp);
                    AstSFormatF* ধ্রুবক newp
                        = নতুন AstSFormatF{nodep->fileline(), format, মিথ্যা, argsp};
                    যদি (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(নতুন AstScopeName{newp->fileline(), সত্য});
                    }
                    handle.relink(নতুন AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    যাও reloop;  // so exit early; next loop will correct it
                }  //
                অন্যথা যদি (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                যদি (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } যখন (মিথ্যা);
        // Stage 2
        {
            ধ্রুবক V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            জন্য (ধ্রুবক auto& tconnect : tconnects) {
                AstVar* ধ্রুবক portp = tconnect.first;
                ধ্রুবক AstArg* ধ্রুবক argp = tconnect.second;
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক pinp = argp->exprp();
                যদি (!pinp) চালু;  // Argument error we'll find later
                // Change data types based on above accept completion
                যদি (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                যদি (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ধ্রুবক V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            জন্য (ধ্রুবক auto& tconnect : tconnects) {
                ধ্রুবক AstVar* ধ্রুবক portp = tconnect.first;
                ধ্রুবক AstArg* ধ্রুবক argp = tconnect.second;
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক pinp = argp->exprp();
                যদি (!pinp) চালু;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        যদি (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ধ্রুবক V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            জন্য (ধ্রুবক auto& tconnect : tconnects) {
                ধ্রুবক AstVar* ধ্রুবক portp = tconnect.first;
                ধ্রুবক AstArg* ধ্রুবক argp = tconnect.second;
                Ast_গ্রন্থি_রাশিমালা* pinp = argp->exprp();
                যদি (!pinp) চালু;  // Argument error we'll find later
                Ast_গ্রন্থি_DType* ধ্রুবক portDTypep = portp->dtypep()->skipRefToEnump();
                যদি (VN_IS(pinp, EmptyQueue)) {
                    Ast_গ্রন্থি_রাশিমালা* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    যদি (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = নতুন AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক pinDTypep = pinp->dtypep()->skipRefToEnump();
                ধ্রুবক AstIfaceRefDType* ধ্রুবক portIfacep
                    = VN_CAST(portDTypep->elemDTypep(সত্য), IfaceRefDType);
                ধ্রুবক VCastable ifaceCastable
                    = portIfacep ? Ast_গ্রন্থি::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ সত্য)
                                 : VCastable{VCastable::UNSUPPORTED};
                ধ্রুবক যৌক্তিক matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                ধ্রুবক যৌক্তিক pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->অঙ্ক().isNull();
                যদি ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    যদি (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } অন্যথা {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } অন্যথা যদি (portIfacep && portIfacep->isVirtual() && portp->is_প্রবেশ() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } অন্যথা যদি (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                যদি (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    চালু;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                যদি (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } অন্যথা যদি (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } অন্যথা {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    শূন্য handleStdRandomizeArgs(Ast_গ্রন্থি_FTask_Ref* ধ্রুবক nodep) {
        AstConst* nullp = nullptr;
        জন্য (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = argp->exprp();
            যদি (AstConst* ধ্রুবক constp = VN_CAST(exprp, Const)) {
                যদি (constp->অঙ্ক().isNull()) {
                    nullp = constp;
                    চালু;
                }
            }
            যদি (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                চালু;
            }
            যদি (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                চালু;
            }
            যদি (ধ্রুবক AstCMethodHard* ধ্রুবক methodp = VN_CAST(exprp, CMethodHard)) {
                যদি (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    চালু;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        যদি (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        যদি (nodep->didWidth()) ফেরত;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        যদি (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(সত্য);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            ফেরত;  // Handled in V3Randomize
        } অন্যথা যদি (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ধ্রুবক classp = VN_CAST(nodep->classOrPackagep(), Class);
            যদি (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(সত্য);
                Ast_গ্রন্থি_DType* ধ্রুবক adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, মিথ্যা, মিথ্যা, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                জন্য (ধ্রুবক AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_পরবর্তী(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                যদি (withp) nodep->withp(withp);
                nodep->didWidth(সত্য);
                ফেরত;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            যদি (nodep->name() == "randomize") {
                AstClassRefDType* ধ্রুবক adtypep
                    = নতুন AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, মিথ্যা, মিথ্যা, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                জন্য (ধ্রুবক AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_পরবর্তী(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } অন্যথা যদি (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } অন্যথা যদি (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(সত্য);
                v3Global.useRandomizeMethods(সত্য);
                AstCExpr* ধ্রুবক newp
                    = নতুন AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            } অন্যথা যদি (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(সত্য);
                v3Global.useRandomizeMethods(সত্য);
                AstCExpr* ধ্রুবক newp
                    = নতুন AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            } অন্যথা {
                UASSERT_OBJ(মিথ্যা, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        যদি (nodep->didWidth()) {
            যদি (withp) nodep->withp(withp);
            ফেরত;
        }
        যদি ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            যৌক্তিক allow = মিথ্যা;
            যদি (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                যদি (ধ্রুবক AstFuncRef* ধ্রুবক funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } অন্যথা যদি (ধ্রুবক AstTaskRef* ধ্রুবক taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                যদি (!allow) {
                    ধ্রুবক AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    যদি (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    ধ্রুবক AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    যদি (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            যদি (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ধ্রুবক AstClass* ধ্রুবক cgClassp = m_containingClassFinder.find(nodep);
                যদি (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ধ্রুবক AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    যদি (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            যদি (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        যদি (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(নতুন AstScopeName{nodep->fileline(), মিথ্যা});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        যদি (withp) nodep->withp(withp);
        nodep->didWidth(সত্য);
        // See steps that follow in visit(AstFuncRef*)
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    শূন্য visit(AstSenItem* nodep) override {
        যদি (nodep->isComboStar()) ফেরত;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        যদি (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* ধ্রুবক concatOrReplp = VN_CAST(nodep->sensp(), গ্রন্থি_Biop);
            auto* ধ্রুবক rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            যদি (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* ধ্রুবক lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->add_পরবর্তী_Here(নতুন AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(নতুন AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            যদি (nodep->edgeType().anEdge()) {
                Ast_গ্রন্থি_DType* ধ্রুবক sensDtp = nodep->sensp()->dtypep()->skipRefp();
                যদি (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } অন্যথা যদি (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    শূন্য visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ধ্রুবক refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    শূন্য visit(AstWait* nodep) override {
        যদি (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }
        যদি (nodep->fileline()->timingOn()) {
            যদি (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুবক varrefp = VN_CAST(nodep->condp(), গ্রন্থি_Var_Ref)) {
                    যদি (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                ফেরত;
            } অন্যথা যদি (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } অন্যথা {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        যদি (Ast_গ্রন্থি* ধ্রুবক stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_Back_With_পরবর্তী();
            nodep->replaceWith(stmtsp);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_গ্রন্থি_DType* ধ্রুবক vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->সূচক_Arg_Refp(), nullptr);
        userIterateChildren(nodep->মান_Arg_Refp(), nullptr);
        যদি (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } অন্যথা {
            যদি (!nodep->exprp()->hasDType()) {
                user_Iterate_And_পরবর্তী(nodep->exprp(), nullptr);
            } অন্যথা {
                যদি (vdtypep) {
                    user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } অন্যথা {  // 'sort with' allows arbitrary type
                    user_Iterate_And_পরবর্তী(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            যদি (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } অন্যথা {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    শূন্য visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        যদি (nodep->সূচক()) {
            nodep->dtypeFrom(m_withp->সূচক_Arg_Refp());
        } অন্যথা {
            nodep->dtypeFrom(m_withp->মান_Arg_Refp());
        }
    }
    শূন্য visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা* nodep) override {
        যদি (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    শূন্য visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        যদি (nodep->didWidthAndSet()) ফেরত;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        যদি (Ast_গ্রন্থি_DType* ধ্রুবক vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } অন্যথা যদি (!nodep->dtypep()) {
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
        user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, FINAL}.p());
        nodep->didWidth(সত্য);
    }
    শূন্য visitClass(AstClass* nodep) {
        যদি (nodep->didWidthAndSet()) ফেরত;

        // If the class is std::process
        যদি (nodep->name() == "process") {
            AstPackage* ধ্রুবক packagep = getItemPackage(nodep);
            যদি (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                যদি (AstVar* ধ্রুবক varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_গ্রন্থি_DType* ধ্রুবক dtypep = varp->getChildDTypep();
                    যদি (!varp->dtypep()) {
                        VL_DO_DANGLING(ঠেলো_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* ধ্রুবক newdtypep = নতুন AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                যদি (Ast_গ্রন্থি_FTask* ধ্রুবক ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), গ্রন্থি_FTask)) {
                    ftaskp->setNeedProcess();
                    v3Global.setUsesTiming();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        user_Iterate_And_পরবর্তী(nodep->extendsp(), nullptr);
        userIterateChildren(nodep, nullptr);  // First size all members
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enum_অস্থায়ী_Names);
        VL_RESTORER_COPY(m_inside_অস্থায়ী_Names);
        যদি (AstClass* ধ্রুবক classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } অন্যথা {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([এই](ধ্রুবক AstVar* varp) { m_curModVars.ঢোকাও(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    শূন্য visit_Or_Lu64(Ast_গ্রন্থি_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    শূন্য visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            যদি (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    নতুন AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            }
        }
    }
    শূন্য visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    শূন্য visit_Os32_Lr(Ast_গ্রন্থি_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    শূন্য visit_Ou64_Lr(Ast_গ্রন্থি_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    শূন্য visit_log_not(AstLogNot* nodep) {
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
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            যদি (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ধ্রুবক newp
                    = নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        }
    }
    শূন্য visit_log_and_or(Ast_গ্রন্থি_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_গ্রন্থি_রাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    শূন্য visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit_red_and_or(Ast_গ্রন্থি_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    শূন্য visit_red_unknown(Ast_গ্রন্থি_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    যৌক্তিক isEquivalentDType(ধ্রুবক Ast_গ্রন্থি_DType* lhs, ধ্রুবক Ast_গ্রন্থি_DType* rhs) {
        // a) If two types match, they are equivalent.
        যদি (!lhs || !rhs) ফেরত মিথ্যা;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        যদি (lhs == rhs) ফেরত সত্য;
        // If both are basic types, check if they are the same type
        যদি (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ধ্রুবক auto* lb = VN_CAST(lhs, BasicDType);
            ধ্রুবক auto* rb = VN_CAST(rhs, BasicDType);
            যদি (lb->isString() != rb->isString()) ফেরত মিথ্যা;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ধ্রুবক যৌক্তিক lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ধ্রুবক যৌক্তিক rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        যদি (lhsIsUnpackArray || rhsIsUnpackArray) {
            যদি (lhsIsUnpackArray && rhsIsUnpackArray) {
                ধ্রুবক AstUnpackArrayDType* ধ্রুবক lhsp = VN_CAST(lhs, UnpackArrayDType);
                ধ্রুবক AstUnpackArrayDType* ধ্রুবক rhsp = VN_CAST(rhs, UnpackArrayDType);
                ধ্রুবক পূর্ণসংখ্যা lsz = lhsp->elementsConst();
                ধ্রুবক পূর্ণসংখ্যা rsz = rhsp->elementsConst();
                যদি (lsz >= 0 && rsz >= 0 && lsz != rsz) ফেরত মিথ্যা;
                ফেরত isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            ফেরত মিথ্যা;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ধ্রুবক যৌক্তিক lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ধ্রুবক যৌক্তিক rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ধ্রুবক যৌক্তিক lhsIsQueue = VN_IS(lhs, QueueDType);
        ধ্রুবক যৌক্তিক rhsIsQueue = VN_IS(rhs, QueueDType);
        ধ্রুবক যৌক্তিক lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ধ্রুবক যৌক্তিক rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        যদি (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            যদি (ধ্রুবক AstDynArrayDType* ধ্রুবক lhsp = VN_CAST(lhs, DynArrayDType)) {
                যদি (ধ্রুবক AstDynArrayDType* ধ্রুবক rhsp = VN_CAST(rhs, DynArrayDType)) {
                    ফেরত isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            যদি (ধ্রুবক AstQueueDType* ধ্রুবক lhsp = VN_CAST(lhs, QueueDType)) {
                যদি (ধ্রুবক AstQueueDType* ধ্রুবক rhsp = VN_CAST(rhs, QueueDType)) {
                    ফেরত isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            যদি (ধ্রুবক AstAssocArrayDType* ধ্রুবক lhsp = VN_CAST(lhs, AssocArrayDType)) {
                যদি (ধ্রুবক AstAssocArrayDType* ধ্রুবক rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    ফেরত isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->চাবি_DTypep(), rhsp->চাবি_DTypep());
                }
            }

            ফেরত মিথ্যা;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        যদি (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            যদি (lhs->width() != rhs->width()) ফেরত মিথ্যা;
            যদি (lhs->isFourstate() != rhs->isFourstate()) ফেরত মিথ্যা;
            যদি (lhs->isSigned() != rhs->isSigned()) ফেরত মিথ্যা;
            ফেরত সত্য;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        যদি (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) ফেরত মিথ্যা;
        ফেরত !(VN_IS(lhs, গ্রন্থি_UOr_Struct_DType) || VN_IS(rhs, গ্রন্থি_UOr_Struct_DType));
    }

    স্থির যৌক্তিক isAggregateType(ধ্রুবক Ast_গ্রন্থি* nodep) {
        যদি (!nodep) ফেরত মিথ্যা;
        ধ্রুবক Ast_গ্রন্থি_DType* dtypep = nodep->dtypep();
        যদি (!dtypep) ফেরত মিথ্যা;
        dtypep = dtypep->skipRefp();
        যদি (!dtypep) ফেরত মিথ্যা;
        ফেরত dtypep->isAggregateType();
    }

    শূন্য visit_cmp_eq_gt(Ast_গ্রন্থি_Biop* nodep, ধ্রুবক ComparisonType cmpType) {
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
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ধ্রুবক যৌক্তিক lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            ধ্রুবক যৌক্তিক rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            যদি (lhsIsPat && !rhsIsPat) {
                user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } অন্যথা যদি (rhsIsPat && !lhsIsPat) {
                user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } অন্যথা {
                user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            ধ্রুবক যৌক্তিক isAggrLhs = isAggregateType(nodep->lhsp());
            ধ্রুবক যৌক্তিক isAggrRhs = isAggregateType(nodep->rhsp());

            যদি ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক lhsDType = nodep->lhsp()->dtypep();
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক rhsDType = nodep->rhsp()->dtypep();

                যদি (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_গ্রন্থি* ধ্রুবক newp
                        = নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } অন্যথা যদি (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                যদি (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_গ্রন্থি* ধ্রুবক newp
                        = নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত;
                }
                যদি (Ast_গ্রন্থি_Biop* ধ্রুবক newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } অন্যথা যদি (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                যদি (Ast_গ্রন্থি_Biop* ধ্রুবক newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } অন্যথা {
                ধ্রুবক যৌক্তিক signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                যদি (Ast_গ্রন্থি_Biop* ধ্রুবক newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ধ্রুবক পূর্ণসংখ্যা width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ধ্রুবক পূর্ণসংখ্যা ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                যৌক্তিক warnOn = সত্য;
                যদি (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    যদি ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = মিথ্যা;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    যদি ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = মিথ্যা;
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
    শূন্য visit_cmp_real(Ast_গ্রন্থি_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit_cmp_string(Ast_গ্রন্থি_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit_cmp_type(Ast_গ্রন্থি_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            ধ্রুবক AstAttrOf* ধ্রুবক lhsap = VN_AS(nodep->lhsp(), AttrOf);
            ধ্রুবক AstAttrOf* ধ্রুবক rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক lhsDtp = lhsap->dtypep();
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ধ্রুবক যৌক্তিক invert = VN_IS(nodep, NeqT);
            ধ্রুবক যৌক্তিক identical
                = Ast_গ্রন্থি::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ধ্রুবক যৌক্তিক eq = invert ^ identical;
            Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    শূন্য visit_negate_not(Ast_গ্রন্থি_Uniop* nodep, যৌক্তিক real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            যদি (!real_ok) checkCvtUS(nodep->lhsp(), মিথ্যা);
        }
        যদি (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            যদি (Ast_গ্রন্থি_Uniop* ধ্রুবক newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                ফেরত;
            }
        } অন্যথা {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        যদি (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        যদি (m_vup->final()) {
            Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = expDTypep;
            // Some warning suppressions
            যৌক্তিক lhsWarn = সত্য;
            যদি (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                যদি (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = মিথ্যা;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    শূন্য visit_signed_unsigned(Ast_গ্রন্থি_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), সত্য);
            ধ্রুবক পূর্ণসংখ্যা width = nodep->lhsp()->width();
            Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    শূন্য visit_shift(Ast_গ্রন্থি_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ধ্রুবক Ast_গ্রন্থি_Biop* ধ্রুবক newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (শূন্য)newp;  // Ununused
    }
    শূন্য iterate_shift_prelim(Ast_গ্রন্থি_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), মিথ্যা);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            যদি (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_গ্রন্থি_Biop* iterate_shift_final(Ast_গ্রন্থি_Biop* nodep) {
        // Nodep maybe edited
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->final()) {
            Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            যদি (VN_IS(nodep, ShiftRS)) {
                যদি (Ast_গ্রন্থি_Biop* ধ্রুবক newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            যৌক্তিক warnOn = সত্য;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            যদি (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), গ্রন্থি_Assign)) warnOn = মিথ্যা;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ধ্রুবক shiftp = VN_CAST(nodep->rhsp(), Const);
            যদি (shiftp && !shiftp->অঙ্ক().isFourState() && shiftp->width() <= 32) {
                ধ্রুবক int64_t shiftVal = shiftp->অঙ্ক().toSQuad();
                যদি (VN_IS(nodep, ShiftL)) {
                    যদি (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = মিথ্যা;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            যদি (nodep->rhsp()->width() > 32) {
                যদি (shiftp && shiftp->অঙ্ক().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_সংখ্যা অঙ্ক{shiftp, 32, shiftp->অঙ্ক()};
                    Ast_গ্রন্থি* ধ্রুবক shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(নতুন AstConst{shiftrhsp->fileline(), অঙ্ক});
                    VL_DO_DANGLING(shiftrhsp->delete_বৃক্ষ(), shiftrhsp);
                }
            }
        }
        ফেরত nodep;  // May edit
    }

    শূন্য visit_boolexpr_and_or(Ast_গ্রন্থি_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), মিথ্যা);
            checkCvtUS(nodep->rhsp(), মিথ্যা);
            ধ্রুবক পূর্ণসংখ্যা width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            ধ্রুবক পূর্ণসংখ্যা mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            ধ্রুবক যৌক্তিক expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_গ্রন্থি_রাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        যদি (m_vup->final()) {
            Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    শূন্য visit_add_sub_replace(Ast_গ্রন্থি_Biop* nodep, যৌক্তিক real_ok, ধ্রুবক যৌক্তিক modOrDiv) {
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
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_পরবর্তী(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_পরবর্তী(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            যদি (!real_ok) {
                checkCvtUS(nodep->lhsp(), মিথ্যা);
                checkCvtUS(nodep->rhsp(), মিথ্যা);
            }
            যদি (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                যদি (Ast_গ্রন্থি_Biop* ধ্রুবক newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                ফেরত;
            } অন্যথা যদি (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            } অন্যথা {
                ধ্রুবক পূর্ণসংখ্যা width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ধ্রুবক পূর্ণসংখ্যা mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                ধ্রুবক যৌক্তিক expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_গ্রন্থি_রাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        যদি (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            যদি (Ast_গ্রন্থি_Biop* ধ্রুবক newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            যৌক্তিক lhsWarn = সত্য;
            যৌক্তিক rhsWarn = সত্য;
            যদি (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                যদি (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = মিথ্যা;
                যদি (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = মিথ্যা;
                যদি (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = মিথ্যা;  // do_increment + ...
                যদি (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = মিথ্যা;  // ... + do_increment
            } অন্যথা যদি (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                যদি (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = মিথ্যা;
                যদি (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = মিথ্যা;
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
    শূন্য visit_real_add_sub(Ast_গ্রন্থি_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    শূন্য visit_real_neg_ceil(Ast_গ্রন্থি_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_রাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    শূন্য visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->রাশিমালা_Format_2(), nodep, "Assumes constant format");
        যৌক্তিক inPct = মিথ্যা;
        Ast_গ্রন্থি_রাশিমালা* argp = nodep->exprsp();
        string newFormat;
        জন্য (অক্ষর ch : nodep->text()) {
            যদি (!inPct && ch == '%') {
                inPct = সত্য;
                newFormat += ch;
            } অন্যথা যদি (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } অন্যথা যদি (!inPct) {  // Normal text
                newFormat += ch;
            } অন্যথা {
                inPct = মিথ্যা;
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nextp = argp ? VN_AS(argp->nextp(), গ্রন্থি_রাশিমালা) : nullptr;
                AstSFormatArg* ধ্রুবক fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_গ্রন্থি_রাশিমালা* subargp = fargp ? fargp->exprp() : argp;
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                নির্বাচন (ch) {
                ক্ষেত্র '?':  // Unspecified by user, guess
                    যদি (dtypep->isDouble()) {
                        ch = 'g';
                    } অন্যথা যদি (dtypep->isString()) {
                        ch = 's';
                    } অন্যথা যদি (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } অন্যথা {
                        ch = 'p';
                    }
                    argp = nextp;
                    বিরতি;
                ক্ষেত্র '%': বিরতি;  // %% - just output a %
                ক্ষেত্র 'm': বিরতি;  // %m - auto insert "name"
                ক্ষেত্র 'l': বিরতি;  // %m - auto insert "library"
                ক্ষেত্র 'd':
                    যদি (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        যদি (subargp->isDouble()) spliceCvtS(subargp, সত্য, 64);
                        argp = nextp;
                    }
                    বিরতি;
                ক্ষেত্র 'b':  // FALLTHRU
                ক্ষেত্র 'o':  // FALLTHRU
                ক্ষেত্র 'x':
                    যদি (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    বিরতি;
                ক্ষেত্র 't':  // Decimal/float time
                    যদি (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    বিরতি;
                ক্ষেত্র 'e':  // FALLTHRU
                ক্ষেত্র 'f':  // FALLTHRU
                ক্ষেত্র 'g':
                    যদি (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        যদি (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    বিরতি;
                ক্ষেত্র 'p':  // FALLTHRU
                ক্ষেত্র 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    যদি (subargp && !fargp) {
                        যদি (AstEnumDType* ধ্রুবক enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    বিরতি;
                পূর্বনির্ধারিত:  // Most operators, just move to next argument
                    argp = nextp;
                    বিরতি;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    স্থির AstEnumDType* formatEnumDType(Ast_গ্রন্থি_রাশিমালা* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        যদি (!enumDtp) {
            যদি (ধ্রুবক AstVarRef* ধ্রুবক varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        ফেরত enumDtp;
    }

    Ast_রাশিমালা_Stmt* newEnumCapture(Ast_গ্রন্থি_রাশিমালা* valuep, AstEnumDType* dtypep) {
        File_সারি* ধ্রুবক flp = valuep->fileline();
        AstVar* ধ্রুবক varp = নতুন AstVar{flp, VVarType::XTEMP, m_enum_অস্থায়ী_Names.get(valuep), dtypep};
        varp->noSubst(সত্য);
        যদি (m_ftaskp) {
            varp->funcLocal(সত্য);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } অন্যথা {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_রাশিমালা_Stmt* ধ্রুবক exprp = নতুন Ast_রাশিমালা_Stmt{
            flp, নতুন AstAssign{flp, নতুন AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            নতুন AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        ফেরত exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_গ্রন্থি_রাশিমালা* valuep, AstEnumDType* dtypep) {
        যদি (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_গ্রন্থি_রাশিমালা* keyp;
        যদি (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_বৃক্ষ(মিথ্যা);
        } অন্যথা {
            Ast_রাশিমালা_Stmt* ধ্রুবক capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_বৃক্ষ_Pure(মিথ্যা);
        }
        ধ্রুবক VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ধ্রুবক argp = নতুন AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        ফেরত argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    যৌক্তিক widthBad(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_DType* expDTypep) {
        ধ্রুবক পূর্ণসংখ্যা expWidth = expDTypep->width();
        পূর্ণসংখ্যা expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        যদি (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) ফেরত মিথ্যা;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        যদি (expWidthMin == 0) expWidthMin = expWidth;
        যদি (nodep->dtypep()->width() == expWidth) ফেরত মিথ্যা;
        যদি (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) ফেরত সত্য;
        যদি (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) ফেরত সত্য;
        ফেরত মিথ্যা;
    }

    শূন্য fixWidthExtend(Ast_গ্রন্থি_রাশিমালা* nodep, Ast_গ্রন্থি_DType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<পূর্ণসংখ্যা>(extendRule) << ") old: " << nodep);
        যদি (extendRule == EXTEND_OFF) ফেরত;
        AstConst* ধ্রুবক constp = VN_CAST(nodep, Const);
        ধ্রুবক পূর্ণসংখ্যা expWidth = expDTypep->width();
        যদি (constp && !constp->অঙ্ক().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_সংখ্যা অঙ্ক{nodep, expWidth, constp->অঙ্ক()};
            অঙ্ক.isSigned(মিথ্যা);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstConst{nodep->fileline(), অঙ্ক};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } অন্যথা যদি (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(সত্য);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } অন্যথা {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            যৌক্তিক doSigned = মিথ্যা;
            নির্বাচন (extendRule) {
            ক্ষেত্র EXTEND_ZERO: doSigned = মিথ্যা; বিরতি;
            ক্ষেত্র EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); বিরতি;
            ক্ষেত্র EXTEND_LHS: doSigned = nodep->isSigned(); বিরতি;
            পূর্বনির্ধারিত: nodep->v3fatalSrc("bad case");
            }
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp
                = (doSigned ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(সত্য);
            linker.relink(newp);
            nodep = newp;
        }
        যদি (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = spliceCvtD(nodep);
            nodep = newp;
        }
        যদি (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } অন্যথা {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    শূন্য fixWidthReduce(Ast_গ্রন্থি_রাশিমালা* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ধ্রুবক পূর্ণসংখ্যা expWidth = 1;
        ধ্রুবক পূর্ণসংখ্যা expSigned = মিথ্যা;
        যদি (nodep->width() == expWidth) ফেরত;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ধ্রুবক constp = VN_CAST(nodep, Const);
        যদি (constp) {
            V3_সংখ্যা অঙ্ক(nodep, expWidth);
            অঙ্ক.opRedOr(constp->অঙ্ক());
            অঙ্ক.isSigned(expSigned);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstConst{nodep->fileline(), অঙ্ক};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_বৃক্ষ(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } অন্যথা {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_গ্রন্থি_রাশিমালা* newp;
            Ast_গ্রন্থি_DType* ধ্রুবক dtypeSkipp = nodep->dtypep()->skipRefp();
            যদি (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = নতুন AstNeq{nodep->fileline(),
                                  নতুন AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } অন্যথা {
                // But, AstRedOr can be faster on wide
                newp = নতুন AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    যৌক্তিক fixAutoExtend(Ast_গ্রন্থি_রাশিমালা*& nodepr, পূর্ণসংখ্যা expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        যদি (AstConst* ধ্রুবক constp = VN_CAST(nodepr, Const)) {
            যদি (constp->অঙ্ক().autoExtend() && !constp->অঙ্ক().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_সংখ্যা অঙ্ক(constp, expWidth);
                অঙ্ক.opRepl(constp->অঙ্ক(), expWidth);  // {width{'1}}
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstConst{constp->fileline(), অঙ্ক};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_বৃক্ষ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ফেরত সত্য;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            অন্যথা যদি (!constp->অঙ্ক().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->অঙ্ক().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_সংখ্যা অঙ্ক(constp, expWidth);
                অঙ্ক.opExtendXZ(constp->অঙ্ক(), constp->width());
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstConst{constp->fileline(), অঙ্ক};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_বৃক্ষ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ফেরত সত্য;
            }
        }
        ফেরত মিথ্যা;  // No change
    }
    যৌক্তিক isBaseClassRecurse(ধ্রুবক AstClass* ধ্রুবক cls1p, ধ্রুবক AstClass* ধ্রুবক cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        যদি (cls1p == cls2p) ফেরত সত্য;
        জন্য (ধ্রুবক AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            যদি (isBaseClassRecurse(cls1p, cextp->classp())) ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }
    // Lines to end a message on nodep with the data types of its operands
    স্থির string warnDTypes(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক Ast_গ্রন্থি_DType* lhsDtp,
                             ধ্রুবক Ast_গ্রন্থি_DType* rhsDtp, ধ্রুবক string& lhsName = "Left-hand",
                             ধ্রুবক string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        যদি (lhsState == rhsState) lhsState = rhsState = "";
        ফেরত "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    স্থির যৌক্তিক unpackedArraySizesMatch(ধ্রুবক Ast_গ্রন্থি_DType* lhsDtp, ধ্রুবক Ast_গ্রন্থি_DType* rhsDtp) {
        যখন (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            যদি (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                ফেরত মিথ্যা;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        ফেরত সত্য;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    শূন্য checkUnpackedArrayAssignmentCompatible(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক T* ধ্রুবক lhsRefp,
                                                ধ্রুবক N* ধ্রুবক rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::মান || std::is_same<T, Ast_গ্রন্থি_Var_Ref>::মান)
                && (std::is_same<N, AstVar>::মান || std::is_same<N, Ast_গ্রন্থি_Var_Ref>::মান),
            "Unsupported types provided.");
        যদি (!lhsRefp || !rhsRefp) ফেরত;
        std::string lhsName;
        std::string rhsName;
        যদি (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } অন্যথা {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক lhsDtp = lhsRefp->dtypep()->skipRefp();
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক rhsDtp = rhsRefp->dtypep()->skipRefp();
        ধ্রুবক যৌক্তিক isLhsAggregate = lhsDtp->isAggregateType();
        ধ্রুবক যৌক্তিক isRhsAggregate = rhsDtp->isAggregateType();
        যদি (!isLhsAggregate && !isRhsAggregate) ফেরত;
        যদি (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ফেরত;
        } অন্যথা যদি (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            ফেরত;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(মিথ্যা),
                                      rhsDim = rhsDtp->dimensions(মিথ্যা);
        // Check if unpacked array dimensions are matching
        যদি (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ফেরত;
        }

        ধ্রুবক Ast_গ্রন্থি_DType* lhsDtpIterp = lhsDtp;
        ধ্রুবক Ast_গ্রন্থি_DType* rhsDtpIterp = rhsDtp;
        জন্য (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            যদি (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                ফেরত;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        যদি (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    শূন্য checkClassAssign(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* rhsp,
                          Ast_গ্রন্থি_DType* ধ্রুবক lhsDTypep, যৌক্তিক isOutputArg = মিথ্যা) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক lhsRawDTypep = lhsDTypep->skipRefp();
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক rhsRawDTypep = rhsp->dtypep()->skipRefp();
        যদি (ধ্রুবক AstClassRefDType* ধ্রুবক lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            যদি (ধ্রুবক AstClassRefDType* ধ্রুবক rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ধ্রুবক AstClass* ধ্রুবক dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ধ্রুবক AstClass* ধ্রুবক srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                যদি (isBaseClassRecurse(dstp, srcp)) ফেরত;
            } অন্যথা যদি (rhsp->isNull()) {
                ফেরত;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } অন্যথা যদি (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        যদি (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                নতুন AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, গ্রন্থি_রাশিমালা), lhsDTypep});
        }
    }
    স্থির যৌক্তিক similarDTypeRecurse(ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক node1p,
                                    ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক node2p) {
        ফেরত node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    শূন্য iterateCheckFileDesc(Ast_গ্রন্থি* parentp, Ast_গ্রন্থি* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckReal(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    শূন্য iterateCheckSigned8(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    শূন্য iterateCheckSigned32(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    শূন্য iterateCheckUInt32(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    শূন্য iterateCheckDelay(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        যদি (stage & FINAL) {
            Ast_গ্রন্থি_DType* expDTypep;
            যদি (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } অন্যথা {
                File_সারি* ধ্রুবক newFl = নতুন File_সারি{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, সত্য);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, মিথ্যা);
        }
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckString(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    শূন্য iterateCheckTyped(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp,
                           Ast_গ্রন্থি_DType* expDTypep, Stage stage) {
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        যদি (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckTypedSelfPrelim(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp,
                                     Ast_গ্রন্থি_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        যদি (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckIntegralSelf(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, সত্য);
    }
    শূন্য iterateCheckSelf(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Determ determ,
                          Stage stage, যৌক্তিক integralOnly = মিথ্যা) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, গ্রন্থি_রাশিমালা) ? checkCvtUS(VN_AS(underp, গ্রন্থি_রাশিমালা), integralOnly) : underp;
        Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckSizedSelf(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Determ determ,
                               Stage stage, যৌক্তিক deferUntyped = মিথ্যা) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, গ্রন্থি_রাশিমালা) ? checkCvtUS(VN_AS(underp, গ্রন্থি_রাশিমালা), মিথ্যা) : underp;
        Ast_গ্রন্থি_DType* ধ্রুবক expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_গ্রন্থি_DType* ধ্রুবক checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        যদি (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckAssign(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* rhsp, Stage stage,
                            Ast_গ্রন্থি_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        যদি (AstConst* constp = VN_CAST(rhsp, Const)) {
            যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                যদি (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    যদি (basicp->width() == 8 && constp->অঙ্ক().isFromString()) {
                        AstConsArray* newp = নতুন AstConsArray{
                            constp->fileline(), lhsDTypep,
                            নতুন AstConst{constp->fileline(), AstConst::Widthed_মান{}, 8, 0}};
                        জন্য (ধ্রুবক পূর্ণসংখ্যা aindex : arrayp->declRange().seqLoToHi()) {
                            পূর্ণসংখ্যা cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_সংখ্যা selected{constp, 8};
                            selected.opSel(constp->অঙ্ক(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            যদি (!selected.isFourState()) {
                                যদি (ধ্রুবক uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 নতুন AstConst{constp->fileline(),
                                                              AstConst::Widthed_মান{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        ধ্রুবক যৌক্তিক lhsStream = (VN_IS(parentp, গ্রন্থি_Assign)
                                && VN_IS(VN_AS(parentp, গ্রন্থি_Assign)->lhsp(), গ্রন্থি_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, সত্য, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (শূন্য)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_গ্রন্থি_রাশিমালা* convertUnpackedOrder(Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি_DType* dtypep) {
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        যদি (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) ফেরত exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        ফেরত newp;
    }

    স্থির যৌক্তিক isLogicalOperator(ধ্রুবক Ast_গ্রন্থি* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        ফেরত VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    শূন্য iterateCheckBool(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Stage stage) {
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
        Ast_গ্রন্থি_DType* ধ্রুবক underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        যদি (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_গ্রন্থি* ধ্রুবক newp
                = নতুন AstNeqD{parentp->fileline(), VN_AS(underp, গ্রন্থি_রাশিমালা),
                              নতুন AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } অন্যথা যদি (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_গ্রন্থি* ধ্রুবক newp
                = নতুন AstNeqN{parentp->fileline(), VN_AS(underp, গ্রন্থি_রাশিমালা),
                              নতুন AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } অন্যথা যদি (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, গ্রন্থি_রাশিমালা)), underp);  // Changed
        } অন্যথা যদি (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(নতুন AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ঠেলো_Deletep(underp), underp);
        } অন্যথা {
            ধ্রুবক যৌক্তিক bad = widthBad(underp, parentp->findBitDType());
            যদি (bad) {
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
            যদি (Ast_রাশিমালা_Stmt* ধ্রুবক রাশিমালা_Stmtp = VN_CAST(underp, রাশিমালা_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(রাশিমালা_Stmtp->resultp());
                রাশিমালা_Stmtp->dtypeFrom(রাশিমালা_Stmtp->resultp());
            } অন্যথা যদি (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, গ্রন্থি_রাশিমালা)), underp);  // Changed
            }
        }
    }

    স্থির শূন্য checkEnum(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* parentp, ধ্রুবক Ast_গ্রন্থি_DType* targetDTypep,
                          ধ্রুবক Ast_গ্রন্থি_DType* sourceDTypep, ধ্রুবক Ast_গ্রন্থি_রাশিমালা* sourceExprp) {
        যদি (ধ্রুবক AstEnumDType* ধ্রুবক targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            ধ্রুবক VCastable castable
                = Ast_গ্রন্থি::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            যদি (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    Ast_গ্রন্থি* iterateCheck(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side, Ast_গ্রন্থি* underp, Determ determ,
                          Stage stage, Ast_গ্রন্থি_DType* expDTypep, ExtendRule extendRule,
                          যৌক্তিক warnOn = সত্য, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ধ্রুবক StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        যদি (rhsAccess.isReadOnly()) {
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = VN_CAST(underp, গ্রন্থি_রাশিমালা)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        যদি (VN_IS(underp, গ্রন্থি_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } অন্যথা যদি (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } অন্যথা যদি (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } অন্যথা যদি (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_গ্রন্থি* ধ্রুবক oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, গ্রন্থি_রাশিমালা));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } অন্যথা যদি (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_গ্রন্থি* ধ্রুবক oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, গ্রন্থি_রাশিমালা), সত্য, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } অন্যথা যদি (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_গ্রন্থি* ধ্রুবক oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, গ্রন্থি_রাশিমালা));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } অন্যথা {
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক underDtp = underp->dtypep()->skipRefp();
            যদি (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_গ্রন্থি* ধ্রুবক newp
                    = নতুন AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(underp), underp);
                underp = newp;
            }
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক expDtp = expDTypep->skipRefp();
            যদি (determ == ASSIGN && VN_IS(expDtp, গ্রন্থি_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            যদি (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ধ্রুবক AstBasicDType* ধ্রুবক expBasicp = expDTypep->basicp();
            ধ্রুবক AstBasicDType* ধ্রুবক underBasicp = underp->dtypep()->basicp();
            যদি (expBasicp && underBasicp) {
                যদি (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    যদি (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, গ্রন্থি_রাশিমালা));
                    }
                    যদি (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_গ্রন্থি_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                যদি (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } অন্যথা যদি (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    যদি (underp->isSigned() != subDTypep->isSigned()
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
                } অন্যথা {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, গ্রন্থি_রাশিমালা), expDTypep, extendRule,
                                warnOn);
            } অন্যথা যদি (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } অন্যথা যদি (ধ্রুবক AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ধ্রুবক AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                যদি (VN_IS(underp, Const) && VN_AS(underp, Const)->অঙ্ক().isNull()) {
                    // '= null' is ok
                } অন্যথা যদি (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } অন্যথা যদি (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } অন্যথা যদি (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } অন্যথা {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } অন্যথা {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        ফেরত underp;
    }

    শূন্য
    widthCheckSized(Ast_গ্রন্থি* parentp, ধ্রুবক অক্ষর* side,
                    Ast_গ্রন্থি_রাশিমালা* underp,  // Node to be checked or have typecast added in front of
                    Ast_গ্রন্থি_DType* expDTypep, ExtendRule extendRule, যৌক্তিক warnOn = সত্য) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ধ্রুবক AstBasicDType* ধ্রুবক expBasicp = expDTypep->basicp();
        ধ্রুবক AstBasicDType* ধ্রুবক underBasicp = underp->dtypep()->basicp();
        যদি (expDTypep == underp->dtypep()) {
            ফেরত;  // Same type must match
        } অন্যথা যদি (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#যদি VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            ফেরত;
        } অন্যথা {
            ধ্রুবক পূর্ণসংখ্যা expWidth = expDTypep->width();
            পূর্ণসংখ্যা expWidthMin = expDTypep->widthMin();
            যদি (expWidthMin == 0) expWidthMin = expWidth;
            ধ্রুবক যৌক্তিক bad = widthBad(underp, expDTypep);
            যদি ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                ফেরত;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            যদি (VN_IS(parentp->backp(), Cast_আকার)) warnOn = মিথ্যা;
            যদি (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = মিথ্যা;
            যদি (VN_IS(underp, Const) && VN_AS(underp, Const)->অঙ্ক().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = মিথ্যা;
            }
            যদি ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = মিথ্যা;
            }
            যদি (bad && warnOn) {
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
            যদি (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ধ্রুবক Ast_গ্রন্থি_Assign* assignp = VN_CAST(parentp, গ্রন্থি_Assign);
                ধ্রুবক AstPin* pinp = VN_CAST(parentp, Pin);
                যদি (assignp && VN_IS(assignp->lhsp(), গ্রন্থি_Stream)) {
                } অন্যথা যদি (pinp && pinp->modVarp()->direction() != VDirection::প্রবেশ) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } অন্যথা যদি (assignp && VN_IS(underp, গ্রন্থি_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ধ্রুবক পূর্ণসংখ্যা shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* ধ্রুবক widenedp = নতুন AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(সত্য);
                    // Shift left so zeros fill on the right
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক shiftedp = নতুন AstShiftL{
                        underp->fileline(), widenedp,
                        নতুন AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } অন্যথা {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_গ্রন্থি_রাশিমালা* checkCvtUS(Ast_গ্রন্থি_রাশিমালা* nodep, যৌক্তিক fatal) {
        যদি (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            যদি (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } অন্যথা {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, মিথ্যা, 32);
        }
        ফেরত nodep;
    }

    Ast_গ্রন্থি_রাশিমালা* spliceCvtD(Ast_গ্রন্থি_রাশিমালা* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        যদি (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_গ্রন্থি_রাশিমালা* newp;
            যদি (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = নতুন AstISToRD{nodep->fileline(), nodep};
            } অন্যথা {
                newp = নতুন AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            ফেরত newp;
        } অন্যথা {
            ফেরত nodep;
        }
    }
    Ast_গ্রন্থি_রাশিমালা* spliceCvtS(Ast_গ্রন্থি_রাশিমালা* nodep, যৌক্তিক warnOn, পূর্ণসংখ্যা width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        যদি (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                যদি (constp->isDouble() && V3_সংখ্যা::epsilonIntegral(constp->অঙ্ক().toDouble())) {
                    warnOn = মিথ্যা;
                }
            }
            যদি (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            ফেরত newp;
        } অন্যথা {
            ফেরত nodep;
        }
    }
    Ast_গ্রন্থি_রাশিমালা* spliceCvtString(Ast_গ্রন্থি_রাশিমালা* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        যদি (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            ফেরত newp;
        } অন্যথা {
            ফেরত nodep;
        }
    }
    Ast_গ্রন্থি_Biop* replaceWithUOrSVersion(Ast_গ্রন্থি_Biop* nodep, যৌক্তিক signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        যদি (signedFlavorNeeded == nodep->signedFlavor()) ফেরত nullptr;
        যদি (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        নির্বাচন (nodep->type()) {
        ক্ষেত্র VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); ফেরত nullptr;
        ক্ষেত্র VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); ফেরত nullptr;
        ক্ষেত্র VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ফেরত nullptr;
        ক্ষেত্র VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ফেরত nullptr;
        ক্ষেত্র VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ফেরত nullptr;
        ক্ষেত্র VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ফেরত nullptr;
        ক্ষেত্র VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); ফেরত nullptr;
        ক্ষেত্র VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); ফেরত nullptr;
        ক্ষেত্র VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); ফেরত nullptr;
        পূর্বনির্ধারিত: বিরতি;
        }
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি_Biop* newp = nullptr;
        নির্বাচন (nodep->type()) {
        ক্ষেত্র VNType::Gt: newp = নতুন AstGtS{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::GtS: newp = নতুন AstGt{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Gte: newp = নতুন AstGteS{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::GteS: newp = নতুন AstGte{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Lt: newp = নতুন AstLtS{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::LtS: newp = নতুন AstLt{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Lte: newp = নতুন AstLteS{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::LteS: newp = নতুন AstLte{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Div: newp = নতুন AstDivS{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::DivS: newp = নতুন AstDiv{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::ModDiv: newp = নতুন AstModDivS{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::ModDivS: newp = নতুন AstModDiv{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Mul: newp = নতুন AstMulS{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::MulS: newp = নতুন AstMul{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::ShiftR: newp = নতুন AstShiftRS{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::ShiftRS: newp = নতুন AstShiftR{fl, lhsp, rhsp}; বিরতি;
        পূর্বনির্ধারিত:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            বিরতি;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ফেরত newp;
    }
    Ast_গ্রন্থি_Biop* replaceWithDVersion(Ast_গ্রন্থি_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        যদি (nodep->doubleFlavor()) ফেরত nullptr;
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        নির্বাচন (nodep->type()) {
        ক্ষেত্র VNType::Add: newp = নতুন AstAddD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Sub: newp = নতুন AstSubD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Pow: newp = নতুন AstPowD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Eq:
        ক্ষেত্র VNType::EqCase: newp = নতুন AstEqD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Neq:
        ক্ষেত্র VNType::NeqCase: newp = নতুন AstNeqD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Gt:
        ক্ষেত্র VNType::GtS: newp = নতুন AstGtD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Gte:
        ক্ষেত্র VNType::GteS: newp = নতুন AstGteD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Lt:
        ক্ষেত্র VNType::LtS: newp = নতুন AstLtD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Lte:
        ক্ষেত্র VNType::LteS: newp = নতুন AstLteD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Div:
        ক্ষেত্র VNType::DivS: newp = নতুন AstDivD{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Mul:
        ক্ষেত্র VNType::MulS: newp = নতুন AstMulD{fl, lhsp, rhsp}; বিরতি;
        পূর্বনির্ধারিত:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            বিরতি;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ফেরত newp;
    }
    Ast_গ্রন্থি_Biop* replaceWithNVersion(Ast_গ্রন্থি_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        যদি (nodep->stringFlavor()) ফেরত nullptr;
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        নির্বাচন (nodep->type()) {
        ক্ষেত্র VNType::Eq:
        ক্ষেত্র VNType::EqCase:
        ক্ষেত্র VNType::EqWild: newp = নতুন AstEqN{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Neq:
        ক্ষেত্র VNType::NeqCase:
        ক্ষেত্র VNType::NeqWild: newp = নতুন AstNeqN{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Gt:
        ক্ষেত্র VNType::GtS: newp = নতুন AstGtN{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Gte:
        ক্ষেত্র VNType::GteS: newp = নতুন AstGteN{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Lt:
        ক্ষেত্র VNType::LtS: newp = নতুন AstLtN{fl, lhsp, rhsp}; বিরতি;
        ক্ষেত্র VNType::Lte:
        ক্ষেত্র VNType::LteS: newp = নতুন AstLteN{fl, lhsp, rhsp}; বিরতি;
        পূর্বনির্ধারিত:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            বিরতি;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ফেরত newp;
    }
    Ast_গ্রন্থি_Uniop* replaceWithDVersion(Ast_গ্রন্থি_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        যদি (nodep->doubleFlavor()) ফেরত nullptr;
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্রন্থি_Uniop* newp = nullptr;
        নির্বাচন (nodep->type()) {
        ক্ষেত্র VNType::Negate: newp = নতুন AstNegateD{fl, lhsp}; বিরতি;
        পূর্বনির্ধারিত:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            বিরতি;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ফেরত newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    শূন্য replaceWithSFormat(AstMethodCall* nodep, ধ্রুবক string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ধ্রুবক argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_গ্রন্থি_Var_Ref* ধ্রুবক fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstAssign{
            nodep->fileline(), fromp,
            নতুন AstSFormatF{nodep->fileline(), format, মিথ্যা, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        ঠেলো_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_গ্রন্থি_DType* iterateEditMoveDTypep(Ast_গ্রন্থি* parentp, Ast_গ্রন্থি_DType* dtnodep) {
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
        ধ্রুবক যৌক্তিক child1 = (parentp->getChildDTypep() == dtnodep);
        ধ্রুবক যৌক্তিক child2 = (parentp->getChild2DTypep() == dtnodep);
        যদি (child1 || child2) {
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
        যদি (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        ফেরত dtnodep;
    }

    AstConst* dimension_মান(File_সারি* fileline, Ast_গ্রন্থি_DType* nodep, VAttrType attrType,
                             পূর্ণসংখ্যা dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_গ্রন্থি_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        জন্য (পূর্ণসংখ্যা i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            যদি (ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক adtypep = VN_CAST(dtypep, গ্রন্থি_Array_DType)) {
                declRange = adtypep->declRange();
                যদি (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                চালু;
            } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক adtypep
                       = VN_CAST(dtypep, গ্রন্থি_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                বিরতি;  // Sub elements don't look like arrays and can't iterate into
            } অন্যথা যদি (ধ্রুবক AstBasicDType* ধ্রুবক adtypep = VN_CAST(dtypep, BasicDType)) {
                যদি (adtypep->isRanged()) declRange = adtypep->declRange();
                বিরতি;
            }
            বিরতি;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        পূর্ণসংখ্যা val = 0;
        নির্বাচন (attrType) {
        ক্ষেত্র VAttrType::DIM_BITS: {
            পূর্ণসংখ্যা bits = 1;
            যখন (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                যদি (ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক adtypep = VN_CAST(dtypep, গ্রন্থি_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    চালু;
                } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক adtypep
                           = VN_CAST(dtypep, গ্রন্থি_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    বিরতি;
                } অন্যথা যদি (ধ্রুবক AstBasicDType* ধ্রুবক adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    বিরতি;
                }
                বিরতি;
            }
            যদি (dim == 0) {
                val = 0;
            } অন্যথা যদি (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } অন্যথা {
                val = bits;
            }
            বিরতি;  // LCOV_EXCL_LINE
        }
        ক্ষেত্র VAttrType::DIM_উপরের: val = !declRange.ranged() ? 0 : declRange.hi(); বিরতি;
        ক্ষেত্র VAttrType::DIM_বাম: val = !declRange.ranged() ? 0 : declRange.বাম(); বিরতি;
        ক্ষেত্র VAttrType::DIM_নিচের: val = !declRange.ranged() ? 0 : declRange.lo(); বিরতি;
        ক্ষেত্র VAttrType::DIM_ডান: val = !declRange.ranged() ? 0 : declRange.ডান(); বিরতি;
        ক্ষেত্র VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            বিরতি;
        ক্ষেত্র VAttrType::DIM_আকার: val = !declRange.ranged() ? 0 : declRange.elements(); বিরতি;
        পূর্বনির্ধারিত: nodep->v3fatalSrc("Missing DIM ATTR type case"); বিরতি;
        }
        যদি (!valp) valp = নতুন AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        ফেরত valp;
    }
    AstVarRef* dimensionVarRefp(Ast_গ্রন্থি_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        যদি (cachedpr) ফেরত cachedpr->clone_বৃক্ষ(মিথ্যা);

        Ast_গ্রন্থি_Array_DType* ধ্রুবক vardtypep = নতুন AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), নতুন AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* ধ্রুবক consp = নতুন AstConsArray{nodep->fileline(), vardtypep};
        জন্য (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_মান(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_বৃক্ষ(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(সত্য);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(সত্য);
        ঠেলো_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ফেরত cachedpr->clone_বৃক্ষ(মিথ্যা);
    }
    স্থির uint64_t enum_Max_মান(ধ্রুবক Ast_গ্রন্থি* errNodep, ধ্রুবক AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        জন্য (ধ্রুবক AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ধ্রুবক AstConst* ধ্রুবক vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            যদি (!vconstp->অঙ্ক().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        যদি (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            ফেরত 64;
        }
        ফেরত maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ধ্রুবক nodep, VAttrType attrType, যৌক্তিক assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        যদি (cachedpr) ফেরত cachedpr->clone_বৃক্ষ(মিথ্যা);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_গ্রন্থি_DType* basep;
        যদি (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } অন্যথা যদি (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } অন্যথা {
            basep = nodep->dtypep();
        }
        Ast_গ্রন্থি_DType* vardtypep;
        যদি (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = নতুন AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } অন্যথা {
            vardtypep = নতুন AstUnpackArrayDType{flp, basep, নতুন AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        যদি (attrType == VAttrType::ENUM_NAME) {
            defaultp = নতুন AstConst{flp, AstConst::String{}, ""};
        } অন্যথা যদি (attrType == VAttrType::ENUM_পরবর্তী || attrType == VAttrType::ENUM_পূর্ব) {
            defaultp = নতুন AstConst{flp, V3_সংখ্যা{nodep, nodep->width(), 0}};
        } অন্যথা যদি (attrType == VAttrType::ENUM_VALID) {
            defaultp = নতুন AstConst{flp, AstConst::BitFalse{}};
        } অন্যথা {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_গ্রন্থি_রাশিমালা*> মানসমূহ;
        {
            uint64_t ordinal = 0;
            AstEnumItem* ধ্রুবক firstp = nodep->itemsp();
            ধ্রুবক AstEnumItem* prevp = firstp;  // Prev must start with last item
            যখন (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            জন্য (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* ধ্রুবক nextp = VN_AS(itemp->nextp(), EnumItem);
                ধ্রুবক AstConst* ধ্রুবক vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                যদি (!vconstp->অঙ্ক().isAnyXZ()) {  // Can 2-state runtime decode
                    ধ্রুবক uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    যদি (attrType == VAttrType::ENUM_NAME) {
                        মানসমূহ[i] = নতুন AstConst{flp, AstConst::String{},
                                                 V3_সংখ্যা::displayedEnumName(itemp)};
                    } অন্যথা যদি (attrType == VAttrType::ENUM_পরবর্তী) {
                        মানসমূহ[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_বৃক্ষ(মিথ্যা);  // A const
                    } অন্যথা যদি (attrType == VAttrType::ENUM_পূর্ব) {
                        মানসমূহ[i] = prevp->valuep()->clone_বৃক্ষ(মিথ্যা);  // A const
                    } অন্যথা যদি (attrType == VAttrType::ENUM_VALID) {
                        মানসমূহ[i] = নতুন AstConst{flp, AstConst::BitTrue{}};
                    } অন্যথা {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_গ্রন্থি_রাশিমালা* valuep = nullptr;
        যদি (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ধ্রুবক consp = নতুন AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            জন্য (ধ্রুবক auto& itr : মানসমূহ) {
                V3_সংখ্যা চাবি{nodep, nodep->width()};
                চাবি.setQuad(itr.first);
                consp->addItemsp(নতুন AstConsAssocItem{flp, নতুন AstConst{flp, চাবি}, itr.second});
            }
            valuep = consp;
        } অন্যথা {
            AstConsArray* ধ্রুবক consp = নতুন AstConsArray{flp, vardtypep, defaultp};
            জন্য (uint64_t i = 0; i < (msbdim + 1); ++i) {
                যদি (মানসমূহ[i]) consp->valuep(i, মানসমূহ[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(সত্য);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(সত্য);
        ঠেলো_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ফেরত cachedpr->clone_বৃক্ষ(মিথ্যা);
    }

    Ast_গ্রন্থি_রাশিমালা* enumSelect(Ast_গ্রন্থি_রাশিমালা* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        যদি (attrType == VAttrType::ENUM_NAME) {
            যদি (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep, Const)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->অঙ্ক().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত newp;
            }
        }
        যদি (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_রাশিমালা_Stmt* ধ্রুবক capturep = newEnumCapture(nodep, adtypep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            ফেরত capturep;
        }
        যদি (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_গ্রন্থি_রাশিমালা* indexp = নতুন AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            জন্য (ধ্রুবক AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ধ্রুবক AstConst* ধ্রুবক constp = VN_AS(itemp->valuep(), Const);
                যদি (constp->অঙ্ক().isAnyXZ()) চালু;
                indexp = নতুন AstCond{nodep->fileline(),
                                     নতুন AstEq{nodep->fileline(), nodep->clone_বৃক্ষ(মিথ্যা),
                                               itemp->valuep()->clone_বৃক্ষ(মিথ্যা)},
                                     নতুন AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ধ্রুবক tabRefp = enumVarRefp(adtypep, attrType, মিথ্যা, ordinal);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত newp;
        }
        ধ্রুবক uint64_t msbdim = enum_Max_মান(nodep, adtypep);
        ধ্রুবক যৌক্তিক assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_গ্রন্থি_রাশিমালা* newp;
        যদি (assoc) {
            AstVarRef* ধ্রুবক tabRefp = enumVarRefp(adtypep, attrType, সত্য, 0);
            newp = নতুন AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } অন্যথা {
            ধ্রুবক পূর্ণসংখ্যা selwidth = V3_সংখ্যা::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ধ্রুবক tabRefp
                = enumVarRefp(adtypep, attrType, মিথ্যা, (1ULL << selwidth) - 1);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? নতুন AstGt{nodep->fileline(), nodep->clone_বৃক্ষ(মিথ্যা),
                                  নতুন AstConst{nodep->fileline(),
                                               V3_সংখ্যা{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = নতুন AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                নতুন AstSel{nodep->fileline(), nodep, 0, selwidth}};
            যদি (boundp) {
                newp = নতুন AstCond{nodep->fileline(), boundp,
                                   নতুন AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        যদি (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } অন্যথা যদি (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } অন্যথা {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        ফেরত newp;
    }
    Ast_গ্রন্থি_রাশিমালা* enumTestValid(Ast_গ্রন্থি_রাশিমালা* valp, AstEnumDType* enumDtp) {
        ধ্রুবক uint64_t maxval = enum_Max_মান(valp, enumDtp);
        ধ্রুবক যৌক্তিক assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_গ্রন্থি_রাশিমালা* testp = nullptr;
        File_সারি* ধ্রুবক fl_novalue = নতুন File_সারি{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, সত্য);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, সত্য);
        যদি (assoc) {
            AstVarRef* ধ্রুবক tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, সত্য, 0);
            testp = নতুন AstAssocSel{fl_novalue, tabRefp, valp->clone_বৃক্ষ_Pure(মিথ্যা)};
        } অন্যথা {
            ধ্রুবক পূর্ণসংখ্যা selwidth = V3_সংখ্যা::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ধ্রুবক tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, মিথ্যা, (1ULL << selwidth) - 1);
            File_সারি* ধ্রুবক fl_nowidth = নতুন File_সারি{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, সত্য);
            testp = নতুন AstCond{
                fl_novalue,
                নতুন AstGt{fl_nowidth, valp->clone_বৃক্ষ_Pure(মিথ্যা),
                          নতুন AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                নতুন AstConst{fl_novalue, AstConst::BitFalse{}},
                নতুন AstArraySel{fl_novalue, tabRefp,
                                নতুন AstSel{fl_novalue, valp->clone_বৃক্ষ_Pure(মিথ্যা), 0, selwidth}}};
        }
        ফেরত testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, ধ্রুবক VNumRange& range) {
        PatVecMap patmap;
        পূর্ণসংখ্যা element = range.বাম();
        জন্য (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            যদি (patp->keyp()) {
                যদি (ধ্রুবক AstConst* ধ্রুবক constp = patternConstKeyp(patp)) {
                    যদি (constp->অঙ্ক().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } অন্যথা {
                        element = constp->toSInt();
                    }
                }
            }
            ধ্রুবক যৌক্তিক newEntry = patmap.emplace(element, patp).second;
            যদি (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.বাম_To_ডান_Inc();
        }
        ফেরত patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    শূন্য patConcatConvertRecurse(AstPattern* patternp, Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (AstConcat* ধ্রুবক concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } অন্যথা {
            AstPatMember* ধ্রুবক newp
                = নতুন AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(সত্য);
            patternp->addItemsp(newp);
        }
    }

    শূন্য makeOpenArrayShell(Ast_গ্রন্থি_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_গ্রন্থি_FTask* ধ্রুবক oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_গ্রন্থি_FTask* ধ্রুবক newTaskp = oldTaskp->clone_বৃক্ষ(মিথ্যা);
        newTaskp->dpiOpenChild(সত্য);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->add_পরবর্তী_Here(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ধ্রুবক V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            AstVar* ধ্রুবক portp = tconnect.first;
            ধ্রুবক AstArg* ধ্রুবক argp = tconnect.second;
            ধ্রুবক Ast_গ্রন্থি* ধ্রুবক pinp = argp->exprp();
            যদি (!pinp) চালু;  // Argument error we'll find later
            যদি (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_গ্রন্থি_DType* ধ্রুবক oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_গ্রন্থি* nodep) {
                    যদি (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    যৌক্তিক markHasOpenArray(Ast_গ্রন্থি_FTask* nodep) {
        যৌক্তিক hasOpen = মিথ্যা;
        জন্য (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুবক portp = VN_CAST(stmtp, Var)) {
                যদি (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(সত্য);
                    hasOpen = সত্য;
                }
            }
        }
        ফেরত hasOpen;
    }
    যৌক্তিক hasOpenArrayDTypeRecurse(Ast_গ্রন্থি_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        যদি (VN_IS(nodep, UnsizedArrayDType)) ফেরত সত্য;
        যদি (nodep->subDTypep()) ফেরত hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        ফেরত মিথ্যা;
    }
    Ast_গ্রন্থি_DType* dtypeNotIntAtomOrVecRecurse(Ast_গ্রন্থি_DType* nodep, যৌক্তিক ranged = মিথ্যা) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        যদি (AstBasicDType* ধ্রুবক dtp = VN_CAST(nodep, BasicDType)) {
            যদি (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                ফেরত dtp;  // Packed when already packed
            যদি (dtp->keyword().isIntNumeric()) ফেরত nullptr;
            ফেরত dtp;
        } অন্যথা যদি (AstPackArrayDType* ধ্রুবক dtp = VN_CAST(nodep, PackArrayDType)) {
            যদি (ranged) ফেরত dtp;  // Packed when already packed
            ফেরত dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), সত্য);
        }
        ফেরত nodep;
    }
    Ast_গ্রন্থি_DType* dtypeNot4StateIntegralRecurse(Ast_গ্রন্থি_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        যদি (AstBasicDType* ধ্রুবক dtp = VN_CAST(nodep, BasicDType)) {
            যদি (!dtp->keyword().isFourstate()) ফেরত dtp;
            ফেরত nullptr;
        } অন্যথা যদি (Ast_গ্রন্থি_Array_DType* ধ্রুবক dtp = VN_CAST(nodep, গ্রন্থি_Array_DType)) {
            ফেরত dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } অন্যথা যদি (Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক dtp = VN_CAST(nodep, গ্রন্থি_UOr_Struct_DType)) {
            জন্য (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_গ্রন্থি_DType* ধ্রুবক badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                যদি (badDtp) ফেরত badDtp;
            }
            ফেরত nullptr;
        }
        ফেরত nodep;
    }
    শূন্য checkForceReleaseLhs(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        যদি (Ast_গ্রন্থি* ধ্রুবক selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    শূন্য assert_At_রাশিমালা(Ast_গ্রন্থি* nodep) {
        যদি (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    শূন্য assertAtStatement(Ast_গ্রন্থি* nodep) {
        যদি (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    শূন্য checkConstantOrReplace(Ast_গ্রন্থি* nodep, যৌক্তিক noFourState, ধ্রুবক string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ধ্রুবক constp = VN_CAST(nodep, Const);
        যদি (!constp || (noFourState && constp->অঙ্ক().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ঠেলো_Deletep(nodep), nodep, constp);
            ফেরত;
        }
    }
    Ast_গ্রন্থি* গ্রন্থি_For_Unsized_Warning(Ast_গ্রন্থি* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        যদি (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            ফেরত nodep->op1p();
        } অন্যথা যদি (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            ফেরত nodep->op2p();
        }
        ফেরত nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_গ্রন্থি* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        যদি (VL_LIKELY(!nodep)) ফেরত nullptr;
        যদি (auto* ধ্রুবক refp = VN_CAST(nodep, RefDType)) {
            যদি (refp->typedefp() == typedefp) ফেরত refp;
        }
        যদি (auto* ধ্রুবক refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) ফেরত refp;
        যদি (auto* ধ্রুবক refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) ফেরত refp;
        যদি (auto* ধ্রুবক refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) ফেরত refp;
        যদি (auto* ধ্রুবক refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) ফেরত refp;
        ফেরত nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    স্থির যৌক্তিক isIfaceArrayDType(ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        যদি (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) ফেরত মিথ্যা;
        ধ্রুবক AstIfaceRefDType* ধ্রুবক irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        ফেরত irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    স্থির std::string unpacked_Array_আকার(ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        std::string আকার_2;
        dtypep = dtypep->skipRefp();
        যখন (ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            যদি (!আকার_2.empty()) আকার_2 += "x";
            আকার_2 += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        ফেরত আকার_2;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    স্থির Ast_গ্রন্থি_রাশিমালা* newIfaceArrayElement(Ast_গ্রন্থি_রাশিমালা* fromp, পূর্ণসংখ্যা bit) {
        AstConsArray* ধ্রুবক consp = VN_CAST(fromp, ConsArray);
        যদি (!consp || !isIfaceArrayDType(consp->dtypep())) ফেরত nullptr;
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        যদি (bit < 0 || bit >= arrp->elementsConst()) ফেরত nullptr;
        ফেরত consp->valuep(bit)->clone_বৃক্ষ(মিথ্যা);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_গ্রন্থি_রাশিমালা* new_Iface_Array_মান(ধ্রুবক Ast_গ্রন্থি_Var_Ref* refp, Ast_গ্রন্থি_DType* dtypep,
                                    ধ্রুবক std::string& name, ধ্রুবক std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_সারি* ধ্রুবক flp = refp->fileline();
        AstUnpackArrayDType* ধ্রুবক arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        যদি (!arrp) {
            AstVar* ধ্রুবক varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_গ্রন্থি_Var_Ref* newp;
            যদি (ধ্রুবক AstVarXRef* ধ্রুবক xrefp = VN_CAST(refp, VarXRef)) {
                newp = নতুন AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } অন্যথা {
                newp = নতুন AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            ফেরত newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ধ্রুবক consp = নতুন AstConsArray{flp, arrp};
        ধ্রুবক VNumRange range = arrp->declRange();
        জন্য (ধ্রুবক পূর্ণসংখ্যা i : range.seq_বাম_To_ডান()) {
            ধ্রুবক std::string s = name + "__BRA__" + Ast_গ্রন্থি::encode_সংখ্যা(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_মান(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        ফেরত consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    শূন্য replaceSplitIfaceArrayRef(Ast_গ্রন্থি_Var_Ref* nodep) {
        AstVar* ধ্রুবক varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        যদি (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_মান(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    শূন্য expandIfaceArrayPinDimensions(AstPin* nodep, ধ্রুবক Ast_গ্রন্থি_DType* dtypep,
                                       Ast_গ্রন্থি_রাশিমালা* conp, ধ্রুবক std::string& suffix,
                                       AstVar*& elemVarpr) {
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        যদি (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ধ্রুবক newp = nodep->clone_বৃক্ষ(মিথ্যা);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            যদি (conp) newp->exprp(conp->clone_বৃক্ষ(মিথ্যা));
            nodep->add_পরবর্তী_Here(newp);
            ফেরত;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ধ্রুবক VNumRange range = arrp->declRange();
        জন্য (পূর্ণসংখ্যা n = range.elements() - 1; n >= 0; --n) {
            ধ্রুবক পূর্ণসংখ্যা i = range.বাম() + n * range.বাম_To_ডান_Inc();
            ধ্রুবক std::string s = suffix + "__BRA__" + Ast_গ্রন্থি::encode_সংখ্যা(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক elemConnp = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
                যদি (!conp) ফেরত nullptr;
                ধ্রুবক AstUnpackArrayDType* ধ্রুবক conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ধ্রুবক পূর্ণসংখ্যা elements = conArrp->elementsConst();
                // Counted from lo
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                ফেরত resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    শূন্য expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ধ্রুবক portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        জন্য (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = VN_AS(nodep->exprp(), গ্রন্থি_রাশিমালা);
        যদি (exprp) ঠেলো_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ধ্রুবক যৌক্তিক connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpacked_Array_আকার(exprp->dtypep()) == unpacked_Array_আকার(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_গ্রন্থি* userIterateSubtreeReturnEdits(Ast_গ্রন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ফেরত nullptr;
        Ast_গ্রন্থি* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        ফেরত ret;
    }
    শূন্য userIterate(Ast_গ্রন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ফেরত;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    শূন্য user_Iterate_And_পরবর্তী(Ast_গ্রন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ফেরত;
        যদি (nodep->didWidth()) ফেরত;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate_And_পরবর্তী_Null(nodep);
    }
    শূন্য userIterateChildren(Ast_গ্রন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ফেরত;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    শূন্য userIterateChildrenBackwardsConst(Ast_গ্রন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ফেরত;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

সর্বজনীন:
    // CONSTRUCTORS
    WidthVisitor(যৌক্তিক paramsOnly,  // [in] TRUE if we are considering parameters only.
                 যৌক্তিক doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enum_অস্থায়ী_Names{"__Venum"}
        , m_inside_অস্থায়ী_Names{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_গ্রন্থি* mainAcceptEdit(Ast_গ্রন্থি* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        জন্য (AstVar* ধ্রুবক varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(ঠেলো_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        ফেরত nodep;
    }
    স্থির যৌক্তিক lowerAsFixedAggregate(ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep) {
        ফেরত dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    শূন্য streamAssignLower(Ast_গ্রন্থি_Assign* nodep) {
        Ast_গ্রন্থি_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        যদি (Ast_গ্রন্থি_Stream* ধ্রুবক streamp = VN_CAST(nodep->rhsp(), গ্রন্থি_Stream)) {
            Ast_গ্রন্থি_DType* ধ্রুবক lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ধ্রুবক পূর্ণসংখ্যা lwidth = lhsDTypeSkippedRefp->widthStream();
            ধ্রুবক পূর্ণসংখ্যা rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            যদি (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            যদি (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                ধ্রুবক পূর্ণসংখ্যা queue_Element_আকার = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queue_Element_আকার <= lwidth, nodep, "LHS < RHS");
            }
            যদি (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    নতুন AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        যদি (Ast_গ্রন্থি_Stream* ধ্রুবক streamp = VN_CAST(nodep->lhsp(), গ্রন্থি_Stream)) {
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_গ্রন্থি_DType* ধ্রুবক lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            ধ্রুবক পূর্ণসংখ্যা lwidth = lhsStreamDTypep->widthStream();
            ধ্রুবক পূর্ণসংখ্যা rwidth = rhsDTypep->widthStream();
            যদি (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            যদি (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক dstp = streamp->lhsp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* srcp = nodep->rhsp()->unlinkFrBack();
                যদি (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_গ্রন্থি_রাশিমালা::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } অন্যথা {
                    যদি (srcp->width() > lwidth) {
                        srcp = নতুন AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ঠেলো_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(নতুন AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } অন্যথা যদি (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(নতুন AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Width class functions

শূন্য V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ধ্রুবক WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ধ্রুবক VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{মিথ্যা, মিথ্যা};
        (শূন্য)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (শূন্য)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("width", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_গ্রন্থি* V3Width::widthParamsEdit(Ast_গ্রন্থি* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{সত্য, মিথ্যা};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ফেরত nodep;
}

শূন্য V3Width::streamAssignLowerEdit(Ast_গ্রন্থি_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{মিথ্যা, মিথ্যা};
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
Ast_গ্রন্থি* V3Width::widthGenerateParamsEdit(
    Ast_গ্রন্থি* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{সত্য, সত্য};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ফেরত nodep;
}

Ast_গ্রন্থি* V3Width::selectNonConstantRecurse(Ast_গ্রন্থি* nodep, যৌক্তিক inSel) {
    // If node has a non-constant select, return that select
    Ast_গ্রন্থি* resultp = nullptr;
    যদি (Ast_গ্রন্থি_Sel* ধ্রুবক anodep = VN_CAST(nodep, গ্রন্থি_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        যদি (resultp) ফেরত resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), সত্য);
    } অন্যথা যদি (AstSel* ধ্রুবক anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        যদি (resultp) ফেরত resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), সত্য);
    } অন্যথা যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুবক anodep = VN_CAST(nodep, গ্রন্থি_Var_Ref)) {
        যদি (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) ফেরত anodep;
    } অন্যথা {
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        যদি (resultp) ফেরত resultp;
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        যদি (resultp) ফেরত resultp;
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        যদি (resultp) ফেরত resultp;
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    ফেরত resultp;
}
