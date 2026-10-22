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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Width.h"

#அந்தர்பாவ "V3Assert.h"
#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Begin.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3ConstPool.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3LinkLValue.h"
#அந்தர்பாவ "V3MemberMap.h"
#அந்தர்பாவ "V3Number.h"
#அந்தர்பாவ "V3Randomize.h"
#அந்தர்பாவ "V3String.h"
#அந்தர்பாவ "V3Task.h"
#அந்தர்பாவ "V3UniqueNames.h"
#அந்தர்பாவ "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#அந்தர்பாவ "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<மாறிலி Ast_முனை*, AstClass*> m_cache;

    AstClass* findCached(Ast_முனை* nodep) {
        எனில் (!nodep) திரும்பு nullptr;
        மாறிலி auto it = m_cache.find(nodep);
        எனில் (it != m_cache.end()) திரும்பு it->second;

        AstClass* classp = nullptr;
        எனில் (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } இல்லையெனில் எனில் (AstClass* மாறிலி parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } இல்லையெனில் எனில் (AstClassPackage* மாறிலி packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } இல்லையெனில் {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        திரும்பு classp;
    }

பொது:
    AstClass* find(Ast_முனை* nodep) {
        எனில் (AstClass* மாறிலி classp = VN_CAST(nodep, Class)) திரும்பு classp;
        எனில் (AstClassPackage* மாறிலி packagep = VN_CAST(nodep, ClassPackage)) {
            திரும்பு packagep->classp();
        }
        திரும்பு findCached(nodep);
    }
};

//######################################################################

எண்ணிக்கை Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, மாறிலி Stage& rhs) {
    திரும்பு str << ("-PFB"[static_cast<முழு>(rhs)]);
}

எண்ணிக்கை Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, மாறிலி Determ& rhs) {
    நிலையான constexpr மாறிலி எழுத்து* மாறிலி s_det[] = {"SELF", "CNTX", "ASSN"};
    திரும்பு str << s_det[rhs];
}

எண்ணிக்கை StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, செய்தி) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               செய்தி)

//######################################################################
// Width state, as a visitor of each AstNode

வகுப்பு WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_முனை_DType* மாறிலி m_dtypep;  // Parent's data type to resolve to
    மாறிலி Stage m_stage;  // If true, report errors
    மாறிலி StreamUse m_streamUse;  // Current expression may be a stream concat

பொது:
    WidthVP(Ast_முனை_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        எனில் (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { திரும்பு இது; }
    தருக்க selfDtm() மாறிலி { திரும்பு m_dtypep == nullptr; }
    StreamUse streamUse() மாறிலி { திரும்பு m_streamUse; }
    Ast_முனை_DType* dtypep() மாறிலி {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        திரும்பு m_dtypep;
    }
    Ast_முனை_DType* dtypeNullp() மாறிலி { திரும்பு m_dtypep; }
    Ast_முனை_DType* dtypeNullSkipRefp() மாறிலி {
        Ast_முனை_DType* dtp = dtypeNullp();
        எனில் (dtp) dtp = dtp->skipRefp();
        திரும்பு dtp;
    }
    Ast_முனை_DType* dtypeOverridep(Ast_முனை_DType* மாறிலி defaultp) மாறிலி {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        எனில் (!m_dtypep) திரும்பு defaultp;
        எனில் (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            திரும்பு defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        திரும்பு m_dtypep;
    }
    முழு width() மாறிலி {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        திரும்பு m_dtypep->width();
    }
    முழு widthMin() மாறிலி {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        திரும்பு m_dtypep->widthMin();
    }
    தருக்க prelim() மாறிலி { திரும்பு m_stage & PRELIM; }
    தருக்க final() மாறிலி { திரும்பு m_stage & FINAL; }
    வெறுமை dump(std::ostream& str) மாறிலி {
        எனில் (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } இல்லையெனில் {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, மாறிலி WidthVP* vup) {
    எனில் (vup) vup->dump(str);
    திரும்பு str;
}

//######################################################################

வகுப்பு WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    வெறுமை clearWidthRecurse(Ast_முனை* nodep) {
        க்கு (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(பொய்);
            எனில் (Ast_முனை* மாறிலி refp = nodep->op1p()) clearWidthRecurse(refp);
            எனில் (Ast_முனை* மாறிலி refp = nodep->op2p()) clearWidthRecurse(refp);
            எனில் (Ast_முனை* மாறிலி refp = nodep->op3p()) clearWidthRecurse(refp);
            எனில் (Ast_முனை* மாறிலி refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

பொது:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    மெய்நிகர் ~WidthClearVisitor() = இயல்பு;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_முனை_iterate_instead_of_Ast_முனை_accept

//######################################################################

வகுப்பு WidthVisitor final : பொது VNVisitor {
    // TYPES
    using PatVecMap = std::map<முழு, AstPatMember*>;
    using DTypeMap = std::map<மாறிலி std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<மாறிலி AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    தருக்க m_underFork = பொய்;  // Visiting under a fork
    தருக்க m_underSExpr = பொய்;  // Visiting under a sequence expression
    தருக்க m_underPackedArray = பொய்;  // Visiting under a AstPackArrayDType
    தருக்க m_underMemberSel = பொய்;  // Viting under a MemberSel
    Ast_முனை* m_seqUnsupp = nullptr;  // Property has unsupported node
    தருக்க m_hasSExpr = பொய்;  // Property has a sequence expression
    மாறிலி AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    மாறிலி AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_முனை_Module* m_modep = nullptr;  // Current module
    மாறிலி AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_முனை_Procedure* m_procedurep = nullptr;  // Current final/always
    மாறிலி AstWith* m_withp = nullptr;  // Current 'with' statement
    மாறிலி AstFunc* m_funcp = nullptr;  // Current function
    மாறிலி AstAttrOf* m_attrp = nullptr;  // Current attribute
    மாறிலி Ast_முனை_கோவை* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    மாறிலி தருக்க m_paramsOnly;  // Computing parameter value; limit operation
    மாறிலி தருக்க m_doGenerate;  // Do errors later inside generate statement
    தருக்க m_streamConcat = பொய்;  // True if visiting arguments of stream concatenation
    தருக்க m_inParamOverride = பொய்;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<மாறிலி Ast_முனை_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<மாறிலி Ast_முனை_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<மாறிலி AstVar*> m_curModVars;  // Variables declared in current module

    நிலையான constexpr முழு ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    எண்ணிக்கை ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    எண்ணிக்கை வகுப்பு ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    நிலையான StreamUse streamUseForDeterm(Determ determ) {
        திரும்பு determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() மாறிலி { திரும்பு m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    நிலையான வெறுமை packIfUnpacked(Ast_முனை_கோவை* மாறிலி nodep) {
        எனில் (AstUnpackArrayDType* மாறிலி unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            மாறிலி முழு elements_இலக்கம் = unpackDTypep->arrayUnpackedElements();
            மாறிலி முழு unpackMinBits = elements_இலக்கம் * unpackDTypep->subDTypep()->widthMin();
            மாறிலி முழு unpackBits = elements_இலக்கம் * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(புதிய AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    நிலையான Ast_முனை_DType* fromDTypep(Ast_முனை* fromp) {
        எனில் (Ast_முனை_DType* மாறிலி dtypep = VN_CAST(fromp, முனை_DType))
            திரும்பு dtypep->skipRefOrNullp();
        திரும்பு fromp ? fromp->dtypep() : nullptr;
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
    வெறுமை visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    வெறுமை visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    வெறுமை visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    வெறுமை visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    வெறுமை visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    வெறுமை visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    வெறுமை visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    வெறுமை visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    வெறுமை visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    வெறுமை visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    வெறுமை visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    வெறுமை visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    வெறுமை visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    வெறுமை visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    வெறுமை visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    வெறுமை visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    வெறுமை visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    வெறுமை visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    வெறுமை visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    வெறுமை visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    வெறுமை visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    வெறுமை visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    வெறுமை visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    வெறுமை visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    வெறுமை visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    வெறுமை visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    வெறுமை visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    வெறுமை visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    வெறுமை visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    வெறுமை visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    வெறுமை visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    வெறுமை visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    வெறுமை visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    வெறுமை visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    வெறுமை visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    வெறுமை visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    வெறுமை visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    வெறுமை visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    வெறுமை visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    வெறுமை visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    வெறுமை visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    வெறுமை visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    வெறுமை visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    வெறுமை visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    வெறுமை visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    வெறுமை visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    வெறுமை visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    வெறுமை visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, உண்மை, பொய்); }
    வெறுமை visit(AstSub* nodep) override { visit_add_sub_replace(nodep, உண்மை, பொய்); }
    வெறுமை visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, உண்மை, உண்மை); }
    வெறுமை visit(AstMul* nodep) override { visit_add_sub_replace(nodep, உண்மை, பொய்); }
    // These can't promote to real
    வெறுமை visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, பொய், உண்மை); }
    வெறுமை visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, பொய், உண்மை); }
    வெறுமை visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, பொய், பொய்); }
    வெறுமை visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, பொய், உண்மை); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    வெறுமை visit(AstNegate* nodep) override { visit_negate_not(nodep, உண்மை); }
    // Unary never real
    வெறுமை visit(AstNot* nodep) override { visit_negate_not(nodep, பொய்); }

    // Real: inputs and output real
    வெறுமை visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    வெறுமை visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    வெறுமை visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    வெறுமை visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    வெறுமை visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    வெறுமை visit(Ast_முனை_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    வெறுமை visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    வெறுமை visit(Ast_முனை_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    வெறுமை visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    வெறுமை visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    வெறுமை visit(AstShiftL* nodep) override { visit_shift(nodep); }
    வெறுமை visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    வெறுமை visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    வெறுமை visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    வெறுமை visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    வெறுமை visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    வெறுமை visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    வெறுமை visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        எனில் (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    வெறுமை visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        எனில் (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    வெறுமை visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        எனில் (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    வெறுமை visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        எனில் (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    வெறுமை visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        எனில் (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    வெறுமை visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    வெறுமை visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            எனில் (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } இல்லையெனில் {
                nodep->dtypeSetInteger();
            }
        }
    }
    வெறுமை visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    வெறுமை visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    வெறுமை visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    வெறுமை visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    வெறுமை visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    வெறுமை visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            புதிய AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    நிலையான தருக்க isOverridePattern(மாறிலி Ast_முனை* nodep) {
        எனில் (மாறிலி AstPattern* மாறிலி patternp = VN_CAST(nodep, Pattern)) {
            திரும்பு !patternp->childDTypep();
        }
        மாறிலி AstCond* மாறிலி condp = VN_CAST(nodep, Cond);
        திரும்பு condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    வெறுமை visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_கோவை(nodep);
        எனில் (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            மாறிலி முழு errors = V3Error::error_மொத்தம்();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = பொய்;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->condp(), Const);
            மாறிலி தருக்க known = constp && !constp->இலக்கம்().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            எனில் (!known && V3Error::error_மொத்தம்() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_முனை_கோவை* மாறிலி keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            திரும்பு;
        }
        எனில் (m_vup->prelim()) {  // First stage evaluation
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
            Ast_முனை_DType* மாறிலி thenDTypep = nodep->thenp()->dtypep();
            Ast_முனை_DType* மாறிலி elseDTypep = nodep->elsep()->dtypep();
            Ast_முனை_DType* மாறிலி thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_முனை_DType* மாறிலி elseDTypeSkippedp = elseDTypep->skipRefp();
            எனில் (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } இல்லையெனில் எனில் (nodep->thenp()->is_Class_Handle_மதிப்பு()
                       || nodep->elsep()->is_Class_Handle_மதிப்பு()) {
                Ast_முனை_DType* commonClassTypep = nullptr;
                எனில் (nodep->thenp()->is_Class_Handle_மதிப்பு() && nodep->elsep()->is_Class_Handle_மதிப்பு()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_முனை::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                எனில் (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } இல்லையெனில் {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } இல்லையெனில் எனில் (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                எனில் (Ast_முனை::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } இல்லையெனில் {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    எனில் (!Ast_முனை::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } இல்லையெனில் எனில் (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } இல்லையெனில் எனில் (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } இல்லையெனில் {
                மாறிலி தருக்க isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                தருக்க handled = பொய்;
                மாறிலி தருக்க isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_முனை_DType* மாறிலி thenEnumDTypep = thenDTypep->skipRefToEnump();
                எனில் (VN_IS(thenDTypeSkippedp, முனை_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_மரம்(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = உண்மை;
                } இல்லையெனில் எனில் (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_மரம்(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    எனில் (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = உண்மை;
                    } இல்லையெனில் {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(புதிய File_வரி{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, உண்மை);
                    }
                }
                எனில் (!handled) {
                    மாறிலி முழு width = std::max(thenDTypep->width(), elseDTypep->width());
                    மாறிலி முழு widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    மாறிலி VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    மாறிலி தருக்க isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    எனில் ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } இல்லையெனில் எனில் ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } இல்லையெனில் {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        எனில் (m_vup->final()) {
            Ast_முனை_DType* மாறிலி expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_முனை_DType* மாறிலி subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    வெறுமை visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_முனை_DType* மாறிலி vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        எனில் (Ast_முனை_கோவை* மாறிலி newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        எனில் (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* மாறிலி newp = புதிய AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            திரும்பு;
        }

        // Concat handling
        எனில் (m_vup->prelim()) {
            எனில் (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            எனில் (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } இல்லையெனில் {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                எனில் (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_முனை_கோவை::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            எனில் (மாறிலி AstReplicate* மாறிலி repp = VN_CAST(nodep->lhsp(), Replicate)) {
                எனில் (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
            }
            எனில் (மாறிலி AstReplicate* மாறிலி repp = VN_CAST(nodep->rhsp(), Replicate)) {
                எனில் (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
            }
        }
        எனில் (m_vup->final()) {
            எனில் (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_முனை_கோவை* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_முனை_கோவை* rhsp = nodep->rhsp()->unlinkFrBack();
                எனில் (!lhsp->isString()) lhsp = புதிய AstCvtPackString{lhsp->fileline(), lhsp};
                எனில் (!rhsp->isString()) rhsp = புதிய AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_முனை* மாறிலி newp = புதிய AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            }
            எனில் (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                முனை_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    வெறுமை visit(AstConcatN* nodep) override {
        எனில் (nodep->didWidth()) திரும்பு;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        எனில் (m_vup->final()) {
            nodep->didWidth(உண்மை);
            எனில் (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                முனை_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    வெறுமை visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    வெறுமை visit(AstDelay* nodep) override {
        எனில் (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            எனில் (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            எனில் (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                திரும்பு;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            திரும்பு;
        }
        எனில் (Ast_முனை_கோவை* மாறிலி fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            திரும்பு;
        }
        எனில் (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        எனில் (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        எனில் (nodep->fileline()->timingOn()) {
            எனில் (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                திரும்பு;
            } இல்லையெனில் எனில் (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } இல்லையெனில் {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        எனில் (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    வெறுமை visit(AstFork* nodep) override {
        எனில் (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        எனில் (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* மாறிலி newp = புதிய AstBegin{nodep->fileline(), nodep->name(), nullptr, பொய்};
            nodep->replaceWith(newp);
            எனில் (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            எனில் (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            எனில் (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = உண்மை;
            iterateChildren(nodep);
        } இல்லையெனில் எனில் (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        } இல்லையெனில் {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    வெறுமை visitWaitOrDisableFork(Ast_முனை* nodep) {
        எனில் (nodep->fileline()->timingOn()) {
            எனில் (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
                திரும்பு;
            } இல்லையெனில் எனில் (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    வெறுமை visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    வெறுமை visit(AstToLowerN* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    வெறுமை visit(AstToUpperN* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    வெறுமை visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->countp(), Const);
            எனில் (constp) {
                எனில் (constp->இலக்கம்().isFourState()
                    || (constp->dtypep()->isSigned() && constp->இலக்கம்().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } இல்லையெனில் {
                    times = constp->toSInt();
                }
            }

            Ast_முனை_DType* மாறிலி vdtypep = m_vup->dtypeNullSkipRefp();
            எனில் (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                எனில் (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                எனில் (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                } இல்லையெனில் {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* மாறிலி newp = புதிய AstPattern{
                        nodep->fileline(),
                        புதிய AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    திரும்பு;
                }
            }
            எனில் (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            எனில் (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } இல்லையெனில் {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            எனில் ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_முனை* மாறிலி newp
                    = புதிய AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            } இல்லையெனில் {
                எனில் (!constp) nodep->v3error("Replication value isn't a constant.");
                எனில் (times == 0
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
        எனில் (m_vup->final()) {
            எனில் (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                முனை_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    வெறுமை visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        எனில் (m_vup->final()) {
            எனில் (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                முனை_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    வெறுமை visit(Ast_முனை_Dist_Biop* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    வெறுமை visit(Ast_முனை_Dist_Triop* nodep) override {
        எனில் (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    தருக்க streamImplicitUseAllowed(மாறிலி Ast_முனை_Stream* nodep) மாறிலி {
        எனில் (m_streamConcat) திரும்பு உண்மை;
        எனில் (m_vup && m_vup->streamUse() != STREAM_USE_NONE) திரும்பு உண்மை;

        மாறிலி Ast_முனை* மாறிலி backp = nodep->backp();
        எனில் (!backp) திரும்பு உண்மை;  // Error elsewhere

        திரும்பு VN_IS(backp, முனை_Assign);
    }
    வெறுமை visit(Ast_முனை_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            m_streamConcat = உண்மை;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = பொய்;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->rhsp(), Const)) {
                எனில் (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } இல்லையெனில் {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            மாறிலி Ast_முனை_DType* மாறிலி lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            எனில் (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, முனை_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, முனை_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } இல்லையெனில் எனில் (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } இல்லையெனில் {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_முனை_கோவை::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        எனில் (m_vup->final()) {
            எனில் (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            எனில் (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                முனை_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    வெறுமை visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), உண்மை,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), உண்மை,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        எனில் (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            மாறிலி முழு width = nodep->elementsConst();
            எனில் (nodep->fromBracket() && nodep->இடது_Const() > nodep->வலது_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } இல்லையெனில் எனில் (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            மாறிலி தருக்க inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            மாறிலி தருக்க inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            மாறிலி தருக்க inTypeTable = !m_modep;
            மாறிலி Ast_முனை* basep = nodep->backp();
            போது (VN_IS(basep, Range)) basep = basep->backp();
            எனில் (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->இடது_Const() << ":" << nodep->வலது_Const()
                                            << "]");
            }
        }
    }

    வெறுமை visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        எனில் (nodep->didWidth()) திரும்பு;
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), பொய்);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            முழு width = nodep->widthConst();
            எனில் (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                திரும்பு;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            மாறிலி தருக்க isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            மாறிலி தருக்க inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            எனில் (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                தள்ளு_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(புதிய AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            எனில் (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                எனில் (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_முனை_DType* மாறிலி subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    பொய் /*noerror*/);
                } இல்லையெனில் {
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
            முழு frommsb = nodep->fromp()->width() - 1;
            முழு fromlsb = 0;
            மாறிலி முழு elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            எனில் (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } இல்லையெனில் {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            மாறிலி முழு selwidth = V3_எண்::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_முனை_DType* மாறிலி selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            எனில் (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                எனில் (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            எனில் (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                எனில் (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } இல்லையெனில் எனில் (!inParameterizedTemplate) {
                    எனில் (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } இல்லையெனில் {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                எனில் (!isWriteSelect) {
                    // Extend it
                    மாறிலி முழு extendTo = nodep->msbConst() + 1;
                    Ast_முனை_DType* மாறிலி subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    பொய் /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            எனில் (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                பொய் /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    வெறுமை visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            எனில் (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->bitp(), Const)) {
                    மாறிலி முழு bit = constp->toSInt();
                    Ast_முனை_கோவை* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    எனில் (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        மாறிலி AstUnpackArrayDType* மாறிலி arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
            }
            //
            முழு frommsb;
            முழு fromlsb;
            மாறிலி Ast_முனை_DType* மாறிலி fromDtp = nodep->fromp()->dtypep()->skipRefp();
            எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                எனில் (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                எனில் (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } இல்லையெனில் {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            மாறிலி முழு selwidth = V3_எண்::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_முனை_DType* மாறிலி selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            எனில் (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                எனில் (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            எனில் (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                எனில் (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    எனில் (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                பொய் /*NOWARN*/);
            }
        }
    }

    வெறுமை visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            மாறிலி Ast_முனை_DType* மாறிலி fromDtp = nodep->fromp()->dtypep()->skipRefp();
            மாறிலி AstAssocArrayDType* மாறிலி adtypep = VN_CAST(fromDtp, AssocArrayDType);
            எனில் (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->சாவி_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    வெறுமை visit(AstAlias* nodep) override {
        எனில் (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        மாறிலி auto check_If_கோவை_Ok = [இது](மாறிலி Ast_முனை_கோவை* மாறிலி exprp) {
            எனில் (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                திரும்பு பொய்;
            }

            மாறிலி AstVarRef* மாறிலி varRefp = VN_CAST(exprp, VarRef);
            எனில் (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                திரும்பு பொய்;
            }
            AstVar* மாறிலி varp = varRefp->varp();
            எனில் (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                திரும்பு பொய்;
            }
            எனில் (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                திரும்பு பொய்;
            } இல்லையெனில் {
                m_aliasedVars.செருகு(varp);
            }
            திரும்பு உண்மை;
        };

        check_If_கோவை_Ok(nodep->itemsp());
        Ast_முனை_DType* மாறிலி firstItemDtypep = nodep->itemsp()->dtypep();
        க்கு (Ast_முனை* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_கோவை_Ok(VN_AS(itemp, முனை_கோவை));
            எனில் (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    வெறுமை visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            மாறிலி Ast_முனை_DType* மாறிலி fromDtp = nodep->fromp()->dtypep()->skipRefp();
            மாறிலி AstWildcardArrayDType* மாறிலி adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            எனில் (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            மாறிலி AstBasicDType* மாறிலி basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            எனில் (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    வெறுமை visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            மாறிலி Ast_முனை_DType* மாறிலி fromDtp = nodep->fromp()->dtypep()->skipRefp();
            மாறிலி AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            எனில் (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_முனை_DType* மாறிலி newDtp
                = புதிய AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          புதிய AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            எனில் (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                மாறிலி முழு subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                மாறிலி VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                எனில் ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(புதிய AstConsArray{
                        nodep->fileline(), newDtp,
                        புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                } இல்லையெனில் எனில் ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            எனில் (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* மாறிலி fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* மாறிலி newp = புதிய AstConsArray{nodep->fileline(), newDtp};
                மாறிலி VNumRange range = nodep->declRange();
                க்கு (முழு n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_மரம்(பொய்));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        }
    }

    வெறுமை visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_முனை* மாறிலி selp = V3Width::widthSelNoIterEdit(nodep);
        எனில் (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            திரும்பு;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    வெறுமை visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_முனை* மாறிலி selp = V3Width::widthSelNoIterEdit(nodep);
        எனில் (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            திரும்பு;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    வெறுமை visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_முனை* மாறிலி selp = V3Width::widthSelNoIterEdit(nodep);
        எனில் (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            திரும்பு;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    வெறுமை visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_முனை* மாறிலி selp = V3Width::widthSelNoIterEdit(nodep);
        எனில் (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            திரும்பு;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    வெறுமை visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        எனில் (m_vup && m_vup->prelim()) {
            எனில் (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } இல்லையெனில் எனில் (nodep->இலக்கம்().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } இல்லையெனில் எனில் (nodep->இலக்கம்().isString()) {
                nodep->dtypeSetString();
            } இல்லையெனில் எனில் (nodep->இலக்கம்().isDouble()) {
                nodep->dtypeChgWidth(nodep->இலக்கம்().width(), nodep->இலக்கம்().sized()
                                                               ? nodep->இலக்கம்().width()
                                                               : nodep->இலக்கம்().widthToFit());
            } இல்லையெனில் {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->இலக்கம்().width(),
                    nodep->இலக்கம்().sized() ? nodep->இலக்கம்().width() : nodep->இலக்கம்().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->இலக்கம்().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    வெறுமை visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        எனில் (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    வெறுமை visit(AstFell* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ உண்மை);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstFalling* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ உண்மை);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstFuture* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ உண்மை);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    வெறுமை visit(AstPast* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ உண்மை);
            nodep->dtypeFrom(nodep->exprp());
            எனில் (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ உண்மை);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->ticksp(), Const);
                எனில் (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_மரம்();
                } இல்லையெனில் எனில் (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_மரம்();
                } இல்லையெனில் எனில் (V3AssertCommon::check_Sva_மதிப்பு_Limit(nodep, constp, "$past tick value")) {
                    எனில் (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    வெறுமை visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            மாறிலி AstConst* மாறிலி minConstp = VN_CAST(nodep->countp(), Const);
            எனில் (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } இல்லையெனில் எனில் (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } இல்லையெனில் எனில் (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            எனில் (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                மாறிலி AstConst* மாறிலி maxConstp = VN_CAST(nodep->maxCountp(), Const);
                எனில் (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } இல்லையெனில் எனில் (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } இல்லையெனில் எனில் (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_முனை_கோவை* propp = nodep->propp();
            எனில் (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            எனில் (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            மாறிலி தருக்க loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            மாறிலி தருக்க hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            எனில் (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_முனை* மாறிலி boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                திரும்பு;
            }
            எனில் (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                திரும்பு;
            }
            மாறிலி AstConst* மாறிலி loConstp = VN_CAST(nodep->loBoundp(), Const);
            மாறிலி AstConst* மாறிலி hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            எனில் (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            எனில் (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            எனில் (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } இல்லையெனில் எனில் (loConstp) {
                V3AssertCommon::check_Sva_மதிப்பு_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            எனில் (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_மதிப்பு_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            எனில் (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            தருக்க hasPropertyOp = propp->isMultiCycleSva();
            எனில் (!hasPropertyOp) {
                propp->foreach([&](மாறிலி Ast_முனை* np) {
                    எனில் (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = உண்மை;
                    }
                });
            }
            எனில் (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstRising* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ உண்மை);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstRose* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ உண்மை);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    வெறுமை visit(AstSampled* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ உண்மை);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    வெறுமை visit(AstSetuphold* nodep) override {
        File_வரி* மாறிலி flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        எனில் (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        எனில் (nodep->deldatap()) {
            எனில் (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } இல்லையெனில் {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        எனில் (!newp) {
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        nodep->replaceWith(புதிய AstAlways{newp});
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_வரி* மாறிலி flp, Ast_முனை_கோவை* மாறிலி evp,
                                         Ast_முனை_கோவை* மாறிலி delp) {
        Ast_முனை_கோவை* மாறிலி lhsp = delp->clone_மரம்_Pure(பொய்);
        Ast_முனை_கோவை* மாறிலி rhsp = evp->clone_மரம்_Pure(பொய்);
        UASSERT_OBJ(VN_IS(lhsp, முனை_Var_Ref) || VN_IS(lhsp, முனை_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        எனில் (Ast_முனை_Var_Ref* varRefp = VN_CAST(lhsp, முனை_Var_Ref)) {
            எனில் (varRefp->varp()->direction() == VDirection::உள்ளீடு) {
                VL_DO_DANGLING(தள்ளு_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(தள்ளு_Deletep(rhsp), rhsp);
                திரும்பு nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        எனில் (Ast_முனை_Pre_Sel* selp = VN_CAST(lhsp, முனை_Pre_Sel)) {
            எனில் (Ast_முனை_Var_Ref* varRefp = VN_CAST(selp->fromp(), முனை_Var_Ref)) {
                எனில் (varRefp->varp()->direction() == VDirection::உள்ளீடு) {
                    VL_DO_DANGLING(தள்ளு_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(தள்ளு_Deletep(rhsp), rhsp);
                    திரும்பு nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        திரும்பு புதிய AstAssignW{flp, lhsp, rhsp};
    }

    வெறுமை visit(AstStable* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ உண்மை);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstSteady* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ உண்மை);
            nodep->dtypeSetBit();
        }
    }

    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    வெறுமை visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    வெறுமை visit(AstUntil* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            எனில் (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            எனில் (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    வெறுமை visit(AstRand* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            எனில் (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } இல்லையெனில் {
                nodep->dtypeSetInt();  // Says the spec
            }
            எனில் (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    வெறுமை visit(AstSEventually* nodep) override {
        எனில் (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::Widthed_மதிப்பு{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            திரும்பு;
        }
        எனில் (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstSGotoRep* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            எனில் (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } இல்லையெனில் {
                width_Check_Rep_மொத்தம்(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstSNonConsRep* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            எனில் (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } இல்லையெனில் {
                width_Check_Rep_மொத்தம்(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    வெறுமை width_Check_Rep_மொத்தம்(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->countp(), Const);
        எனில் (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } இல்லையெனில் எனில் (constp->dtypep()->isSigned() && constp->இலக்கம்().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } இல்லையெனில் எனில் (constp->இலக்கம்().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } இல்லையெனில் எனில் (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    வெறுமை widthCheckGotoRepRange(T_Rep* nodep, மாறிலி எழுத்து* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        மாறிலி AstConst* மாறிலி minConstp = VN_CAST(nodep->countp(), Const);
        மாறிலி AstConst* மாறிலி maxConstp = VN_CAST(nodep->maxCountp(), Const);
        எனில் (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            திரும்பு;
        }
        எனில் (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            திரும்பு;
        }
        எனில் (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            திரும்பு;
        }
        எனில் (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    வெறுமை visit(AstSThroughout* nodep) override {
        m_hasSExpr = உண்மை;
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = பொய்;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = உண்மை;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = உண்மை;
        m_hasSExpr = உண்மை;
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            எனில் (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = உண்மை;
        m_hasSExpr = உண்மை;
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            க்கு (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstURandomRange* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_முனை_DType* மாறிலி expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    வெறுமை visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        எனில் (VN_IS(nodep->backp(), IsUnbounded)) திரும்பு;  // Ok, leave
        எனில் (VN_IS(nodep->backp(), BracketArrayDType)) திரும்பு;  // Ok, leave
        எனில் (VN_IS(nodep->backp(), InsideRange)) திரும்பு;  // Ok, leave
        எனில் (மாறிலி auto* மாறிலி varp = VN_CAST(nodep->backp(), Var)) {
            எனில் (varp->isParam()) திரும்பு;  // Ok, leave
        }
        Ast_முனை* backp = nodep->backp();
        எனில் (VN_IS(backp, Sub)) backp = backp->backp();
        எனில் (மாறிலி auto* மாறிலி selp = VN_CAST(backp, SelExtract)) {
            எனில் (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) திரும்பு;
        }
        எனில் (மாறிலி auto* மாறிலி selp = VN_CAST(backp, SelBit)) {
            எனில் (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) திரும்பு;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    வெறுமை visit(AstInferredDisable* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) nodep->dtypeSetBit();
    }
    வெறுமை visit(AstIsUnbounded* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        எனில் (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        எனில் (m_vup->final()) {
            Ast_முனை_DType* மாறிலி expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            எனில் (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    வெறுமை visit(AstCLog2* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    வெறுமை widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_முனை_கோவை* மாறிலி valuep = nodep->valuep();
        மாறிலி std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(valuep, Const)) {
            எனில் (!constp->இலக்கம்().isNegative()) திரும்பு;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } இல்லையெனில் எனில் (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } இல்லையெனில் {
            திரும்பு;
        }
        // Continue with the default weight
        valuep->replaceWith(புதிய AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    வெறுமை widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    வெறுமை visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* மாறிலி cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        எனில் (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            திரும்பு;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        எனில் (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            திரும்பு;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        எனில் (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            எனில் (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } இல்லையெனில் {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } இல்லையெனில் எனில் (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_முனை_கோவை* மாறிலி valuep = nodep->valuep();
            எனில் (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(புதிய AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
            }
            திரும்பு;
        } இல்லையெனில் எனில் (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            திரும்பு;
        } இல்லையெனில் எனில் (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->valuep(), Const);
            எனில் (!constp || !constp->இலக்கம்().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstCoverOption* nodep) override {
        எனில் (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            திரும்பு;
        }
        எனில் (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            திரும்பு;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        எனில் (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    வெறுமை visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        எனில் (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    வெறுமை visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        எனில் (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    வெறுமை widthCovergroupRanges(Ast_முனை* rangesp, முழு fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        மாறிலி auto fill = [&](Ast_முனை* nodep) {
            எனில் (!fillWidth) திரும்பு;
            Ast_முனை_கோவை* exprp = VN_AS(nodep, முனை_கோவை);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        க்கு (Ast_முனை *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            எனில் (AstInsideRange* மாறிலி rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } இல்லையெனில் எனில் (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } இல்லையெனில் {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    வெறுமை visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    வெறுமை visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        எனில் (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        எனில் (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    வெறுமை visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        எனில் (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        எனில் (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    வெறுமை visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        எனில் (Ast_முனை_கோவை* மாறிலி countp = nodep->countp()) {
            எனில் (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } இல்லையெனில் {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    வெறுமை widthCoverFilter(Ast_முனை_கோவை* filterp, மாறிலி எழுத்து* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), முனை_கோவை);
        File_வரி* மாறிலி fl = filterp->fileline();
        மாறிலி Ast_முனை_DType* மாறிலி resultp = filterp->dtypep()->skipRefp();
        எனில் (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                புதிய AstNeqD{fl, filterp, புதிய AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } இல்லையெனில் எனில் (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(புதிய AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(தள்ளு_Deletep(filterp), filterp);
        } இல்லையெனில் {
            fixWidthReduce(filterp);
        }
    }
    வெறுமை visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    வெறுமை visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    வெறுமை visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            எனில் (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                திரும்பு;
            }
            checkCvtUS(nodep->lhsp(), பொய்);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            எனில் (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->இலக்கம்().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        எனில் (m_vup->final()) {
            Ast_முனை_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_முனை_DType* மாறிலி lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_முனை* newp = nullptr;  // No change
            எனில் (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = புதிய AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } இல்லையெனில் எனில் (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = புதிய AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } இல்லையெனில் எனில் (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = புதிய AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            எனில் (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        }
    }
    வெறுமை visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(Ast_மொத்தம்_Bits* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            மாறிலி முழு widthMin = V3_எண்::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    வெறுமை visit(Ast_மொத்தம்_Ones* nodep) override {
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            மாறிலி முழு widthMin = V3_எண்::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    வெறுமை visit(AstCvtPackString* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        எனில் (Ast_முனை_DType* மாறிலி dt = nodep->lhsp()->dtypep()) {
            எனில் (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    வெறுமை visit(AstCvtPackedToArray* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    வெறுமை visit(AstCvtArrayToArray* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    வெறுமை visit(AstCvtArrayToPacked* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    வெறுமை visit(AstCvtUnpackToQueue* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    வெறுமை visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        எனில் (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    வெறுமை visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        மாறிலி இரட்டை timePrescale = constp->இலக்கம்().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        மாறிலி இரட்டை time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* மாறிலி newp = புதிய AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstEventControl* nodep) override {
        எனில் (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }
        எனில் (nodep->fileline()->timingOn()) {
            எனில் (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                திரும்பு;
            } இல்லையெனில் எனில் (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } இல்லையெனில் {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        எனில் (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    நிலையான string typenameExtends(மாறிலி AstClass* classp) {
        // An interface class may extend several, so those are not named
        எனில் (!classp || classp->isInterfaceClass()) திரும்பு "";
        க்கு (மாறிலி AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            எனில் (extendsp->isImplements()) தொடர்;
            மாறிலி AstClass* மாறிலி basep = extendsp->classOrNullp();
            திரும்பு basep ? " extends class " + basep->dtypeName(உண்மை) : "";
        }
        திரும்பு "";
    }
    வெறுமை visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        எனில் (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        தேர்வு (nodep->attrType()) {
        நிலை VAttrType::DIM_DIMENSIONS:
        நிலை VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_முனை_DType* மாறிலி dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            மாறிலி std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(உண்மை);
            மாறிலி முழு val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(புதிய AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            நிறுத்து;
        }
        நிலை VAttrType::DIM_BITS_OR_எண்: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            எனில் (VN_IS(nodep->fromp(), முனை_கோவை)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் {
                Ast_முனை* newp = புதிய AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            திரும்பு;
        }
        நிலை VAttrType::DIM_BITS:
        நிலை VAttrType::DIM_உயர்வு:
        நிலை VAttrType::DIM_INCREMENT:
        நிலை VAttrType::DIM_இடது:
        நிலை VAttrType::DIM_கீழ்:
        நிலை VAttrType::DIM_வலது:
        நிலை VAttrType::DIM_SIZE: {
            Ast_முனை_DType* மாறிலி dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            எனில் (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                மாறிலி Ast_முனை_Module* மாறிலி ownModp = v3Global.rootp()->containingModule(dtypep);
                எனில் (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    திரும்பு;
                }
            }

            எனில் (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                தேர்வு (nodep->attrType()) {
                நிலை VAttrType::DIM_SIZE: {
                    Ast_முனை_கோவை* மாறிலி fromp = VN_AS(nodep->fromp()->unlinkFrBack(), முனை_கோவை);
                    Ast_முனை* மாறிலி newp
                        = புதிய AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(உண்மை);
                    newp->protect(பொய்);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
                    நிறுத்து;
                }
                நிலை VAttrType::DIM_இடது:
                நிலை VAttrType::DIM_கீழ்: {
                    Ast_முனை* மாறிலி newp = புதிய AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    நிறுத்து;
                }
                நிலை VAttrType::DIM_வலது:
                நிலை VAttrType::DIM_உயர்வு: {
                    Ast_முனை_கோவை* மாறிலி fromp = VN_AS(nodep->fromp()->unlinkFrBack(), முனை_கோவை);
                    Ast_முனை_கோவை* மாறிலி sizep
                        = புதிய AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(உண்மை);
                    sizep->protect(பொய்);
                    Ast_முனை* மாறிலி newp
                        = புதிய AstSub{nodep->fileline(), sizep,
                                     புதிய AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
                    நிறுத்து;
                }
                நிலை VAttrType::DIM_INCREMENT: {
                    Ast_முனை_கோவை* மாறிலி newp
                        = புதிய AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    நிறுத்து;
                }
                நிலை VAttrType::DIM_BITS: {
                    எனில் (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } இல்லையெனில் {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    நிறுத்து;
                }
                இயல்பு: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } இல்லையெனில் {
                மாறிலி std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(உண்மை);
                மாறிலி uint32_t msbdim = dimpair.first + dimpair.second;
                எனில் (!nodep->dimp() || msbdim < 1) {
                    எனில் (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_முனை_கோவை* மாறிலி fromp = VN_AS(nodep->fromp()->unlinkFrBack(), முனை_கோவை);
                        Ast_முனை* மாறிலி newp = புதிய AstShiftL{
                            nodep->fileline(), புதிய AstLenN{nodep->fileline(), fromp},
                            புதிய AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    } இல்லையெனில் {
                        Ast_முனை_DType* மாறிலி baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        மாறிலி முழு dim = 1;
                        AstConst* மாறிலி newp = dimension_மதிப்பு(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    }
                } இல்லையெனில் எனில் (VN_IS(nodep->dimp(), Const)) {
                    மாறிலி முழு dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* மாறிலி newp
                        = dimension_மதிப்பு(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                } இல்லையெனில் {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* மாறிலி tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_முனை_கோவை* மாறிலி dimp = nodep->dimp()->unlinkFrBack();
                    Ast_முனை_கோவை* மாறிலி newp = புதிய AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                }
            }
            நிறுத்து;
        }
        நிலை VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            நிறுத்து;
        நிலை VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_முனை_DType* மாறிலி protoDtp = nodep->fromp()->dtypep();
            Ast_முனை_DType* மாறிலி declDtp = [&]() {
                எனில் (m_ftaskp->fvarp()) திரும்பு m_ftaskp->fvarp()->dtypep();
                Ast_முனை_DType* மாறிலி voidp = புதிய AstVoidDType{m_ftaskp->fileline()};
                தள்ளு_Deletep(voidp);  // Note voidp used past here
                திரும்பு voidp;
            }();
            எனில் (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            நிறுத்து;
        }
        நிலை VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_முனை_DType* மாறிலி dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(உண்மை);
            எனில் (மாறிலி AstClassRefDType* மாறிலி classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_முனை* மாறிலி newp = புதிய AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            நிறுத்து;
        }
        நிலை VAttrType::TYPEID:
            எனில் (Ast_முனை_DType* dtypep = VN_CAST(nodep->fromp(), முனை_DType)) {
                nodep->dtypep(dtypep);
            } இல்லையெனில் {
                nodep->dtypep(VN_AS(nodep->fromp(), முனை_கோவை)->dtypep());
            }
            நிறுத்து;
        நிலை VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            நிறுத்து;
        இயல்பு: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            நிறுத்து;
        }
        }
    }
    வெறுமை visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    வெறுமை visit(AstCellArrayRef* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(உண்மை);
    }

    வெறுமை visit(AstCellRef* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;

        எனில் (Ast_முனை_கோவை* மாறிலி cellExprp = VN_CAST(nodep->cellp(), முனை_கோவை)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        எனில் (Ast_முனை_கோவை* மாறிலி exprp = VN_CAST(nodep->exprp(), முனை_கோவை)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } இல்லையெனில் {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(உண்மை);
    }

    // DTYPES
    வெறுமை visit(Ast_முனை_Array_DType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed

        எனில் (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* மாறிலி basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            எனில் (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* மாறிலி newp = புதிய AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        எனில் (VN_IS(nodep, PackArrayDType)) m_underPackedArray = உண்மை;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        எனில் (AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            எனில் (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(உண்மை);
        } இல்லையெனில் {
            மாறிலி Ast_முனை_DType* மாறிலி elemDTypep = nodep->subDTypep()->skipRefp();
            எனில் (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            மாறிலி முழு width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            எனில் (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                மாறிலி Ast_முனை_DType* baseDTypep = nodep->subDTypep();
                போது (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                மாறிலி AstBasicDType* மாறிலி basicp = nodep->basicp();
                எனில் (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(உண்மை));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    வெறுமை visit(AstAssocArrayDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->சாவி_DTypep(iterateEditMoveDTypep(nodep, nodep->சாவி_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    வெறுமை visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_முனை_DType* மாறிலி childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_முனை* மாறிலி elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_முனை* newp;
        எனில் (VN_IS(elementsp, Unbounded)) {
            newp = புதிய AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_மரம்(), elementsp);
        } இல்லையெனில் எனில் (Ast_முனை_DType* மாறிலி keyp = VN_CAST(elementsp, முனை_DType)) {
            newp = புதிய AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } இல்லையெனில் {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_வரி* மாறிலி elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, உண்மை);
            // Must be expression that is constant, but we'll determine that later
            newp = புதிய AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                புதிய AstRange{nodep->fileline(), புதிய AstConst(elementsp->fileline(), 0),
                             புதிய AstSub{elementsNewFl, VN_AS(elementsp, முனை_கோவை),
                                        புதிய AstConst(elementsNewFl, 1)},
                             உண்மை}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    வெறுமை visit(AstConstraintRefDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    வெறுமை visit(AstDynArrayDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    வெறுமை visit(AstQueueDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        எனில் (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_மரம்();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    வெறுமை visit(AstVoidDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    வெறுமை visit(AstUnsizedArrayDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        எனில் (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    வெறுமை visit(AstWildcardArrayDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    வெறுமை visit(AstBasicDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        எனில் (nodep->generic()) திரும்பு;  // Already perfect
        எனில் (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } இல்லையெனில் எனில் (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } இல்லையெனில் எனில் (nodep->implicit()) {
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
    வெறுமை visit(AstConstDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    வெறுமை visit(AstRefDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(உண்மை);
        எனில் (nodep->typeofp()) {  // type(typeofp_expression)
            எனில் (Ast_முனை_DType* typeofDtp = VN_CAST(nodep->typeofp(), முனை_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } இல்லையெனில் {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_முனை* மாறிலி typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_மரம்(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        எனில் (nodep->subDTypep()) {
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
        எனில் (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            மாறிலி தருக்க inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            எனில் (inTemplateModule) {
                nodep->doingWidth(பொய்);
                திரும்பு;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(பொய்);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    வெறுமை widthDTypeNameParams(மாறிலி AstTypedef* nodep) {
        மாறிலி VDTypeNameScopes* மாறிலி scopesp = VDTypeNameScopes::currentp();
        எனில் (!scopesp) திரும்பு;  // Computing parameters, as by V3Param, so naming as found
        Ast_முனை* scopep = nullptr;
        க்கு (மாறிலி Ast_முனை* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            எனில் (மாறிலி std::vector<Ast_முனை*>* மாறிலி paramsp = scopesp->paramsp(scopep)) {
                க்கு (Ast_முனை* மாறிலி paramp : *paramsp) {
                    எனில் (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    வெறுமை visit(AstTypedef* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        எனில் (auto* மாறிலி refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        மாறிலி தருக்க declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        எனில் (declares) {
            Ast_முனை_DType* மாறிலி dtypep = nodep->dtypep();
            எனில் (Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep = VN_CAST(dtypep, முனை_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } இல்லையெனில் எனில் (AstEnumDType* மாறிலி edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    வெறுமை visit(AstParamTypeDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        எனில் (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    வெறுமை visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        எனில் (Ast_முனை_DType* மாறிலி dtp = VN_CAST(nodep->lhsp(), முனை_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } இல்லையெனில் {
            எனில் (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(புதிய AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_முனை_DType* மாறிலி toDtp = nodep->உச்சி()->dtypep()->skipRefToEnump();
        Ast_முனை_DType* மாறிலி fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_வரி* மாறிலி fl = nodep->fileline();
        மாறிலி auto castable = Ast_முனை::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_முனை* newp;
        எனில் (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            திரும்பு;
        } இல்லையெனில் எனில் (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* மாறிலி enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_முனை_கோவை* மாறிலி testp = enumTestValid(nodep->fromp(), enumDtp);
            File_வரி* மாறிலி fl_novalue = புதிய File_வரி{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, உண்மை);
            newp = புதிய AstCond{
                fl, testp,
                புதிய Ast_கோவை_Stmt{fl,
                                புதிய AstAssign{fl_novalue, nodep->உச்சி()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                புதிய AstConst{fl, AstConst::Signed32{}, 1}},
                புதிய AstConst{fl, AstConst::Signed32{}, 0}};
        } இல்லையெனில் எனில் ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = புதிய Ast_கோவை_Stmt{
                fl,
                புதிய AstAssign{fl, nodep->உச்சி()->unlinkFrBack(),
                              புதிய AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                புதிய AstConst{fl, AstConst::Signed32{}, 1}};
        } இல்லையெனில் எனில் (castable == VCastable::INCOMPATIBLE) {
            newp = புதிய AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } இல்லையெனில் {
            newp = புதிய AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    வெறுமை visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        எனில் (AstConst* மாறிலி constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_முனை* மாறிலி newp
                = புதிய AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } இல்லையெனில் எனில் (Ast_முனை_DType* மாறிலி refp = VN_CAST(nodep->dtp(), முனை_DType)) {
            refp->unlinkFrBack();
            Ast_முனை* மாறிலி newp = புதிய AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } இல்லையெனில் {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை visit(AstCast* nodep) override {
        எனில் (nodep->didWidth()) திரும்பு;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        எனில் (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            எனில் (AstSigned* மாறிலி fromp = VN_CAST(nodep->fromp(), Signed)) {
                எனில் (VN_IS(fromp->lhsp(), முனை_Stream)) {
                    Ast_முனை* மாறிலி lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_மரம்(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_முனை_DType* மாறிலி toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_முனை_DType* மாறிலி fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            மாறிலி auto castable = Ast_முனை::computeCastable(toDtp, fromDtp, nodep->fromp());
            தருக்க bad = பொய்;
            எனில் (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = உண்மை;
            } இல்லையெனில் எனில் (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } இல்லையெனில் எனில் (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = உண்மை;
            } இல்லையெனில் எனில் (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = உண்மை;
            } இல்லையெனில் {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_முனை_கோவை* newp = nullptr;
            எனில் (bad) {
            } இல்லையெனில் எனில் (AstBasicDType* மாறிலி basicp = toDtp->basicp()) {
                எனில் (!basicp->isString() && fromDtp->isString()) {
                    newp = புதிய AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } இல்லையெனில் எனில் (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_முனை_DType* மாறிலி origDTypep = nodep->dtypep();
                    எனில் (!VN_IS(fromDtp, StreamDType)) {
                        மாறிலி முழு width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } இல்லையெனில் {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 பொய்);
                }
                எனில் (newp) {
                } இல்லையெனில் எனில் (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    எனில் (nodep->fromp()->isSigned()) {
                        newp = புதிய AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } இல்லையெனில் {
                        newp = புதிய AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } இல்லையெனில் எனில் (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = புதிய AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } இல்லையெனில் எனில் (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = புதிய AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } இல்லையெனில் எனில் (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = புதிய AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } இல்லையெனில் {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } இல்லையெனில் எனில் (VN_IS(toDtp, QueueDType)) {
                எனில் (VN_IS(fromDtp, BasicDType)) {
                    newp = புதிய AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } இல்லையெனில் எனில் (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    முழு srcElementBits = 1;
                    முழு dstElementBits = 1;
                    எனில் (Ast_முனை_DType* மாறிலி elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    மாறிலி AstQueueDType* மாறிலி dstQueueDtp = VN_AS(toDtp, QueueDType);
                    எனில் (Ast_முனை_DType* மாறிலி elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = புதிய AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  பொய்,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } இல்லையெனில் எனில் (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } இல்லையெனில் {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            எனில் (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        எனில் (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, பொய், STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_முனை_கோவை* மாறிலி underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(உண்மை);
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(உண்மை);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            முழு width = nodep->rhsp()->toSInt();
            எனில் (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        எனில் (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_முனை* மாறிலி underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    வெறுமை visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    வெறுமை castSized(Ast_முனை* nodep, Ast_முனை* underp, முழு width) {
        மாறிலி AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        எனில் (!underDtp) underDtp = underp->dtypep()->basicp();
        எனில் (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            மாறிலி முழு calcWidth = std::max(width, underDtp->width());
            Ast_முனை_DType* மாறிலி calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         பொய், STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_முனை_DType* மாறிலி outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, முனை_கோவை), outDtp, EXTEND_EXP,
                            பொய்);
            VL_DANGLING(underp);
        }
    }
    வெறுமை visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(Ast_Constraint_கோவை* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    வெறுமை visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        எனில் (nodep->didWidth()) திரும்பு;
        எனில் (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            தள்ளு_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(உண்மை);
            திரும்பு;
        }
        nodep->doingWidth(உண்மை);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        எனில் (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.தள்ளு_back(nodep);
        எனில் (nodep->attrsp()) {
            nodep->attrsp()->foreach([இது, nodep](AstAttrOf* attrp) {
                எனில் (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    எனில் (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), முனை_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(தள்ளு_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        எனில் (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_முனை_DType* dtp = nodep->dtypep();
            Ast_முனை_DType* np = nullptr;
            போது (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                எனில் (மாறிலி AstDynArrayDType* மாறிலி unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    எனில் (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } இல்லையெனில் {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* மாறிலி newp
                        = புதிய AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    எனில் (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } இல்லையெனில் {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } இல்லையெனில் {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        எனில் (AstWildcardArrayDType* மாறிலி wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        எனில் (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(உண்மை);
        // Parameters if implicit untyped inherit from what they are assigned to
        மாறிலி AstBasicDType* மாறிலி bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        தருக்க didchk = பொய்;
        மாறிலி தருக்க implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        எனில் (implicitParam) {
            எனில் (nodep->valuep()) {
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
                எனில் (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } இல்லையெனில் எனில் (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } இல்லையெனில் {
                    முழு width = 0;
                    Ast_முனை_DType* மாறிலி மதிப்பு_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(மதிப்பு_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    மாறிலி AstBasicDType* மாறிலி மதிப்பு_Bdtypep = மதிப்பு_DTypep->basicp();
                    தருக்க issigned = பொய்;
                    எனில் (bdtypep->isNosign()) {
                        எனில் (மதிப்பு_Bdtypep && மதிப்பு_Bdtypep->isSigned()) issigned = உண்மை;
                    } இல்லையெனில் {
                        issigned = bdtypep->isSigned();
                    }
                    எனில் (மதிப்பு_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        எனில் (மாறிலி auto* மாறிலி constp = VN_CAST(nodep->valuep(), Const)) {
                            எனில் (constp->இலக்கம்().isString()) {
                                width = constp->இலக்கம்().toString().length() * 8;
                            }
                        }
                        எனில் (width < 8) width = 8;
                    } இல்லையெனில் எனில் (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } இல்லையெனில் {
                        எனில் (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    மாறிலி தருக்க is_மதிப்பு_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    எனில் (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_மதிப்பு_Fourstate));
                    } இல்லையெனில் {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_மதிப்பு_Fourstate);
                    }
                    didchk = உண்மை;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } இல்லையெனில் {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } இல்லையெனில் எனில் (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        எனில் (nodep->isNet()) {
            Ast_முனை_DType* மாறிலி badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            எனில் (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        எனில் (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(உண்மை);
        nodep->doingWidth(பொய்);
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        எனில் (nodep->didWidth()) திரும்பு;
        எனில் (!nodep->varp()) {
            எனில் (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, பொய்,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                திரும்பு;
            } இல்லையெனில் {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        எனில் (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        எனில் (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            திரும்பு;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        எனில் (VN_IS(nodep->backp(), முனை_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        எனில் (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(உண்மை);
        எனில் (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } இல்லையெனில் எனில் (nodep->access().isWriteOrRW() && nodep->varp()->is_உள்ளீடு()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            மாறிலி தருக்க hierRef = !m_curModVars.மொத்தம்(nodep->varp());
            மாறிலி தருக்க netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            மாறிலி தருக்க contAssign = VN_IS(nodep->abovep(), AssignW);
            மாறிலி தருக்க pinConn = VN_IS(nodep->abovep(), Pin);
            எனில் (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } இல்லையெனில் எனில் (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        எனில் (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(உண்மை);
    }

    வெறுமை visit(AstEnumDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_முனை_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_முனை_DType* மாறிலி badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        எனில் (badDtp) {
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
        V3_எண் இலக்கம்(nodep, nodep->width(), 0);
        மாறிலி V3_எண் one{nodep, nodep->width(), 1};
        தருக்க wrapAround = பொய்;
        std::map<மாறிலி V3_எண், AstEnumItem*> inits;
        க்கு (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            எனில் (itemp->valuep()) {
                எனில் (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_மரம்("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                எனில் (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_மரம்();
                    தொடர்;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = பொய்;  // Explicit value resets wrap-around tracking
            }
            எனில் (!itemp->valuep()) {
                எனில் (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                எனில் (இலக்கம்.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(புதிய AstConst{itemp->fileline(), இலக்கம்});
            }

            மாறிலி AstConst* மாறிலி constp = VN_AS(itemp->valuep(), Const);
            எனில் (constp->இலக்கம்().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            இலக்கம்.opAssign(constp->இலக்கம்());
            // Look for duplicates
            மாறிலி auto pair = inits.emplace(இலக்கம், itemp);
            எனில் (!pair.second) {  // IEEE says illegal
                மாறிலி Ast_முனை* மாறிலி otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            மாறிலி தருக்க prevNeg = constp->இலக்கம்().isNegative();
            இலக்கம்.opAdd(one, constp->இலக்கம்());
            எனில் (basicp->isSigned()) {
                wrapAround = !prevNeg && இலக்கம்.isNegative();
            } இல்லையெனில் {
                wrapAround = இலக்கம்.isEqZero();
            }
        }
    }
    வெறுமை visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_முனை_DType* மாறிலி vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        எனில் (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            தருக்க warnOn = உண்மை;
            Ast_முனை_கோவை* valuep = nodep->valuep();
            எனில் (மாறிலி AstAdd* மாறிலி anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                எனில் (anodep->fileline()->equal_First_வரி_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(valuep, Const)) {
                எனில் (static_cast<முழு>(constp->இலக்கம்().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = பொய்;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(புதிய AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    வெறுமை visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        எனில் (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_முனை* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            க்கு (; enump; enump = enump->backp()) {
                எனில் (VN_IS(enump, EnumDType)) நிறுத்து;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    வெறுமை visit(Ast_முனை_Cons* nodep) override {}  // Widthed when created
    வெறுமை visit(Ast_முனை_Cons_Item* nodep) override {}  // Widthed when created
    வெறுமை visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        க்கு (Ast_முனை *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_முனை_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        எனில் (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } இல்லையெனில் எனில் (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } இல்லையெனில் {
            // Take width as maximum across all items
            முழு width = nodep->exprp()->width();
            முழு mwidth = nodep->exprp()->widthMin();
            தருக்க isFourstate = nodep->exprp()->dtypep()->isFourstate();
            க்கு (மாறிலி Ast_முனை* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        க்கு (Ast_முனை *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            எனில் (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } இல்லையெனில் {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        எனில் (m_constraintp) {
            தருக்க canLower = உண்மை;
            க்கு (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                எனில் (VN_IS(ditemp->rangep(), InsideRange)) தொடர்;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                மாறிலி Ast_முனை_DType* மாறிலி itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                எனில் (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = பொய்;
                    நிறுத்து;
                }
            }
            எனில் (canLower) திரும்பு;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_முனை_கோவை* newp = nullptr;
        க்கு (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_முனை_கோவை* inewp
                = insideItem(nodep, nodep->exprp()->clone_மரம்_Pure(பொய்), itemp->rangep());
            எனில் (!inewp) தொடர்;
            Ast_முனை_கோவை* மாறிலி cmpp
                = புதிய AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            புதிய AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, உண்மை);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, உண்மை);
            inewp = புதிய AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? புதிய AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        எனில் (!newp) newp = புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        க்கு (Ast_முனை *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_முனை_DType* expDTypep = nullptr;

        எனில் (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } இல்லையெனில் எனில் (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } இல்லையெனில் {
            // Take width as maximum across all items
            முழு width = nodep->exprp()->width();
            முழு mwidth = nodep->exprp()->widthMin();
            தருக்க isFourstate = nodep->exprp()->dtypep()->isFourstate();
            க்கு (மாறிலி Ast_முனை* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            மாறிலி VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        க்கு (Ast_முனை *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            எனில் (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } இல்லையெனில் எனில் (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_முனை_கோவை* exprp;
        Ast_கோவை_Stmt* கோவை_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        எனில் (!m_constraintp && !nodep->exprp()->isPure()) {
            File_வரி* மாறிலி fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_முனை_DType* மாறிலி கோவை_Dtp = nodep->exprp()->dtypep();
            மாறிலி முழு w = கோவை_Dtp->width();
            Ast_முனை_DType* மாறிலி tempDTypep
                = கோவை_Dtp->widthSized() ? கோவை_Dtp
                                        : nodep->findBitOrLogicDType(w, w, கோவை_Dtp->numeric(),
                                                                     கோவை_Dtp->isFourstate());
            AstVar* மாறிலி varp
                = புதிய AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = புதிய AstVarRef{fl, varp, VAccess::READ};
            கோவை_Stmtp = புதிய Ast_கோவை_Stmt{fl,
                                        புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            எனில் (m_ftaskp) {
                varp->funcLocal(உண்மை);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } இல்லையெனில் {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(கோவை_Stmtp);
        } இல்லையெனில் {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_முனை_கோவை* newp = nullptr;
        க்கு (Ast_முனை_கோவை *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), முனை_கோவை);  // Will be unlinking
            Ast_முனை_கோவை* inewp;
            எனில் (கோவை_Stmtp) {
                inewp = insideItem(nodep, கோவை_Stmtp, itemp);
                கோவை_Stmtp = nullptr;
            } இல்லையெனில் {
                inewp = insideItem(nodep, exprp->clone_மரம்_Pure(பொய்), itemp);
            }
            எனில் (!inewp) தொடர்;
            newp = newp ? புதிய AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        எனில் (கோவை_Stmtp) VL_DO_DANGLING(கோவை_Stmtp->delete_மரம்(), கோவை_Stmtp);
        எனில் (!newp) newp = புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    Ast_முனை_கோவை* insideItem(Ast_முனை* nodep, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* itemp) {
        மாறிலி Ast_முனை_DType* மாறிலி itemDtp = itemp->dtypep()->skipRefp();
        எனில் (AstInsideRange* மாறிலி irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            திரும்பு irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } இல்லையெனில் எனில் (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_முனை_கோவை* மாறிலி inewp = புதிய AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(உண்மை);
            திரும்பு inewp;
        } இல்லையெனில் எனில் (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_மரம்(), exprp);
            திரும்பு nullptr;
        }
        திரும்பு AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    வெறுமை visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        எனில் (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } இல்லையெனில் {
            எனில் (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            எனில் (m_vup->final()) {
                Ast_முனை_DType* மாறிலி expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                மாறிலி முழு expWidth = expDTypep->width();
                மாறிலி தருக்க waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                மாறிலி தருக்க waiveRhs = expWidth == 32
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

    வெறுமை visit(AstIfaceRefDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        எனில் (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* nodep) override {
        எனில் (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(பொய்);
            திரும்பு;
        }
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(உண்மை);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        எனில் (மாறிலி AstUnionDType* மாறிலி unionp = VN_CAST(nodep, UnionDType)) {
            எனில் (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(பொய்);
        // Error checks
        க்கு (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_முனை_DType* மாறிலி dtp = itemp->subDTypep()->skipRefp();
            எனில் (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            எனில் ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                தள்ளு_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        மாறிலி தருக்க isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        மாறிலி தருக்க inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        எனில் (VN_IS(nodep, UnionDType) || nodep->packed()) {
            முழு lsb = 0;
            முழு width = 0;
            தருக்க first = உண்மை;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            க்கு (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            க்கு (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                எனில் (itemp->isFourstate()) nodep->isFourstate(உண்மை);
                itemp->lsb(lsb);
                எனில் (VN_IS(nodep, UnionDType)) {
                    மாறிலி முழு itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    எனில் (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } இல்லையெனில் {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = பொய்;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } இல்லையெனில் {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(பொய்);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    வெறுமை visit(AstThisRef* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        எனில் (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    வெறுமை visit(AstClassRefDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    வெறுமை visit(AstClassOrPackageRef* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        userIterateChildren(nodep, nullptr);
    }
    வெறுமை visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstClassExtends* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        எனில் (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    வெறுமை visit(AstMemberDType* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        எனில் (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    வெறுமை visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        எனில் (nodep->didWidth()) திரும்பு;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = உண்மை;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_முனை_DType* மாறிலி fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        மாறிலி AstMemberSel* மாறிலி fromSel = VN_CAST(nodep->fromp(), MemberSel);
        எனில் (AstClocking* மாறிலி clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            எனில் (AstVar* மாறிலி varp = memberSelClocking(nodep, clockingp)) {
                எனில் (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_மரம்(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                எனில் (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                எனில் (AstIfaceRefDType* மாறிலி adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(உண்மை);  // Must not visit again: will confuse scopes
                திரும்பு;
            }
        } இல்லையெனில் எனில் (Ast_முனை_UOr_Struct_DType* மாறிலி adtypep = VN_CAST(fromDtp, முனை_UOr_Struct_DType)) {
            எனில் (memberSelStruct(nodep, adtypep)) திரும்பு;
        } இல்லையெனில் எனில் (AstClassRefDType* மாறிலி adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            எனில் (memberSelClass(nodep, adtypep)) திரும்பு;
        } இல்லையெனில் எனில் (AstIfaceRefDType* மாறிலி adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            எனில் (Ast_முனை* foundp = memberSelIface(nodep, adtypep)) {
                எனில் (AstClocking* மாறிலி clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                எனில் (AstVar* மாறிலி varp = VN_CAST(foundp, Var)) {
                    எனில் (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* மாறிலி ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    மாறிலி AstVarRef* மாறிலி refp = VN_CAST(nodep->fromp(), VarRef);
                    எனில் (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* மாறிலி newp = புதிய AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(உண்மை);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                        திரும்பு;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(உண்மை);
                    திரும்பு;
                }
                எனில் (AstModport* மாறிலி modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* மாறிலி newDtypep = புதிய AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_முனை_கோவை* மாறிலி fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
                எனில் (AstCell* மாறிலி cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    எனில் (VN_IS(cellp->modp(), Iface)) {
                        மாறிலி string viftopName = cellp->name() + "__Viftop";
                        Ast_முனை_Module* மாறிலி parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        க்கு (Ast_முனை* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            எனில் (AstVar* மாறிலி vp = VN_CAST(itemp, Var)) {
                                எனில் (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    நிறுத்து;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        எனில் (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_முனை_DType* subDtypep = viftopVarp->dtypep();
                        எனில் (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            எனில் (AstIfaceRefDType* மாறிலி subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* மாறிலி newDtypep = புதிய AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(உண்மை);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(உண்மை);
                        திரும்பு;
                    }
                }
                எனில் (Ast_முனை_FTask* மாறிலி ftaskp = VN_CAST(foundp, முனை_FTask)) {
                    AstMethodCall* மாறிலி newp = புதிய AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    திரும்பு;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } இல்லையெனில் எனில் (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_முனை* மாறிலி newp = புதிய AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            திரும்பு;
        } இல்லையெனில் {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    தருக்க memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        எனில் (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* மாறிலி newp = புதிய AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            visit(newp);
            திரும்பு உண்மை;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* மாறிலி first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        க்கு (AstClass* classp = first_classp; classp;) {
            எனில் (Ast_முனை* மாறிலி foundp = m_memberMap.findMember(classp, nodep->name())) {
                எனில் (AstVar* மாறிலி varp = VN_CAST(foundp, Var)) {
                    எனில் (!varp->didWidth()) userIterate(varp, nullptr);
                    எனில் (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* மாறிலி varRefp
                            = புதிய AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                        திரும்பு உண்மை;
                    }
                    எனில் (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(உண்மை);
                    எனில் (nodep->fromp()->same_மரம்(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    திரும்பு உண்மை;
                }
                எனில் (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(புதிய AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு உண்மை;
                }
                எனில் (AstEnumItemRef* மாறிலி adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_மரம்(பொய்));
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு உண்மை;
                }
                எனில் (Ast_முனை_FTask* ftaskp = VN_CAST(foundp, முனை_FTask)) {
                    AstMethodCall* newp = புதிய AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு உண்மை;
                }
                எனில் (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    திரும்பு உண்மை;
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
        க்கு (AstClass* classp = first_classp; classp;) {
            க்கு (Ast_முனை* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                எனில் (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.தள்ளு_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        மாறிலி string suggest = speller.best_Candidate_செய்தி(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        திரும்பு பொய்;  // Caller handles error
    }
    தருக்க modportAllowsItem(Ast_முனை* nodep, மாறிலி AstIfaceRefDType* adtypep, மாறிலி Ast_முனை* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        மாறிலி AstModport* மாறிலி modportp = adtypep->modportp();
        எனில் (!modportp) திரும்பு உண்மை;
        எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(itemp, Var)) {
            எனில் (varp->isParam()) திரும்பு உண்மை;
        } இல்லையெனில் எனில் (!VN_IS(itemp, முனை_FTask) && !VN_IS(itemp, Clocking)) {
            திரும்பு உண்மை;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        எனில் (pair.second) {
            க்கு (மாறிலி Ast_முனை* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.செருகு(mpItemp->name());
            }
        }
        எனில் (pair.first->second.மொத்தம்(itemp->name())) திரும்பு உண்மை;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        திரும்பு பொய்;
    }
    Ast_முனை* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_முனை_Module* மாறிலி ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        க்கு (Ast_முனை* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            எனில் (itemp->name() == nodep->name()) {
                எனில் (!modportAllowsItem(nodep, adtypep, itemp)) {
                    திரும்பு nullptr;  // Caller handles error
                }
                திரும்பு itemp;
            }
            எனில் (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.தள்ளு_Candidate(itemp->prettyName());
            }
        }
        மாறிலி string suggest = speller.best_Candidate_செய்தி(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        திரும்பு nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        க்கு (Ast_முனை* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            எனில் (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                எனில் (citemp->varp()->name() == nodep->name()) திரும்பு citemp->varp();
                speller.தள்ளு_Candidate(citemp->varp()->prettyName());
            }
        }
        மாறிலி string suggest = speller.best_Candidate_செய்தி(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        திரும்பு nullptr;  // Caller handles error
    }
    தருக்க memberSelStruct(AstMemberSel* nodep, Ast_முனை_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        எனில் (AstMemberDType* மாறிலி memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            எனில் (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } இல்லையெனில் எனில் (adtypep->packed()) {
                AstSel* மாறிலி newp = புதிய AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(உண்மை);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } இல்லையெனில் {
                AstStructSel* மாறிலி newp = புதிய AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(உண்மை);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            திரும்பு உண்மை;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        திரும்பு பொய்;
    }

    வெறுமை visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    வெறுமை visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        எனில் (nodep->didWidth()) திரும்பு;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_முனை_DType* மாறிலி fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* மாறிலி basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        எனில் (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } இல்லையெனில் எனில் (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } இல்லையெனில் எனில் (AstEnumDType* மாறிலி adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } இல்லையெனில் எனில் (AstAssocArrayDType* மாறிலி adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } இல்லையெனில் எனில் (AstWildcardArrayDType* மாறிலி adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } இல்லையெனில் எனில் (AstDynArrayDType* மாறிலி adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } இல்லையெனில் எனில் (AstQueueDType* மாறிலி adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } இல்லையெனில் எனில் (AstClassRefDType* மாறிலி adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } இல்லையெனில் எனில் (AstIfaceRefDType* மாறிலி adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } இல்லையெனில் எனில் (AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } இல்லையெனில் எனில் (AstConstraintRefDType* மாறிலி adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } இல்லையெனில் எனில் (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } இல்லையெனில் எனில் (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } இல்லையெனில் {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_முனை_FTask_Ref* nodep, தருக்க required, தருக்க arbReturn,
                              Ast_முனை_DType* returnDtp, Ast_முனை_DType* indexDtp,
                              Ast_முனை_DType* மதிப்பு_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        எனில் (AstWith* மாறிலி withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->மதிப்பு_Arg_Refp()->dtypep(மதிப்பு_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            திரும்பு withp;
        }
        எனில் (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        திரும்பு nullptr;
    }
    வெறுமை methodOkArguments(Ast_முனை_FTask_Ref* nodep, முழு minArg, முழு maxArg) {
        முழு narg = 0;
        எனில் (AstWith* மாறிலி withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(தள்ளு_Deletep(withp->unlinkFrBack()), withp);
        }
        க்கு (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        மாறிலி தருக்க ok = (narg >= minArg) && (narg <= maxArg);
        எனில் (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            க்கு (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    புதிய AstArg{nodep->fileline(), "",
                               புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            க்கு (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                போது (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_மரம்(), argp);
            }
        }
    }

    Ast_முனை_கோவை* methodArg(AstMethodCall* nodep, முழு arg) {
        AstArg* argp = nodep->argsp();
        க்கு (முழு narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        திரும்பு argp->exprp();
    }

    வெறுமை methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        எனில் (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            எனில் (nodep->name() == "num") {
                முழு பொருட்கள் = 0;
                க்கு (Ast_முனை* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++பொருட்கள்;
                newp = புதிய AstConst(nodep->fileline(), AstConst::Signed32{}, பொருட்கள்);
            } இல்லையெனில் எனில் (nodep->name() == "first") {
                மாறிலி AstEnumItem* itemp = adtypep->itemsp();
                எனில் (!itemp) {
                    newp = புதிய AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } இல்லையெனில் {
                    newp = VN_AS(itemp->valuep()->clone_மரம்(பொய்), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } இல்லையெனில் எனில் (nodep->name() == "last") {
                மாறிலி AstEnumItem* itemp = adtypep->itemsp();
                போது (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                எனில் (!itemp) {
                    newp = புதிய AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } இல்லையெனில் {
                    newp = VN_AS(itemp->valuep()->clone_மரம்(பொய்), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            எனில் (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } இல்லையெனில் எனில் (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } இல்லையெனில் எனில் (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } இல்லையெனில் {
                nodep->v3fatalSrc("Bad case");
            }

            எனில் (nodep->name() != "name" && nodep->argsp()) {
                Ast_முனை_கோவை* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, உண்மை);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                எனில் (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_முனை_கோவை* மாறிலி newp = புதிய AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(stepp), stepp);
                    stepp = newp;
                }
                மாறிலி uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                எனில் (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* மாறிலி newp = புதிய AstMethodCall{
                        nodep->fileline(),
                        புதிய AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
                    திரும்பு;
                } இல்லையெனில் எனில் (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    தள்ளு_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* மாறிலி clonep = nodep->clone_மரம்(பொய்);
                    VN_AS(stepp, Const)->இலக்கம்().setLong(1);
                    AstConst* மாறிலி constp = புதிய AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* மாறிலி argp = புதிய AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* மாறிலி newp
                        = புதிய AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
                    திரும்பு;
                }
            }
            Ast_முனை_கோவை* மாறிலி newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    வெறுமை methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        எனில் (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } இல்லையெனில் எனில் (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } இல்லையெனில் எனில் (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_முனை_கோவை* மாறிலி index_exprp = method_Call_Wildcard_Index_கோவை(nodep, adtypep);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } இல்லையெனில் எனில் (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            எனில் (!nodep->argsp()) {
                newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } இல்லையெனில் {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_முனை_கோவை* மாறிலி index_exprp = method_Call_Wildcard_Index_கோவை(nodep, adtypep);
                newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } இல்லையெனில் எனில் (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } இல்லையெனில் எனில் (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* மாறிலி withp
                = methodWithClause(nodep, பொய், பொய், adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* மாறிலி withp
                = methodWithClause(nodep, உண்மை, பொய், nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "map") {
            AstWith* மாறிலி withp
                = methodWithClause(nodep, உண்மை, பொய், adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        எனில் (newp) {
            newp->protect(பொய்);
            newp->didWidth(உண்மை);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        }
    }
    வெறுமை methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        எனில் (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        எனில் (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } இல்லையெனில் எனில் (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->சாவி_DTypep(), BOTH);
            Ast_முனை_கோவை* மாறிலி index_exprp = method_Call_Assoc_Index_கோவை(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->சாவி_DTypep(), BOTH);
            Ast_முனை_கோவை* மாறிலி index_exprp = method_Call_Assoc_Index_கோவை(nodep, adtypep);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } இல்லையெனில் எனில் (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            எனில் (!nodep->argsp()) {
                newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } இல்லையெனில் {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->சாவி_DTypep(),
                                  BOTH);
                Ast_முனை_கோவை* மாறிலி index_exprp = method_Call_Assoc_Index_கோவை(nodep, adtypep);
                newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } இல்லையெனில் எனில் (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } இல்லையெனில் எனில் (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* மாறிலி withp = methodWithClause(nodep, பொய், பொய், adtypep->subDTypep(),
                                                    adtypep->சாவி_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* மாறிலி withp = methodWithClause(
                nodep, பொய், உண்மை, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            எனில் (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->சாவி_DTypep()));
            } இல்லையெனில் {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* மாறிலி withp = methodWithClause(nodep, உண்மை, பொய், nodep->findBitDType(),
                                                    adtypep->சாவி_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* மாறிலி withp = methodWithClause(nodep, உண்மை, பொய், nodep->findBitDType(),
                                                    adtypep->சாவி_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->சாவி_DTypep()));
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "map") {
            AstWith* மாறிலி withp = methodWithClause(nodep, உண்மை, பொய், adtypep->subDTypep(),
                                                    adtypep->சாவி_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        எனில் (newp) {
            newp->protect(பொய்);
            newp->didWidth(உண்மை);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        }
    }
    Ast_முனை_கோவை* method_Call_Assoc_Index_கோவை(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_முனை* மாறிலி index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->சாவி_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        திரும்பு nodep->argsp()->exprp();
    }
    Ast_முனை_கோவை* method_Call_Wildcard_Index_கோவை(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_முனை* மாறிலி index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        திரும்பு nodep->argsp()->exprp();
    }
    வெறுமை methodCallLValueRecurse(AstMethodCall* nodep, Ast_முனை* childp, மாறிலி VAccess& access) {
        எனில் (AstCMethodHard* மாறிலி ichildp = VN_CAST(childp, CMethodHard)) {
            தருக்க methAt = பொய்;
            தருக்க methAtBack = பொய்;
            தருக்க methAtWrite = பொய்;
            தேர்வு (ichildp->method()) {
            நிலை VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = உண்மை;
                நிறுத்து;
            நிலை VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = உண்மை;
                நிறுத்து;
            நிலை VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            நிலை VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            நிலை VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = உண்மை;
                நிறுத்து;
            இயல்பு: நிறுத்து;
            }
            எனில் (methAt || methAtBack || methAtWrite) {
                மாறிலி Ast_முனை_DType* மாறிலி fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                எனில் (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    எனில் (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } இல்லையெனில் எனில் (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                திரும்பு;
            }
        }
        எனில் (Ast_முனை_Var_Ref* மாறிலி varrefp = VN_CAST(childp, முனை_Var_Ref)) {
            varrefp->access(access);
        } இல்லையெனில் எனில் (மாறிலி AstMemberSel* மாறிலி ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } இல்லையெனில் எனில் (மாறிலி AstStructSel* மாறிலி ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_Sel* மாறிலி ichildp = VN_CAST(childp, முனை_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } இல்லையெனில் எனில் (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } இல்லையெனில் {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_முனை_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        எனில் (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            எனில் (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, பொய், உண்மை, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* மாறிலி withp = methodWithClause(
                nodep, பொய், உண்மை, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            எனில் (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } இல்லையெனில் {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* மாறிலி withp
                = methodWithClause(nodep, உண்மை, பொய், nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* மாறிலி withp
                = methodWithClause(nodep, உண்மை, பொய், nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* மாறிலி withp
                = methodWithClause(nodep, உண்மை, பொய், adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        திரும்பு newp;
    }
    வெறுமை methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        எனில் (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } இல்லையெனில் எனில் (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } இல்லையெனில் எனில் (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } இல்லையெனில் எனில் (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* மாறிலி withp
                = methodWithClause(nodep, பொய், பொய், adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் ((newp = methodCallArray(nodep, adtypep))) {
        } இல்லையெனில் {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        எனில் (newp) {
            newp->protect(பொய்);
            newp->didWidth(உண்மை);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        }
    }
    வெறுமை methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        எனில் (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } இல்லையெனில் எனில் (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } இல்லையெனில் எனில் (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } இல்லையெனில் எனில் (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            எனில் (!nodep->argsp()) {
                newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } இல்லையெனில் {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_முனை_கோவை* மாறிலி index_exprp = method_Call_Queue_Index_கோவை(nodep);
                எனில் (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_எடு_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } இல்லையெனில் {
                    newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } இல்லையெனில் எனில் (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_முனை_கோவை* மாறிலி index_exprp = method_Call_Queue_Index_கோவை(nodep);
            AstArg* மாறிலி argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            எனில் (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_தள்ளு_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } இல்லையெனில் {
                newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_செருகு, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } இல்லையெனில் எனில் (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } இல்லையெனில் எனில் (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* மாறிலி argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* மாறிலி withp
                = methodWithClause(nodep, பொய், பொய், adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } இல்லையெனில் எனில் ((newp = methodCallArray(nodep, adtypep))) {
        } இல்லையெனில் {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        எனில் (newp) {
            newp->protect(பொய்);
            newp->didWidth(உண்மை);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        }
    }
    Ast_முனை_கோவை* method_Call_Queue_Index_கோவை(AstMethodCall* nodep) {
        Ast_முனை* மாறிலி index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        திரும்பு nodep->argsp()->exprp();
    }
    வெறுமை methodCallWarnTiming(Ast_முனை_FTask_Ref* மாறிலி nodep, மாறிலி std::string& className) {
        எனில் (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } இல்லையெனில் எனில் (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    வெறுமை methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* மாறிலி ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        எனில் (Ast_முனை_FTask* மாறிலி ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), முனை_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            எனில் (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                திரும்பு;
            }
            எனில் (adtypep->isVirtual()) {
                க்கு (Ast_முனை* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    எனில் (AstVar* மாறிலி mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            எனில் (ftaskp->isStatic()) {
                AstArg* மாறிலி argsp = nodep->argsp();
                எனில் (argsp) argsp->unlinkFrBackWithNext();
                Ast_முனை_FTask_Ref* newp = nullptr;
                எனில் (VN_IS(ftaskp, Task)) {
                    newp = புதிய AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } இல்லையெனில் {
                    newp = புதிய AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            } இல்லையெனில் {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                எனில் (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            திரும்பு;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    வெறுமை handleRandomizeArgs(Ast_முனை_FTask_Ref* மாறிலி nodep, AstClass* மாறிலி classp) {
        தருக்க hasNonNullArgs = பொய்;
        AstConst* nullp = nullptr;
        க்கு (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_முனை_கோவை* exprp = argp->exprp();
            எனில் (AstConst* மாறிலி constp = VN_CAST(exprp, Const)) {
                எனில் (constp->இலக்கம்().isNull()) {
                    nullp = constp;
                    தொடர்;
                }
            }
            hasNonNullArgs = உண்மை;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            எனில் (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_முனை_கோவை* fromp = methodCallp->fromp();
                போது (AstMemberSel* மாறிலி memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* மாறிலி varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            எனில் (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_மரம்(), argp);
                தொடர்;
            }
            போது (exprp) {
                எனில் (AstMemberSel* மாறிலி memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } இல்லையெனில் {
                    எனில் (AstVarRef* மாறிலி varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } இல்லையெனில் {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_மரம்(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                எனில் (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            எனில் (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } இல்லையெனில் எனில் (classp->existsMember([&](மாறிலி AstClass*, மாறிலி AstVar* memberVarp) {
                           திரும்பு memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } இல்லையெனில் {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_மரம்(), argp);
            }
        }
        எனில் (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    வெறுமை methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* மாறிலி first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        எனில் (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, பொய், பொய், adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            க்கு (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                எனில் (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } இல்லையெனில் எனில் (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        க்கு (AstClass* classp = first_classp; classp;) {
            எனில் (nodep->fileline()->timingOn()) {
                எனில் (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* மாறிலி packagep = getItemPackage(classp);
                    // Check if it's std
                    எனில் (packagep && packagep->name() == "std") {
                        எனில் (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } இல்லையெனில் எனில் (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } இல்லையெனில் எனில் (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            எனில் (Ast_முனை_FTask* மாறிலி ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), முனை_FTask)) {
                userIterate(ftaskp, nullptr);
                எனில் (ftaskp->isStatic()) {
                    AstArg* மாறிலி argsp = nodep->argsp();
                    எனில் (argsp) argsp->unlinkFrBackWithNext();
                    Ast_முனை_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    எனில் (!m_vup) {  // Called as task
                        newp = புதிய AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } இல்லையெனில் {
                        newp = புதிய AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
                } இல்லையெனில் {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    எனில் (VN_IS(ftaskp, Task)) {
                        எனில் (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_கோவை)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    எனில் (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                திரும்பு;
            } இல்லையெனில் எனில் (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(உண்மை);
                v3Global.useRandomizeMethods(உண்மை);
                AstCMethodHard* மாறிலி newp
                    = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(உண்மை);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            } இல்லையெனில் எனில் (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_முனை_கோவை* மாறிலி expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_முனை_கோவை* மாறிலி exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(உண்மை);
                v3Global.useRandomizeMethods(உண்மை);
                AstCMethodHard* மாறிலி newp
                    = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(உண்மை);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            } இல்லையெனில் எனில் (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(உண்மை);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            க்கு (AstClass* classp = first_classp; classp;) {
                க்கு (Ast_முனை* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    எனில் (VN_IS(itemp, முனை_FTask)) speller.தள்ளு_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            மாறிலி string suggest = speller.best_Candidate_செய்தி(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    வெறுமை methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        எனில் (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            எனில் (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } இல்லையெனில் {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(உண்மை);
        } இல்லையெனில் {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        எனில் (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } இல்லையெனில் {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(உண்மை);
    }
    வெறுமை methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        எண்ணிக்கை : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        எனில் (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } இல்லையெனில் எனில் (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } இல்லையெனில் எனில் (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } இல்லையெனில் எனில் (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } இல்லையெனில் எனில் (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        எனில் (methodId) {
            AstWith* மாறிலி withp
                = methodWithClause(nodep, பொய், பொய், adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            எனில் (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* மாறிலி newp
                    = புதிய AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                எனில் (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(பொய்);
                newp->didWidth(உண்மை);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
                திரும்பு;
            } இல்லையெனில் {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_வரி* மாறிலி fl = nodep->fileline();
                Ast_முனை_கோவை* newp = nullptr;
                க்கு (முழு i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_முனை_கோவை* மாறிலி arrayRef = nodep->fromp()->clone_மரம்_Pure(பொய்);
                    Ast_முனை_கோவை* மாறிலி selector = புதிய AstArraySel{fl, arrayRef, i};
                    எனில் (!newp) {
                        newp = selector;
                    } இல்லையெனில் {
                        தேர்வு (methodId) {
                        நிலை ARRAY_OR: newp = புதிய AstOr{fl, newp, selector}; நிறுத்து;
                        நிலை ARRAY_AND: newp = புதிய AstAnd{fl, newp, selector}; நிறுத்து;
                        நிலை ARRAY_XOR: newp = புதிய AstXor{fl, newp, selector}; நிறுத்து;
                        நிலை ARRAY_SUM: newp = புதிய AstAdd{fl, newp, selector}; நிறுத்து;
                        நிலை ARRAY_PRODUCT: newp = புதிய AstMul{fl, newp, selector}; நிறுத்து;
                        இயல்பு: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            }
        } இல்லையெனில் எனில் (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(பொய்);
            newp->didWidth(உண்மை);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    வெறுமை methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        எனில் (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* மாறிலி callp = புதிய AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    வெறுமை methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        எனில் (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_முனை* மாறிலி newp = புதிய AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_முனை* மாறிலி newp
                = புதிய AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_முனை* மாறிலி newp
                = புதிய AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "compare" || nodep->name() == "icompare") {
            மாறிலி தருக்க ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* மாறிலி argp = nodep->argsp();
            Ast_முனை_கோவை* மாறிலி lhs = nodep->fromp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rhs = argp->exprp()->unlinkFrBack();
            Ast_முனை* மாறிலி newp = புதிய AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* மாறிலி arg0p = nodep->argsp();
            AstArg* மாறிலி arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_முனை_Var_Ref* மாறிலி fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_முனை_கோவை* மாறிலி rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* மாறிலி varrefp
                = புதிய AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_முனை* மாறிலி newp = புதிய AstAssign{
                nodep->fileline(), fromp, புதிய AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            தள்ளு_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* மாறிலி arg0p = nodep->argsp();
            Ast_முனை_கோவை* மாறிலி lhsp = nodep->fromp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* மாறிலி arg0p = nodep->argsp();
            AstArg* மாறிலி arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_முனை_கோவை* மாறிலி lhsp = nodep->fromp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி thsp = arg1p->exprp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            எனில் (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } இல்லையெனில் எனில் (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } இல்லையெனில் எனில் (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } இல்லையெனில் எனில் (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } இல்லையெனில் எனில் (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } இல்லையெனில் {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_முனை* மாறிலி newp
                = புதிய AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_முனை_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        எனில் (AstQueueDType* மாறிலி queuep = m_queueDTypeIndexed[indexDTypep]) {
            திரும்பு queuep;
        } இல்லையெனில் {
            AstQueueDType* மாறிலி newp
                = புதிய AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            திரும்பு newp;
        }
    }

    வெறுமை visit(AstNew* nodep) override {
        எனில் (nodep->didWidth()) திரும்பு;
        AstClass* classp = nullptr;
        தருக்க assign = பொய்;
        எனில் (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = உண்மை;
            Ast_முனை* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            எனில் (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                எனில் (AstClass* மாறிலி clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* மாறிலி adtypep
                        = புதிய AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } இல்லையெனில் {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            எனில் (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, முனை_DType)
                           ? ("data type "s + VN_AS(warnp, முனை_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                திரும்பு;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            எனில் (Ast_முனை_FTask* மாறிலி ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } இல்லையெனில் {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } இல்லையெனில் {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        எனில் (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    வெறுமை visit(AstNewCopy* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        AstClassRefDType* மாறிலி refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        எனில் (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            திரும்பு;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        எனில் (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    வெறுமை visit(AstNewDynamic* nodep) override {
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        AstDynArrayDType* மாறிலி adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        எனில் (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            திரும்பு;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        எனில் (!VN_IS(nodep->backp(), முனை_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            திரும்பு;
        }
        nodep->dtypep(adtypep);
        எனில் (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        எனில் (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* மாறிலி consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            எனில் (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        }
    }

    வெறுமை visit(Ast_Tagged_கோவை* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    வெறுமை visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    வெறுமை visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    வெறுமை visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    வெறுமை visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    வெறுமை visit(AstPattern* nodep) override {
        எனில் (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            க்கு (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = பொய்;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                எனில் (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), முனை_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                க்கு (Ast_முனை *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            திரும்பு;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = பொய்;
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        UINFO(9, "PATTERN " << nodep);
        எனில் (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        எனில் (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_முனை_DType* dtypep = nodep->dtypep();
        எனில் (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            எனில் (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                திரும்பு;
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            எனில் (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                திரும்பு;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            க்கு (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                மாறிலி முழு times = visitPatMemberRep(patp);
                க்கு (முழு i = 1; i < times; i++) {
                    AstPatMember* மாறிலி newp = patp->clone_மரம்(பொய்);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            க்கு (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                எனில் (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    போது (Ast_முனை_கோவை* மாறிலி movep = VN_AS(patp->lhssp()->nextp(), முனை_கோவை)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_முனை* newkeyp = nullptr;
                        எனில் (patp->keyp()) newkeyp = patp->keyp()->clone_மரம்(உண்மை);
                        AstPatMember* மாறிலி newp
                            = புதிய AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            க்கு (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* மாறிலி nextp = VN_AS(patp->nextp(), PatMember);
                எனில் (patp->isDefault()) {
                    எனில் (defaultp) {
                        VL_DO_DANGLING(தள்ளு_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            போது (மாறிலி AstConstDType* மாறிலி vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            எனில் (auto* மாறிலி vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } இல்லையெனில் எனில் (auto* மாறிலி vdtypep = VN_CAST(dtypep, முனை_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } இல்லையெனில் எனில் (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* மாறிலி consp = புதிய AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } இல்லையெனில் எனில் (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* மாறிலி consp = புதிய AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } இல்லையெனில் எனில் (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } இல்லையெனில் எனில் (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } இல்லையெனில் {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            எனில் (defaultp) VL_DO_DANGLING(தள்ளு_Deletep(defaultp), defaultp);
        }
    }
    வெறுமை patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<மாறிலி AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            மாறிலி AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            போது (patp) {
                செய் {
                    எனில் (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        எனில் (மாறிலி AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            எனில் (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                நிறுத்து;
                            } இல்லையெனில் {
                                மாறிலி std::pair<PatMap::iterator, தருக்க> ret
                                    = patmap.emplace(memp, patp);
                                எனில் (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } இல்லையெனில் எனில் (Ast_முனை_DType* nodedtypep = VN_CAST(patp->keyp(), முனை_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            மாறிலி string dtype = nodedtypep->dtypep()->prettyDTypeName(உண்மை);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } இல்லையெனில் {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } இல்லையெனில் எனில் (memp) {
                        // constant expr
                        மாறிலி std::pair<PatMap::iterator, தருக்க> ret = patmap.emplace(memp, patp);
                        எனில் (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } இல்லையெனில் {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } போது (பொய்);

                // Next
                எனில் (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_முனை_கோவை* newp = nullptr;
        எனில் (vdtypep->packed()) {
            க்கு (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                மாறிலி auto it = patmap.find(memp);
                எனில் (it == patmap.end()) {  // default or default_type assignment
                    எனில் (AstStructDType* மாறிலி memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } இல்லையெனில் {
                        AstPatMember* மாறிலி patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = மதிப்பு_Concat_pattern_Struct(patp, newp, memp, nodep);
                        எனில் (patp) VL_DO_DANGLING(தள்ளு_Deletep(patp), patp);
                    }
                } இல்லையெனில் {  // member assignment
                    newp = மதிப்பு_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } இல்லையெனில் {  // Unpacked
            AstConsStruct* மாறிலி consp = புதிய AstConsStruct{nodep->fileline(), vdtypep};
            க்கு (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                மாறிலி auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                எனில் (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    தள்ளு_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    தள்ளு_Deletep(patp);  // patp used below
                } இல்லையெனில் {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_முனை_கோவை* மாறிலி valuep = pattern_Member_மதிப்பு_Iterate(patp);
                consp->addItemsp(புதிய AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        எனில் (newp) {
            nodep->replaceWith(newp);
        } இல்லையெனில் {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_முனை_கோவை* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_முனை_கோவை* newp,
                                                 AstPattern* nodep, மாறிலி DTypeMap& dtypemap) {
        க்கு (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            எனில் (AstStructDType* மாறிலி memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } இல்லையெனில் {
                AstPatMember* மாறிலி patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = மதிப்பு_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                எனில் (patp) VL_DO_DANGLING(தள்ளு_Deletep(patp), patp);
            }
        }
        திரும்பு newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, மாறிலி DTypeMap& dtypemap) {
        மாறிலி string memp_DType = memp->virtRefDTypep()->prettyDTypeName(உண்மை);
        மாறிலி auto it = dtypemap.find(memp_DType);
        எனில் (it != dtypemap.end()) {
            // default_value for data_type
            திரும்பு it->second->clone_மரம்(பொய்);
        }
        எனில் (defaultp) {
            // default_value for any unmatched member yet
            திரும்பு defaultp->clone_மரம்(பொய்);
        }
        எனில் (memp->valuep()) {
            திரும்பு புதிய AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_மரம்(பொய்), முனை_கோவை),
                                    புதிய AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        திரும்பு nullptr;
    }

    Ast_முனை_கோவை* மதிப்பு_Concat_pattern_Struct(AstPatMember* patp, Ast_முனை_கோவை* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        எனில் (patp) {
            patp->dtypep(memp);
            Ast_முனை_கோவை* மாறிலி valuep = pattern_Member_மதிப்பு_Iterate(patp);
            எனில் (!newp) {
                newp = valuep;
            } இல்லையெனில் {
                AstConcat* மாறிலி concatp = புதிய AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, உண்மை);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, உண்மை);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        திரும்பு newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_முனை_DType* elemDTypep,
                                       மாறிலி DTypeMap& dtypemap = DTypeMap{}) {
        Ast_முனை_கோவை* மாறிலி valuep = defaultp->lhssp()->clone_மரம்(பொய்);
        Ast_முனை_DType* மாறிலி elemDTypeSkipRefp = elemDTypep->skipRefp();
        மாறிலி AstStructDType* மாறிலி structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        மாறிலி தருக்க unpackedStruct = structp && !structp->packed();

        எனில் (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
            திரும்பு defaultp->clone_மரம்(பொய்);
        }
        எனில் (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
            திரும்பு defaultp->clone_மரம்(பொய்);
        }
        எனில் (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        தருக்க wholeElement = unpackedStruct;
        எனில் (Ast_முனை_DType* மாறிலி மதிப்பு_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !மதிப்பு_DTypep->skipRefp()->isIntegralOrPacked()
                                          : மதிப்பு_DTypep->similarDType(elemDTypeSkipRefp);
        }
        எனில் (wholeElement) {
            VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
            திரும்பு defaultp->clone_மரம்(பொய்);
        }

        AstPatMember* மாறிலி nestedDefaultp
            = புதிய AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(உண்மை);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        க்கு (மாறிலி auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_மரம்(பொய்));
        AstPattern* மாறிலி recursivePatternp = புதிய AstPattern{defaultp->fileline(), nestedDefaultp};
        திரும்பு புதிய AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_முனை_கோவை* patternDefaultValuep(AstPatMember* defaultp, Ast_முனை_DType* elemDTypep) {
        எனில் (!defaultp) திரும்பு nullptr;
        AstPatMember* மாறிலி elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_முனை_கோவை* மாறிலி valuep = pattern_Member_மதிப்பு_Iterate(elemDefaultp);
        VL_DO_DANGLING(தள்ளு_Deletep(elemDefaultp), elemDefaultp);
        திரும்பு valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    வெறுமை patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        மாறிலி VNumRange range = arrayDtp->declRange();
        மாறிலி முழு elements = range.elements();
        AstConsArray* மாறிலி newp = புதிய AstConsArray{nodep->fileline(), arrayDtp};
        முழு entn = 0;  // Number of elements given so far, from the left
        தருக்க error = பொய்;  // Reported an error, so the value is not complete
        க்கு (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_முனை_கோவை* மாறிலி valuep = pattern_Member_மதிப்பு_Iterate(patp);
            மாறிலி uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            மாறிலி AstUnpackArrayDType* மாறிலி spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            எனில் (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_மதிப்புகள்(index, valuep);
                தொடர்;
            }
            மாறிலி VCastable castable = Ast_முனை::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            எனில் (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = உண்மை;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        எனில் (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = உண்மை;
        }
        எனில் (error) {
            VL_DO_DANGLING(தள்ளு_Deletep(newp), newp);
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } இல்லையெனில் {
            nodep->replaceWith(newp);
        }
    }
    வெறுமை patternArray(AstPattern* nodep, Ast_முனை_Array_DType* arrayDtp, AstPatMember* defaultp) {
        எனில் (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            திரும்பு;
        }
        மாறிலி VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.இடது() << " to " << range.வலது());
        Ast_முனை* newp = nullptr;
        தருக்க missed = பொய்;  // Some element missed, so not a complete value
        க்கு (முழு entn = 0, ent = range.இடது(); entn < range.elements();
             ++entn, ent += range.இடது_To_வலது_Inc()) {
            Ast_முனை_கோவை* valuep = nullptr;
            மாறிலி auto it = patmap.find(ent);
            எனில் (it == patmap.end()) {
                எனில் (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } இல்லையெனில் {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = உண்மை;
                }
            } இல்லையெனில் {
                AstPatMember* மாறிலி patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_மதிப்பு_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                மாறிலி AstUnpackArrayDType* மாறிலி memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                எனில் (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
                    valuep = புதிய AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            எனில் (valuep) {
                எனில் (VN_IS(arrayDtp, UnpackArrayDType)) {
                    எனில் (!newp) newp = புதிய AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } இல்லையெனில் {  // Packed. Convert to concat for now.
                    எனில் (!newp) {
                        newp = valuep;
                    } இல்லையெனில் {
                        AstConcat* மாறிலி concatp
                            = புதிய AstConcat{valuep->fileline(), VN_AS(newp, முனை_கோவை), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        எனில் (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        எனில் (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(தள்ளு_Deletep(newp), newp);
            newp = புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        எனில் (newp) {
            nodep->replaceWith(newp);
        } இல்லையெனில் {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        எனில் (VN_IS(patp->keyp(), முனை_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            திரும்பு nullptr;
        }
        எனில் (patp->சாவி_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->சாவி_Exprp());  // May change
        AstConst* மாறிலி constp = VN_CAST(patp->சாவி_Exprp(), Const);
        எனில் (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        திரும்பு constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_முனை_DType* சாவி_DTypep) {
        எனில் (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            திரும்பு nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        எனில் (சாவி_DTypep->skipRefp()->isIntegralOrPacked() && patp->சாவி_Exprp()) {
            userIterate(patp->சாவி_Exprp(), WidthVP{சாவி_DTypep, PRELIM}.p());
            மாறிலி முழு width = std::max(patp->சாவி_Exprp()->width(), சாவி_DTypep->width());
            Ast_முனை_DType* மாறிலி contextDTypep
                = சாவி_DTypep->findBitOrLogicDType(width, width, சாவி_DTypep->numeric(), உண்மை);
            iterateCheck(patp, "key", patp->சாவி_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, பொய்);
        }
        AstConst* மாறிலி constp = patternConstKeyp(patp);
        எனில் (!constp) திரும்பு nullptr;
        எனில் (constp->இலக்கம்().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            திரும்பு nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, சாவி_DTypep, EXTEND_EXP, பொய்);
        Ast_முனை_கோவை* மாறிலி convp = patp->சாவி_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_முனை_கோவை* மாறிலி keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        திரும்பு VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_முனை_கோவை* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_முனை_DType* arrayDtp) {
        Ast_முனை_DType* மாறிலி சாவி_DTypep = VN_AS(arrayDtp, AssocArrayDType)->சாவி_DTypep();
        எனில் (!சாவி_DTypep->skipRefp()->isAggregateType() || !patp->சாவி_Exprp()) {
            திரும்பு patternScalarKeyp(patp, சாவி_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->சாவி_Exprp(), சாவி_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->சாவி_Exprp());
        Ast_முனை_கோவை* மாறிலி foldedp = patp->சாவி_Exprp();
        எனில் (!foldedp->is_Constant_மரம்()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            திரும்பு nullptr;
        }
        எனில் (foldedp->exists([](மாறிலி AstConst* constp) { திரும்பு constp->இலக்கம்().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            திரும்பு nullptr;
        }
        திரும்பு foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_முனை_DType* arrayDtp) {
        திரும்பு patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_முனை_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        திரும்பு patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    வெறுமை sort_Cons_பொருட்கள்(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        போது (T_Item* மாறிலி itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.தள்ளு_back(itemp);
        }
        மாறிலி auto சாவி_Of = [](மாறிலி T_Item* itemp) -> மாறிலி V3_எண்& {
            மாறிலி Ast_முனை* மாறிலி keyp = itemp->keyp();
            திரும்பு VN_AS(keyp, Const)->இலக்கம்();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](மாறிலி T_Item* ap, மாறிலி T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            திரும்பு சாவி_Of(ap).isLtXZ(சாவி_Of(bp));
        });
        மாறிலி T_Item* firstp = nullptr;  // First item with the current key
        க்கு (T_Item* மாறிலி itemp : itemps) {
            எனில் (firstp && சாவி_Of(firstp).isCaseEq(சாவி_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } இல்லையெனில் {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    வெறுமை patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_முனை_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        க்கு (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            மாறிலி auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_முனை_கோவை* மாறிலி valuep = pattern_Member_மதிப்பு_Iterate(patp);
            எனில் (keyp) {
                consp->addItemsp(புதிய T_Item{patp->fileline(), keyp, valuep});
            } இல்லையெனில் {
                VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        மாறிலி AstAssocArrayDType* மாறிலி assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        எனில் (!assocDtp || !assocDtp->சாவி_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_பொருட்கள்<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_முனை_கோவை* newConsDynArrayOrQueue(File_வரி* fileline, மாறிலி Ast_முனை_DType* dtypep) {
        எனில் (VN_IS(dtypep, DynArrayDType)) திரும்பு புதிய AstConsDynArray{fileline};
        எனில் (VN_IS(dtypep, QueueDType)) திரும்பு புதிய AstConsQueue{fileline};
        திரும்பு nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    வெறுமை addConsItem(T_Cons* consp, Ast_முனை_கோவை* valuep) {
        Ast_முனை_DType* மாறிலி arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        மாறிலி தருக்க isElement
            = Ast_முனை::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        மாறிலி AstQueueDType* மாறிலி subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        எனில் (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* மாறிலி subp = Ast_முனை::cast<T_Cons>(valuep);
            எனில் (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                எனில் (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(தள்ளு_Deletep(subp), subp);
                திரும்பு;
            }
            எனில் (AstConsQueue* மாறிலி queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_பொருட்கள்(consp, queuep);
                திரும்பு;
            }
            எனில் (AstConsDynArray* மாறிலி dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_பொருட்கள்(consp, dynp);
                திரும்பு;
            }
        }
        T_Item* மாறிலி itemp = புதிய T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        எனில் (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } இல்லையெனில் {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    வெறுமை addConsItem(Ast_முனை_கோவை* consp, Ast_முனை_கோவை* valuep) {
        எனில் (AstConsDynArray* மாறிலி dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } இல்லையெனில் {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    வெறுமை add_Cons_பொருட்கள்(Ast_முனை_கோவை* consp, T_Sub* subp) {
        போது (auto* மாறிலி itemp = subp->itemsp()) {
            Ast_முனை_கோவை* மாறிலி valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(தள்ளு_Deletep(subp), subp);
    }
    வெறுமை patternDynArrayOrQueue(AstPattern* nodep, Ast_முனை_DType* arrayp, AstPatMember* defaultp) {
        Ast_முனை_கோவை* மாறிலி newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        மாறிலி AstPatMember* மாறிலி firstp = VN_AS(nodep->itemsp(), PatMember);
        மாறிலி தருக்க keyed = defaultp || (firstp && firstp->keyp());
        எனில் (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            திரும்பு;
        }
        மாறிலி எழுத்து* மாறிலி kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        எனில் (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        க்கு (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            எனில் (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_மதிப்பு_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    வெறுமை patternBasic(AstPattern* nodep, Ast_முனை_DType* vdtypep, AstPatMember* defaultp) {
        மாறிலி AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        மாறிலி VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_முனை_கோவை* newp = nullptr;
        க்கு (மாறிலி முழு ent : range.seqHiToLo()) {
            Ast_முனை_கோவை* valuep = nullptr;
            மாறிலி auto it = patmap.find(ent);
            எனில் (it == patmap.end()) {
                எனில் (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } இல்லையெனில் {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } இல்லையெனில் {
                AstPatMember* மாறிலி patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_மதிப்பு_Iterate(patp);
            }
            எனில் (valuep) {  // Packed. Convert to concat for now.
                எனில் (!newp) {
                    newp = valuep;
                } இல்லையெனில் {
                    AstConcat* மாறிலி concatp = புதிய AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        எனில் (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        எனில் (newp) {
            nodep->replaceWith(newp);
        } இல்லையெனில் {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_முனை_கோவை* pattern_Member_மதிப்பு_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_முனை_கோவை* valuep = patp->lhssp()->unlinkFrBack();
        எனில் (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            எனில் (AstConst* மாறிலி newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        திரும்பு valuep;
    }

    நிலையான வெறுமை checkEventAssignment(மாறிலி Ast_முனை_Assign* மாறிலி asgnp) {
        string unsupEvtAsgn;
        எனில் (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        எனில் (VN_IS(asgnp->rhsp(), CReset)) திரும்பு;
        எனில் (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        எனில் (!unsupEvtAsgn.empty()) {
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

    நிலையான தருக்க usesDynamicScheduler(Ast_முனை* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        போது (உண்மை) {
            AstVarRef* மாறிலி vrefp = VN_CAST(nodep, VarRef);
            எனில் (vrefp) திரும்பு usesDynamicScheduler(vrefp);
            எனில் (VN_IS(nodep, MemberSel)) {
                திரும்பு உண்மை;
            } இல்லையெனில் எனில் (Ast_முனை_Sel* selp = VN_CAST(nodep, முனை_Sel)) {
                nodep = selp->fromp();
            } இல்லையெனில் {
                திரும்பு பொய்;
            }
        }
    }

    நிலையான தருக்க usesDynamicScheduler(AstVarRef* vrefp) {
        திரும்பு VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    வெறுமை visit(AstPatMember* nodep) override {
        Ast_முனை_DType* மாறிலி vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    முழு visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        எனில் (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->repp(), Const);
            எனில் (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } இல்லையெனில் {
                times = constp->toUInt();
            }
            எனில் (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_மரம்();  // Done with replicate before cloning
        }
        திரும்பு times;
    }

    வெறுமை visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_முனை* propp = nodep->propp();
            போது (AstLogNot* மாறிலி notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* மாறிலி untilp = VN_CAST(propp, Until);
            எனில் (untilp && VN_IS(nodep->backp(), முனை_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(புதிய AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(தள்ளு_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            எனில் (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    தருக்க firstNewStatementOkRecurse(Ast_முனை* nodep) {
        எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
            எனில் (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) திரும்பு உண்மை;
        }
        எனில் (AstAssign* மாறிலி aitemp = VN_CAST(nodep, Assign)) {
            எனில் (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) திரும்பு உண்மை;
        }
        எனில் (AstInitialStaticStmt* மாறிலி aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            க்கு (Ast_முனை* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                எனில் (!firstNewStatementOkRecurse(stmtp)) திரும்பு பொய்;
            }
            திரும்பு உண்மை;
        }
        எனில் (AstInitialAutomaticStmt* மாறிலி aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            க்கு (Ast_முனை* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                எனில் (!firstNewStatementOkRecurse(stmtp)) திரும்பு பொய்;
            }
            திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    வெறுமை handleCaseType(Ast_முனை* casep, Ast_முனை_கோவை* exprp, CaseItem* itemsp) {
        AstAttrOf* மாறிலி exprap = VN_CAST(exprp, AttrOf);
        எனில் (!exprap) திரும்பு;
        எனில் (exprap->attrType() != VAttrType::TYPEID) திரும்பு;

        மாறிலி Ast_முனை_DType* மாறிலி கோவை_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << கோவை_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_வரி* மாறிலி newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, உண்மை);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, உண்மை);  // Side effect of transform
        casep->fileline(newfl);
        க்கு (CaseItem* itemp = itemsp; itemp; itemp = Ast_முனை::as<CaseItem>(itemp->nextp())) {
            எனில் (itemp->isDefault()) தொடர்;
            தருக்க hit = பொய்;
            க்கு (Ast_முனை* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                மாறிலி AstAttrOf* மாறிலி condAttrp = VN_CAST(condp, AttrOf);
                எனில் (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } இல்லையெனில் {
                    Ast_முனை_DType* மாறிலி condDtp = condAttrp->dtypep();
                    எனில் (Ast_முனை::computeCastable(கோவை_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = உண்மை;
                        நிறுத்து;
                    }
                }
            }
            தள்ளு_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(புதிய AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(புதிய AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(தள்ளு_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    வெறுமை handleCase(Ast_முனை* casep, Ast_முனை_கோவை* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_முனை_DType* subDTypep = exprp->dtypep();
        க்கு (CaseItem* itemp = itemsp; itemp; itemp = Ast_முனை::as<CaseItem>(itemp->nextp())) {
            க்கு (Ast_முனை* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                எனில் (condp->dtypep() == subDTypep) தொடர்;

                எனில் (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } இல்லையெனில் எனில் (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } இல்லையெனில் {
                    மாறிலி முழு width = std::max(subDTypep->width(), condp->width());
                    மாறிலி முழு mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    மாறிலி தருக்க issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        க்கு (CaseItem* itemp = itemsp; itemp; itemp = Ast_முனை::as<CaseItem>(itemp->nextp())) {
            க்கு (Ast_முனை *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    வெறுமை visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        க்கு (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            க்கு (Ast_முனை *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    வெறுமை visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    வெறுமை visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    வெறுமை visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        க்கு (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            க்கு (Ast_முனை *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    வெறுமை visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_முனை_DType* மாறிலி itemDTypep = nodep->findUInt32DType();
        க்கு (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            க்கு (Ast_முனை *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    வெறுமை visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    வெறுமை visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    வெறுமை visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    வெறுமை visit(Ast_முனை_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    வெறுமை visit(Ast_முனை_Foreach* nodep) override {
        எனில் (nodep->didWidth()) திரும்பு;
        nodep->didWidth(உண்மை);
        மாறிலி AstForeachHeader* மாறிலி headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_முனை_கோவை* மாறிலி fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_முனை_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        க்கு (Ast_முனை *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            மாறிலி தருக்க empty = VN_IS(argsp, Empty);
            AstVar* மாறிலி varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            எனில் (varp) varp->usedLoopIdx(உண்மை);
            எனில் (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
                திரும்பு;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            எனில் (VN_IS(fromDtp, முனை_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } இல்லையெனில் எனில் (AstBasicDType* மாறிலி adtypep = VN_CAST(fromDtp, BasicDType)) {
                எனில் (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
                    திரும்பு;
                }
            } இல்லையெனில் எனில் (மாறிலி AstAssocArrayDType* மாறிலி adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->சாவி_DTypep());
            } இல்லையெனில் {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
                திரும்பு;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_முனை* மாறிலி bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        எனில் (AstForeach* மாறிலி loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            திரும்பு;
        }
    }

    வெறுமை visit(Ast_முனை_Assign* nodep) override {
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
            எனில் (மாறிலி AstIfaceRefDType* மாறிலி irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                எனில் (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                    திரும்பு;
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
            Ast_முனை_DType* மாறிலி lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            எனில் (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                மாறிலி Ast_முனை_DType* மாறிலி lhsDtp = lhsDTypep->skipRefp();
                மாறிலி Ast_முனை_DType* மாறிலி rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                மாறிலி தருக்க lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                மாறிலி தருக்க rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                எனில் (lhsIsUnpackArray && rhsIsUnpackArray) {
                    எனில் (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_முனை_Var_Ref, Ast_முனை_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), முனை_Var_Ref), VN_CAST(nodep->rhsp(), முனை_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                எனில் (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), முனை_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        எனில் (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        எனில் (Ast_முனை* மாறிலி controlp = nodep->timingControlp()) {
            எனில் (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_மரம்(), controlp);
            } இல்லையெனில் எனில் (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } இல்லையெனில் {
                எனில் (nodep->fileline()->timingOn()) {
                    எனில் (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } இல்லையெனில் {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_மரம்(), controlp);
            }
        }
        எனில் (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            மாறிலி Ast_முனை_DType* மாறிலி lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            எனில் (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
            AstMethodCall* மாறிலி newp
                = புதிய AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            திரும்பு;
        }
        எனில் (மாறிலி AstNewDynamic* மாறிலி dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            எனில் (!dynp->rhsp()) {
                newp = புதிய AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } இல்லையெனில் {
                newp = புதிய AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(உண்மை);
            newp->protect(பொய்);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        எனில் (AstCMethodHard* மாறிலி slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            எனில் (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } இல்லையெனில் எனில் (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } இல்லையெனில் எனில் (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } இல்லையெனில் {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            எனில் (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_முனை_கோவை* மாறிலி fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_முனை_கோவை* மாறிலி lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_முனை_கோவை* மாறிலி msbp = slicep->pinsp()->unlinkFrBack();
                Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* மாறிலி newp
                    = புதிய AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(உண்மை);
                newp->protect(பொய்);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            }
        }

        எனில் (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    வெறுமை visit(AstRSRule* nodep) override {
        எனில் (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    வெறுமை visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    வெறுமை visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    நிலையான தருக்க isFormatNonNumericArg(மாறிலி Ast_முனை_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        திரும்பு dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, முனை_UOr_Struct_DType)
                   && !VN_AS(dtypep, முனை_UOr_Struct_DType)->packed());
    }

    வெறுமை checkFormatNumericArg(Ast_முனை* argp, எழுத்து ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        எனில் (std::tolower(ch) == 'd') திரும்பு;
        // In contrast, %x of a class reference, causes errors on several simulators.
        எனில் (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    வெறுமை visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    வெறுமை visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    வெறுமை visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        எனில் (m_vup && !m_vup->prelim()) திரும்பு;  // Can be called as statement or function
        எனில் (nodep->didWidthAndSet()) திரும்பு;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        எனில் (!nodep->கோவை_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_முனை_கோவை* oldExprsp = nodep->exprsp();
        எனில் (nodep->கோவை_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), முனை_கோவை);
        }
        எனில் (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        போது (Ast_முனை_கோவை* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), முனை_கோவை);
            எனில் (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            எனில் (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                தொடர்;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_முனை_DType* மாறிலி dtypep = argp->dtypep()->skipRefp();
            எனில் (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } இல்லையெனில் எனில் (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } இல்லையெனில் எனில் (isFormatNonNumericArg(dtypep)) {
                எனில் (AstClassRefDType* மாறிலி classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    எனில் (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(உண்மை);
                    }
                } இல்லையெனில் {
                    // Class handles inside containers print type names, not their members.
                    Ast_முனை_DType* முனை_Dtypep = dtypep;
                    போது (முனை_Dtypep) {
                        முனை_Dtypep = முனை_Dtypep->skipRefp();
                        எனில் (Ast_முனை_UOr_Struct_DType* மாறிலி uOrStructDTypep
                            = VN_CAST(முனை_Dtypep, முனை_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(உண்மை);
                        }
                        முனை_Dtypep = முனை_Dtypep->subDTypep();
                    }
                }
                Ast_முனை_கோவை* மாறிலி newp = புதிய AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } இல்லையெனில் எனில் (nodep->கோவை_Format_2()) {
                எனில் (AstEnumDType* மாறிலி enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    தொடர்;
                }
            }
            எனில் (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            எனில் (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } இல்லையெனில் {
                nodep->addExprsp(புதிய AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    வெறுமை visit(AstCReset* nodep) override {
        assert_At_கோவை(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    வெறுமை visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    வெறுமை visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    வெறுமை visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        எனில் (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        எனில் (!m_paramsOnly) {
            nodep->foreach([இது](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    புதிய AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            எனில் (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            தேர்வு (nodep->displayType()) {
            நிலை VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); நிறுத்து;
            நிலை VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); நிறுத்து;
            நிலை VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); நிறுத்து;
            நிலை VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); நிறுத்து;
            இயல்பு: UASSERT_OBJ(பொய், nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        }
    }
    வெறுமை visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        எனில் (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    வெறுமை visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    வெறுமை visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    வெறுமை visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    வெறுமை visit(AstFError* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    வெறுமை visit(AstFEof* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    வெறுமை visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        எனில் (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    வெறுமை visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    வெறுமை visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    வெறுமை visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    வெறுமை visit(AstFGetC* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    வெறுமை visit(AstFGetS* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    வெறுமை visit(AstFUngetC* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    வெறுமை visit(AstFRead* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            எனில் (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            எனில் (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    வெறுமை visit(AstFScanF* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    வெறுமை visit(AstSScanF* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    வெறுமை visit(Ast_அடுக்கு_Trace_F* nodep) override { nodep->dtypeSetString(); }
    வெறுமை visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstSystemF* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(Ast_முனை_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        மாறிலி Ast_முனை_DType* subp = nullptr;
        எனில் (மாறிலி AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            எனில் (!adtypep->சாவி_DTypep()->skipRefp()->basicp()
                || !adtypep->சாவி_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } இல்லையெனில் எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } இல்லையெனில் எனில் (மாறிலி AstDynArrayDType* மாறிலி adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } இல்லையெனில் {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        எனில் (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstTestPlusArgs* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    வெறுமை visit(Ast_மதிப்பு_Plus_Args* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    வெறுமை visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        எனில் (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        எனில் (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        எனில் (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        எனில் (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    வெறுமை visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    வெறுமை visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    வெறுமை visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    வெறுமை visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    வெறுமை visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    தருக்க pinInstArrayElement(AstPin* nodep, மாறிலி AstRange* rangep, முழு idx) {
        Ast_முனை_DType* மாறிலி modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_முனை_DType* மாறிலி conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        எனில் (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) திரும்பு உண்மை;
        // Types don't match, but all dimensions done, still need the usual checks
        எனில் (!rangep) திரும்பு பொய்;

        மாறிலி AstRange* மாறிலி subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        முழு subElems = 1;
        க்கு (மாறிலி AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        மாறிலி முழு elements = rangep->elementsConst();
        மாறிலி முழு from_இடது = idx / subElems;

        மாறிலி AstUnpackArrayDType* மாறிலி arrp = VN_CAST(conDTypep, UnpackArrayDType);
        மாறிலி uint32_t conDims = conDTypep->dimensions(பொய்).second;
        மாறிலி uint32_t modDims = modDTypep->dimensions(பொய்).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        எனில் (arrp && conDims != modDims) {
            எனில் (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                திரும்பு உண்மை;
            }
            // ArraySel index is counted from lo
            மாறிலி முழு bit = arrp->declRange().ascending() ? from_இடது : elements - 1 - from_இடது;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_முனை_கோவை* மாறிலி fromp = VN_AS(nodep->exprp(), முனை_கோவை)->unlinkFrBack();
            எனில் (Ast_முனை_கோவை* மாறிலி elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(தள்ளு_Deletep(fromp), fromp);
            } இல்லையெனில் {
                AstArraySel* மாறிலி selp = புதிய AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(உண்மை);
                nodep->exprp(selp);
            }
            திரும்பு pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        எனில் (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) திரும்பு பொய்;

        // Packed connection
        மாறிலி முழு modwidth = modDTypep->width();
        மாறிலி முழு conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        எனில் (conwidth == modwidth) திரும்பு பொய்;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        எனில் (conwidth == modwidth * elements * subElems) {
            எனில் (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->இடது_Const() << ":"
                                                     << rangep->வலது_Const() << "]");
            }
            மாறிலி முழு selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_முனை_கோவை* மாறிலி fromp = VN_AS(nodep->exprp(), முனை_கோவை)->unlinkFrBack();
            AstSel* மாறிலி selp = புதிய AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_இடது), selwidth};
            selp->didWidth(உண்மை);
            nodep->exprp(selp);
            திரும்பு pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        திரும்பு உண்மை;
    }

    வெறுமை visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        எனில் (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* மாறிலி concatp = VN_CAST(nodep->exprp(), Replicate);
            மாறிலி AstConst* மாறிலி countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            மாறிலி Ast_முனை_DType* மாறிலி dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            எனில் (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* மாறிலி patternp = புதிய AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(தள்ளு_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } இல்லையெனில் எனில் (!m_paramsOnly) {
            எனில் (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            எனில் (!nodep->exprp()) {  // No-connect
                எனில் (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                திரும்பு;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            மாறிலி VDirection pinDirection = nodep->modVarp()->direction();
            எனில் (VN_IS(nodep->exprp(), முனை_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_முனை_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_முனை_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            எனில் (மாறிலி AstIfaceRefDType* மாறிலி modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                மாறிலி AstIfaceRefDType* மாறிலி conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                எனில் (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_முனை_DType* subDTypep = modDTypep;
            மாறிலி முழு modwidth = modDTypep->width();
            முழு conwidth = conDTypep->width();
            எனில் (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } இல்லையெனில் {
                எனில் (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    எனில் (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        எனில் (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        திரும்பு;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                எனில் (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } இல்லையெனில் எனில் (nodep->modVarp()->isTristate()) {
                    எனில் (modwidth != conwidth) {
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
                } இல்லையெனில் எனில் (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                எனில் ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                மாறிலி Ast_முனை_Array_DType* மாறிலி கோவை_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                மாறிலி Ast_முனை_Array_DType* மாறிலி modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                எனில் (கோவை_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } இல்லையெனில் எனில் ((கோவை_Arrayp && !modArrayp) || (!கோவை_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (கோவை_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } இல்லையெனில் {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_முனை_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), முனை_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            எனில் (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    வெறுமை visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        எனில் (!m_paramsOnly) {
            எனில் (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_செய்தி(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            எனில் (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            எனில் (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                தள்ளு_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    வெறுமை visit(AstGatePin* nodep) override {
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            மாறிலி Ast_முனை_DType* மாறிலி pinDTypep = nodep->exprp()->dtypep();
            மாறிலி முழு இலக்கம்_Insts = nodep->rangep()->elementsConst();
            மாறிலி முழு modwidth = இலக்கம்_Insts;
            மாறிலி முழு pinwidth = pinDTypep->width();
            எனில் (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_முனை_DType* மாறிலி subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_முனை* மாறிலி newp
                    = புதிய AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(இலக்கம்_Insts)};
                nodep->replaceWith(newp);
            } இல்லையெனில் {
                எனில் (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
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
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        எனில் (nodep->didWidth()) திரும்பு;
        எனில் (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(உண்மை);
            nodep->didWidth(உண்மை);
            திரும்பு;
        }
        எனில் (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } இல்லையெனில் எனில் (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        எனில் (nodep->classMethod() && nodep->name() == "new") {
            எனில் (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            எனில் (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_முனை* firstp = nullptr;
            க்கு (Ast_முனை* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                எனில் (Ast_Stmt_கோவை* மாறிலி sep = VN_CAST(itemp, Stmt_கோவை)) {
                    எனில் (AstNew* மாறிலி newp = VN_CAST(sep->exprp(), New)) {
                        எனில் (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            நிறுத்து;
                        }
                    }
                    தொடர்;
                }
                எனில் (firstNewStatementOkRecurse(itemp)) தொடர்;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(உண்மை);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        எனில் (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        எனில் (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } இல்லையெனில் எனில் (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } இல்லையெனில் எனில் (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        எனில் (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(உண்மை);
        nodep->doingWidth(பொய்);
        எனில் (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        எனில் (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](மாறிலி AstVarRef* varrefp) {
                   திரும்பு varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                எனில் (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } இல்லையெனில் எனில் (AstAttrOf* மாறிலி attrp = VN_CAST(stmtp, AttrOf)) {
                எனில் (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                எனில் (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        எனில் (protop) {
            க்கு (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                எனில் (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    நிறுத்து;
                } இல்லையெனில் {
                    AstVar* மாறிலி portp = ports[i];
                    AstAttrOf* மாறிலி rprotop = protos[i];
                    Ast_முனை_DType* மாறிலி declDtp = portp->dtypep();
                    Ast_முனை_DType* மாறிலி protoDtp = rprotop->fromp()->dtypep();
                    எனில் (portp->name() != rprotop->name()) {
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
                    } இல்லையெனில் எனில் (!similarDTypeRecurse(protoDtp, declDtp)) {
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
    வெறுமை visit(AstConstraint* nodep) override {
        எனில் (nodep->didWidth()) திரும்பு;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(உண்மை);
    }
    வெறுமை visit(AstProperty* nodep) override {
        எனில் (nodep->didWidth()) திரும்பு;
        எனில் (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(உண்மை);
            nodep->didWidth(உண்மை);
            திரும்பு;
        }
        nodep->doingWidth(உண்மை);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        க்கு (Ast_முனை* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            எனில் (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } இல்லையெனில் எனில் (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } இல்லையெனில் எனில் (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } இல்லையெனில் {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(உண்மை);
        nodep->doingWidth(பொய்);
    }
    வெறுமை visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    வெறுமை visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        எனில் (nodep->didWidth()) திரும்பு;
        nodep->dtypeSetBit();
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } இல்லையெனில் எனில் (VN_IS(stmtp, முனை_கோவை)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } இல்லையெனில் {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(உண்மை);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_முனை* pkgItemp) {
        போது (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        திரும்பு VN_CAST(pkgItemp->backp(), Package);
    }
    வெறுமை visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_முனை_FTask_Ref*>(nodep));
        எனில் (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            எனில் (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_கோவை))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } இல்லையெனில் {
            nodep->dtypeFrom(nodep->taskp());
        }
        எனில் (nodep->fileline()->timingOn()) {
            Ast_முனை_Module* மாறிலி classp = nodep->classOrPackagep();
            எனில் (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* மாறிலி packagep = getItemPackage(classp);
                எனில் (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    நிலையான தருக்க areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        மாறிலி std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        மாறிலி std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        எனில் (dims0.size() != dims1.size()) திரும்பு பொய்;
        க்கு (size_t i = 0; i < dims0.size(); ++i) {
            எனில் (dims0[i]->elementsConst() != dims1[i]->elementsConst()) திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }
    // Makes sure that port and pin have same size and same datatype
    வெறுமை checkUnpackedArrayArgs(AstVar* portp, Ast_முனை* pinp) {
        எனில் (AstUnpackArrayDType* மாறிலி portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            எனில் (AstUnpackArrayDType* மாறிலி pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                எனில் (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                மாறிலி AstBasicDType* மாறிலி portBasicp = portDtypep->basicp();
                மாறிலி AstBasicDType* மாறிலி pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                மாறிலி தருக்க sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                எனில் (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } இல்லையெனில் {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    வெறுமை processFTaskRefArgs(Ast_முனை_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        எனில் (nodep->didWidth()) திரும்பு;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        செய் {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            மாறிலி V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            எனில் (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            க்கு (மாறிலி auto& tconnect : tconnects) {
                மாறிலி AstVar* மாறிலி portp = tconnect.first;
                AstArg* மாறிலி argp = tconnect.second;
                Ast_முனை_கோவை* pinp = argp->exprp();
                எனில் (!pinp) தொடர்;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                எனில் (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_முனை_கோவை* argsp = nullptr;
                    போது (AstArg* மாறிலி nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_மரம்();  // Remove the call's Arg wrapper
                    }
                    string format;
                    எனில் (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->இலக்கம்().toString();
                    } இல்லையெனில் {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(தள்ளு_Deletep(argp), argp);
                    AstSFormatF* மாறிலி newp
                        = புதிய AstSFormatF{nodep->fileline(), format, பொய், argsp};
                    எனில் (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(புதிய AstScopeName{newp->fileline(), உண்மை});
                    }
                    handle.relink(புதிய AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    செல் reloop;  // so exit early; next loop will correct it
                }  //
                இல்லையெனில் எனில் (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_முனை_கோவை* மாறிலி newp = புதிய AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                எனில் (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } போது (பொய்);
        // Stage 2
        {
            மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            க்கு (மாறிலி auto& tconnect : tconnects) {
                AstVar* மாறிலி portp = tconnect.first;
                மாறிலி AstArg* மாறிலி argp = tconnect.second;
                Ast_முனை_கோவை* மாறிலி pinp = argp->exprp();
                எனில் (!pinp) தொடர்;  // Argument error we'll find later
                // Change data types based on above accept completion
                எனில் (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                எனில் (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            க்கு (மாறிலி auto& tconnect : tconnects) {
                மாறிலி AstVar* மாறிலி portp = tconnect.first;
                மாறிலி AstArg* மாறிலி argp = tconnect.second;
                Ast_முனை_கோவை* மாறிலி pinp = argp->exprp();
                எனில் (!pinp) தொடர்;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        எனில் (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            க்கு (மாறிலி auto& tconnect : tconnects) {
                மாறிலி AstVar* மாறிலி portp = tconnect.first;
                மாறிலி AstArg* மாறிலி argp = tconnect.second;
                Ast_முனை_கோவை* pinp = argp->exprp();
                எனில் (!pinp) தொடர்;  // Argument error we'll find later
                Ast_முனை_DType* மாறிலி portDTypep = portp->dtypep()->skipRefToEnump();
                எனில் (VN_IS(pinp, EmptyQueue)) {
                    Ast_முனை_கோவை* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    எனில் (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = புதிய AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(pinp), pinp);
                    pinp = newp;
                }
                மாறிலி Ast_முனை_DType* மாறிலி pinDTypep = pinp->dtypep()->skipRefToEnump();
                மாறிலி AstIfaceRefDType* மாறிலி portIfacep
                    = VN_CAST(portDTypep->elemDTypep(உண்மை), IfaceRefDType);
                மாறிலி VCastable ifaceCastable
                    = portIfacep ? Ast_முனை::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ உண்மை)
                                 : VCastable{VCastable::UNSUPPORTED};
                மாறிலி தருக்க matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                மாறிலி தருக்க pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->இலக்கம்().isNull();
                எனில் ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    எனில் (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } இல்லையெனில் {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } இல்லையெனில் எனில் (portIfacep && portIfacep->isVirtual() && portp->is_உள்ளீடு() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } இல்லையெனில் எனில் (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_முனை_கோவை* மாறிலி newp = புதிய AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                எனில் (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    தொடர்;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                எனில் (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } இல்லையெனில் எனில் (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } இல்லையெனில் {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    வெறுமை handleStdRandomizeArgs(Ast_முனை_FTask_Ref* மாறிலி nodep) {
        AstConst* nullp = nullptr;
        க்கு (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_முனை_கோவை* மாறிலி exprp = argp->exprp();
            எனில் (AstConst* மாறிலி constp = VN_CAST(exprp, Const)) {
                எனில் (constp->இலக்கம்().isNull()) {
                    nullp = constp;
                    தொடர்;
                }
            }
            எனில் (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                தொடர்;
            }
            எனில் (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                தொடர்;
            }
            எனில் (மாறிலி AstCMethodHard* மாறிலி methodp = VN_CAST(exprp, CMethodHard)) {
                எனில் (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    தொடர்;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        எனில் (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        எனில் (nodep->didWidth()) திரும்பு;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        எனில் (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(உண்மை);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            திரும்பு;  // Handled in V3Randomize
        } இல்லையெனில் எனில் (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* மாறிலி classp = VN_CAST(nodep->classOrPackagep(), Class);
            எனில் (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(உண்மை);
                Ast_முனை_DType* மாறிலி adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, பொய், பொய், adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                க்கு (மாறிலி AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                எனில் (withp) nodep->withp(withp);
                nodep->didWidth(உண்மை);
                திரும்பு;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            எனில் (nodep->name() == "randomize") {
                AstClassRefDType* மாறிலி adtypep
                    = புதிய AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, பொய், பொய், adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                க்கு (மாறிலி AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } இல்லையெனில் எனில் (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } இல்லையெனில் எனில் (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(உண்மை);
                v3Global.useRandomizeMethods(உண்மை);
                AstCExpr* மாறிலி newp
                    = புதிய AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            } இல்லையெனில் எனில் (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_முனை_கோவை* மாறிலி expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_முனை_கோவை* மாறிலி exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(உண்மை);
                v3Global.useRandomizeMethods(உண்மை);
                AstCExpr* மாறிலி newp
                    = புதிய AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            } இல்லையெனில் {
                UASSERT_OBJ(பொய், nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        எனில் (nodep->didWidth()) {
            எனில் (withp) nodep->withp(withp);
            திரும்பு;
        }
        எனில் ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            தருக்க allow = பொய்;
            எனில் (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                எனில் (மாறிலி AstFuncRef* மாறிலி funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } இல்லையெனில் எனில் (மாறிலி AstTaskRef* மாறிலி taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                எனில் (!allow) {
                    மாறிலி AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    எனில் (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    மாறிலி AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    எனில் (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            எனில் (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                மாறிலி AstClass* மாறிலி cgClassp = m_containingClassFinder.find(nodep);
                எனில் (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    மாறிலி AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    எனில் (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            எனில் (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        எனில் (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(புதிய AstScopeName{nodep->fileline(), பொய்});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        எனில் (withp) nodep->withp(withp);
        nodep->didWidth(உண்மை);
        // See steps that follow in visit(AstFuncRef*)
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    வெறுமை visit(AstSenItem* nodep) override {
        எனில் (nodep->isComboStar()) திரும்பு;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        எனில் (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* மாறிலி concatOrReplp = VN_CAST(nodep->sensp(), முனை_Biop);
            auto* மாறிலி rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            எனில் (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* மாறிலி lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(புதிய AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(புதிய AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            எனில் (nodep->edgeType().anEdge()) {
                Ast_முனை_DType* மாறிலி sensDtp = nodep->sensp()->dtypep()->skipRefp();
                எனில் (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } இல்லையெனில் எனில் (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    வெறுமை visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* மாறிலி refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    வெறுமை visit(AstWait* nodep) override {
        எனில் (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }
        எனில் (nodep->fileline()->timingOn()) {
            எனில் (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                எனில் (Ast_முனை_Var_Ref* மாறிலி varrefp = VN_CAST(nodep->condp(), முனை_Var_Ref)) {
                    எனில் (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                திரும்பு;
            } இல்லையெனில் எனில் (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } இல்லையெனில் {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        எனில் (Ast_முனை* மாறிலி stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_முனை_DType* மாறிலி vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->மதிப்பு_Arg_Refp(), nullptr);
        எனில் (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } இல்லையெனில் {
            எனில் (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } இல்லையெனில் {
                எனில் (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } இல்லையெனில் {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            எனில் (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } இல்லையெனில் {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    வெறுமை visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        எனில் (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } இல்லையெனில் {
            nodep->dtypeFrom(m_withp->மதிப்பு_Arg_Refp());
        }
    }
    வெறுமை visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    வெறுமை visit(Ast_முனை_கோவை* nodep) override {
        எனில் (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    வெறுமை visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        எனில் (nodep->didWidthAndSet()) திரும்பு;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        எனில் (Ast_முனை_DType* மாறிலி vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } இல்லையெனில் எனில் (!nodep->dtypep()) {
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
        nodep->didWidth(உண்மை);
    }
    வெறுமை visitClass(AstClass* nodep) {
        எனில் (nodep->didWidthAndSet()) திரும்பு;

        // If the class is std::process
        எனில் (nodep->name() == "process") {
            AstPackage* மாறிலி packagep = getItemPackage(nodep);
            எனில் (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                எனில் (AstVar* மாறிலி varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_முனை_DType* மாறிலி dtypep = varp->getChildDTypep();
                    எனில் (!varp->dtypep()) {
                        VL_DO_DANGLING(தள்ளு_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* மாறிலி newdtypep = புதிய AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                எனில் (Ast_முனை_FTask* மாறிலி ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), முனை_FTask)) {
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
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        எனில் (AstClass* மாறிலி classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } இல்லையெனில் {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([இது](மாறிலி AstVar* varp) { m_curModVars.செருகு(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    வெறுமை visit_Or_Lu64(Ast_முனை_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_முனை_DType* மாறிலி subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    வெறுமை visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            எனில் (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    புதிய AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            }
        }
    }
    வெறுமை visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    வெறுமை visit_Os32_Lr(Ast_முனை_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    வெறுமை visit_Ou64_Lr(Ast_முனை_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    வெறுமை visit_log_not(AstLogNot* nodep) {
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
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            எனில் (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* மாறிலி newp
                    = புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        }
    }
    வெறுமை visit_log_and_or(Ast_முனை_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_முனை_கோவை::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    வெறுமை visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit_red_and_or(Ast_முனை_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    வெறுமை visit_red_unknown(Ast_முனை_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    தருக்க isEquivalentDType(மாறிலி Ast_முனை_DType* lhs, மாறிலி Ast_முனை_DType* rhs) {
        // a) If two types match, they are equivalent.
        எனில் (!lhs || !rhs) திரும்பு பொய்;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        எனில் (lhs == rhs) திரும்பு உண்மை;
        // If both are basic types, check if they are the same type
        எனில் (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            மாறிலி auto* lb = VN_CAST(lhs, BasicDType);
            மாறிலி auto* rb = VN_CAST(rhs, BasicDType);
            எனில் (lb->isString() != rb->isString()) திரும்பு பொய்;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        மாறிலி தருக்க lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        மாறிலி தருக்க rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        எனில் (lhsIsUnpackArray || rhsIsUnpackArray) {
            எனில் (lhsIsUnpackArray && rhsIsUnpackArray) {
                மாறிலி AstUnpackArrayDType* மாறிலி lhsp = VN_CAST(lhs, UnpackArrayDType);
                மாறிலி AstUnpackArrayDType* மாறிலி rhsp = VN_CAST(rhs, UnpackArrayDType);
                மாறிலி முழு lsz = lhsp->elementsConst();
                மாறிலி முழு rsz = rhsp->elementsConst();
                எனில் (lsz >= 0 && rsz >= 0 && lsz != rsz) திரும்பு பொய்;
                திரும்பு isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            திரும்பு பொய்;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        மாறிலி தருக்க lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        மாறிலி தருக்க rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        மாறிலி தருக்க lhsIsQueue = VN_IS(lhs, QueueDType);
        மாறிலி தருக்க rhsIsQueue = VN_IS(rhs, QueueDType);
        மாறிலி தருக்க lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        மாறிலி தருக்க rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        எனில் (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            எனில் (மாறிலி AstDynArrayDType* மாறிலி lhsp = VN_CAST(lhs, DynArrayDType)) {
                எனில் (மாறிலி AstDynArrayDType* மாறிலி rhsp = VN_CAST(rhs, DynArrayDType)) {
                    திரும்பு isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            எனில் (மாறிலி AstQueueDType* மாறிலி lhsp = VN_CAST(lhs, QueueDType)) {
                எனில் (மாறிலி AstQueueDType* மாறிலி rhsp = VN_CAST(rhs, QueueDType)) {
                    திரும்பு isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            எனில் (மாறிலி AstAssocArrayDType* மாறிலி lhsp = VN_CAST(lhs, AssocArrayDType)) {
                எனில் (மாறிலி AstAssocArrayDType* மாறிலி rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    திரும்பு isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->சாவி_DTypep(), rhsp->சாவி_DTypep());
                }
            }

            திரும்பு பொய்;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        எனில் (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            எனில் (lhs->width() != rhs->width()) திரும்பு பொய்;
            எனில் (lhs->isFourstate() != rhs->isFourstate()) திரும்பு பொய்;
            எனில் (lhs->isSigned() != rhs->isSigned()) திரும்பு பொய்;
            திரும்பு உண்மை;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        எனில் (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) திரும்பு பொய்;
        திரும்பு !(VN_IS(lhs, முனை_UOr_Struct_DType) || VN_IS(rhs, முனை_UOr_Struct_DType));
    }

    நிலையான தருக்க isAggregateType(மாறிலி Ast_முனை* nodep) {
        எனில் (!nodep) திரும்பு பொய்;
        மாறிலி Ast_முனை_DType* dtypep = nodep->dtypep();
        எனில் (!dtypep) திரும்பு பொய்;
        dtypep = dtypep->skipRefp();
        எனில் (!dtypep) திரும்பு பொய்;
        திரும்பு dtypep->isAggregateType();
    }

    வெறுமை visit_cmp_eq_gt(Ast_முனை_Biop* nodep, மாறிலி ComparisonType cmpType) {
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
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            மாறிலி தருக்க lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            மாறிலி தருக்க rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            எனில் (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } இல்லையெனில் எனில் (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } இல்லையெனில் {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            மாறிலி தருக்க isAggrLhs = isAggregateType(nodep->lhsp());
            மாறிலி தருக்க isAggrRhs = isAggregateType(nodep->rhsp());

            எனில் ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                மாறிலி Ast_முனை_DType* மாறிலி lhsDType = nodep->lhsp()->dtypep();
                மாறிலி Ast_முனை_DType* மாறிலி rhsDType = nodep->rhsp()->dtypep();

                எனில் (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_முனை* மாறிலி newp
                        = புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } இல்லையெனில் எனில் (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                எனில் (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_முனை* மாறிலி newp
                        = புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
                எனில் (Ast_முனை_Biop* மாறிலி newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } இல்லையெனில் எனில் (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                எனில் (Ast_முனை_Biop* மாறிலி newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } இல்லையெனில் {
                மாறிலி தருக்க signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                எனில் (Ast_முனை_Biop* மாறிலி newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                மாறிலி முழு width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                மாறிலி முழு ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                தருக்க warnOn = உண்மை;
                எனில் (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    எனில் ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = பொய்;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    எனில் ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = பொய்;
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
    வெறுமை visit_cmp_real(Ast_முனை_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit_cmp_string(Ast_முனை_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    வெறுமை visit_cmp_type(Ast_முனை_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            மாறிலி AstAttrOf* மாறிலி lhsap = VN_AS(nodep->lhsp(), AttrOf);
            மாறிலி AstAttrOf* மாறிலி rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            மாறிலி Ast_முனை_DType* மாறிலி lhsDtp = lhsap->dtypep();
            மாறிலி Ast_முனை_DType* மாறிலி rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            மாறிலி தருக்க invert = VN_IS(nodep, NeqT);
            மாறிலி தருக்க identical
                = Ast_முனை::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            மாறிலி தருக்க eq = invert ^ identical;
            Ast_முனை* மாறிலி newp = புதிய AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }

    வெறுமை visit_negate_not(Ast_முனை_Uniop* nodep, தருக்க real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            எனில் (!real_ok) checkCvtUS(nodep->lhsp(), பொய்);
        }
        எனில் (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            எனில் (Ast_முனை_Uniop* மாறிலி newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                திரும்பு;
            }
        } இல்லையெனில் {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        எனில் (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        எனில் (m_vup->final()) {
            Ast_முனை_DType* மாறிலி expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_முனை_DType* மாறிலி subDTypep = expDTypep;
            // Some warning suppressions
            தருக்க lhsWarn = உண்மை;
            எனில் (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                எனில் (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = பொய்;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    வெறுமை visit_signed_unsigned(Ast_முனை_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), உண்மை);
            மாறிலி முழு width = nodep->lhsp()->width();
            Ast_முனை_DType* மாறிலி expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_முனை_DType* மாறிலி subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    வெறுமை visit_shift(Ast_முனை_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        மாறிலி Ast_முனை_Biop* மாறிலி newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (வெறுமை)newp;  // Ununused
    }
    வெறுமை iterate_shift_prelim(Ast_முனை_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), பொய்);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            எனில் (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_முனை_Biop* iterate_shift_final(Ast_முனை_Biop* nodep) {
        // Nodep maybe edited
        assert_At_கோவை(nodep);
        எனில் (m_vup->final()) {
            Ast_முனை_DType* மாறிலி expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_முனை_DType* மாறிலி subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            எனில் (VN_IS(nodep, ShiftRS)) {
                எனில் (Ast_முனை_Biop* மாறிலி newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            தருக்க warnOn = உண்மை;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            எனில் (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), முனை_Assign)) warnOn = பொய்;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* மாறிலி shiftp = VN_CAST(nodep->rhsp(), Const);
            எனில் (shiftp && !shiftp->இலக்கம்().isFourState() && shiftp->width() <= 32) {
                மாறிலி int64_t shiftVal = shiftp->இலக்கம்().toSQuad();
                எனில் (VN_IS(nodep, ShiftL)) {
                    எனில் (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = பொய்;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            எனில் (nodep->rhsp()->width() > 32) {
                எனில் (shiftp && shiftp->இலக்கம்().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_எண் இலக்கம்{shiftp, 32, shiftp->இலக்கம்()};
                    Ast_முனை* மாறிலி shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(புதிய AstConst{shiftrhsp->fileline(), இலக்கம்});
                    VL_DO_DANGLING(shiftrhsp->delete_மரம்(), shiftrhsp);
                }
            }
        }
        திரும்பு nodep;  // May edit
    }

    வெறுமை visit_boolexpr_and_or(Ast_முனை_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), பொய்);
            checkCvtUS(nodep->rhsp(), பொய்);
            மாறிலி முழு width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            மாறிலி முழு mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            மாறிலி தருக்க expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_முனை_கோவை::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        எனில் (m_vup->final()) {
            Ast_முனை_DType* மாறிலி expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_முனை_DType* மாறிலி subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    வெறுமை visit_add_sub_replace(Ast_முனை_Biop* nodep, தருக்க real_ok, மாறிலி தருக்க modOrDiv) {
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
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            எனில் (!real_ok) {
                checkCvtUS(nodep->lhsp(), பொய்);
                checkCvtUS(nodep->rhsp(), பொய்);
            }
            எனில் (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                எனில் (Ast_முனை_Biop* மாறிலி newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                திரும்பு;
            } இல்லையெனில் எனில் (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            } இல்லையெனில் {
                மாறிலி முழு width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                மாறிலி முழு mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                மாறிலி தருக்க expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_முனை_கோவை::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        எனில் (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_முனை_DType* மாறிலி expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_முனை_DType* மாறிலி subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            எனில் (Ast_முனை_Biop* மாறிலி newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            தருக்க lhsWarn = உண்மை;
            தருக்க rhsWarn = உண்மை;
            எனில் (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                எனில் (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = பொய்;
                எனில் (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = பொய்;
                எனில் (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = பொய்;  // do_increment + ...
                எனில் (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = பொய்;  // ... + do_increment
            } இல்லையெனில் எனில் (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                எனில் (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = பொய்;
                எனில் (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = பொய்;
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
    வெறுமை visit_real_add_sub(Ast_முனை_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    வெறுமை visit_real_neg_ceil(Ast_முனை_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_கோவை(nodep);
        எனில் (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    வெறுமை visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->கோவை_Format_2(), nodep, "Assumes constant format");
        தருக்க inPct = பொய்;
        Ast_முனை_கோவை* argp = nodep->exprsp();
        string newFormat;
        க்கு (எழுத்து ch : nodep->text()) {
            எனில் (!inPct && ch == '%') {
                inPct = உண்மை;
                newFormat += ch;
            } இல்லையெனில் எனில் (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } இல்லையெனில் எனில் (!inPct) {  // Normal text
                newFormat += ch;
            } இல்லையெனில் {
                inPct = பொய்;
                Ast_முனை_கோவை* மாறிலி nextp = argp ? VN_AS(argp->nextp(), முனை_கோவை) : nullptr;
                AstSFormatArg* மாறிலி fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_முனை_கோவை* subargp = fargp ? fargp->exprp() : argp;
                மாறிலி Ast_முனை_DType* மாறிலி dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                தேர்வு (ch) {
                நிலை '?':  // Unspecified by user, guess
                    எனில் (dtypep->isDouble()) {
                        ch = 'g';
                    } இல்லையெனில் எனில் (dtypep->isString()) {
                        ch = 's';
                    } இல்லையெனில் எனில் (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } இல்லையெனில் {
                        ch = 'p';
                    }
                    argp = nextp;
                    நிறுத்து;
                நிலை '%': நிறுத்து;  // %% - just output a %
                நிலை 'm': நிறுத்து;  // %m - auto insert "name"
                நிலை 'l': நிறுத்து;  // %m - auto insert "library"
                நிலை 'd':
                    எனில் (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        எனில் (subargp->isDouble()) spliceCvtS(subargp, உண்மை, 64);
                        argp = nextp;
                    }
                    நிறுத்து;
                நிலை 'b':  // FALLTHRU
                நிலை 'o':  // FALLTHRU
                நிலை 'x':
                    எனில் (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    நிறுத்து;
                நிலை 't':  // Decimal/float time
                    எனில் (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    நிறுத்து;
                நிலை 'e':  // FALLTHRU
                நிலை 'f':  // FALLTHRU
                நிலை 'g':
                    எனில் (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        எனில் (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    நிறுத்து;
                நிலை 'p':  // FALLTHRU
                நிலை 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    எனில் (subargp && !fargp) {
                        எனில் (AstEnumDType* மாறிலி enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    நிறுத்து;
                இயல்பு:  // Most operators, just move to next argument
                    argp = nextp;
                    நிறுத்து;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    நிலையான AstEnumDType* formatEnumDType(Ast_முனை_கோவை* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        எனில் (!enumDtp) {
            எனில் (மாறிலி AstVarRef* மாறிலி varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        திரும்பு enumDtp;
    }

    Ast_கோவை_Stmt* newEnumCapture(Ast_முனை_கோவை* valuep, AstEnumDType* dtypep) {
        File_வரி* மாறிலி flp = valuep->fileline();
        AstVar* மாறிலி varp = புதிய AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(உண்மை);
        எனில் (m_ftaskp) {
            varp->funcLocal(உண்மை);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } இல்லையெனில் {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_கோவை_Stmt* மாறிலி exprp = புதிய Ast_கோவை_Stmt{
            flp, புதிய AstAssign{flp, புதிய AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            புதிய AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        திரும்பு exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_முனை_கோவை* valuep, AstEnumDType* dtypep) {
        எனில் (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_முனை_கோவை* keyp;
        எனில் (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_மரம்(பொய்);
        } இல்லையெனில் {
            Ast_கோவை_Stmt* மாறிலி capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_மரம்_Pure(பொய்);
        }
        மாறிலி VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* மாறிலி argp = புதிய AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        திரும்பு argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    தருக்க widthBad(Ast_முனை* nodep, Ast_முனை_DType* expDTypep) {
        மாறிலி முழு expWidth = expDTypep->width();
        முழு expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        எனில் (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) திரும்பு பொய்;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        எனில் (expWidthMin == 0) expWidthMin = expWidth;
        எனில் (nodep->dtypep()->width() == expWidth) திரும்பு பொய்;
        எனில் (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) திரும்பு உண்மை;
        எனில் (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) திரும்பு உண்மை;
        திரும்பு பொய்;
    }

    வெறுமை fixWidthExtend(Ast_முனை_கோவை* nodep, Ast_முனை_DType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<முழு>(extendRule) << ") old: " << nodep);
        எனில் (extendRule == EXTEND_OFF) திரும்பு;
        AstConst* மாறிலி constp = VN_CAST(nodep, Const);
        மாறிலி முழு expWidth = expDTypep->width();
        எனில் (constp && !constp->இலக்கம்().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_எண் இலக்கம்{nodep, expWidth, constp->இலக்கம்()};
            இலக்கம்.isSigned(பொய்);
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstConst{nodep->fileline(), இலக்கம்};
            constp->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } இல்லையெனில் எனில் (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(உண்மை);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } இல்லையெனில் {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            தருக்க doSigned = பொய்;
            தேர்வு (extendRule) {
            நிலை EXTEND_ZERO: doSigned = பொய்; நிறுத்து;
            நிலை EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); நிறுத்து;
            நிலை EXTEND_LHS: doSigned = nodep->isSigned(); நிறுத்து;
            இயல்பு: nodep->v3fatalSrc("bad case");
            }
            Ast_முனை_கோவை* மாறிலி newp
                = (doSigned ? static_cast<Ast_முனை_கோவை*>(புதிய AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_முனை_கோவை*>(புதிய AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(உண்மை);
            linker.relink(newp);
            nodep = newp;
        }
        எனில் (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_முனை_கோவை* மாறிலி newp = spliceCvtD(nodep);
            nodep = newp;
        }
        எனில் (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } இல்லையெனில் {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    வெறுமை fixWidthReduce(Ast_முனை_கோவை* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        மாறிலி முழு expWidth = 1;
        மாறிலி முழு expSigned = பொய்;
        எனில் (nodep->width() == expWidth) திரும்பு;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* மாறிலி constp = VN_CAST(nodep, Const);
        எனில் (constp) {
            V3_எண் இலக்கம்(nodep, expWidth);
            இலக்கம்.opRedOr(constp->இலக்கம்());
            இலக்கம்.isSigned(expSigned);
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstConst{nodep->fileline(), இலக்கம்};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_மரம்(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } இல்லையெனில் {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_முனை_கோவை* newp;
            Ast_முனை_DType* மாறிலி dtypeSkipp = nodep->dtypep()->skipRefp();
            எனில் (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = புதிய AstNeq{nodep->fileline(),
                                  புதிய AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } இல்லையெனில் {
                // But, AstRedOr can be faster on wide
                newp = புதிய AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    தருக்க fixAutoExtend(Ast_முனை_கோவை*& nodepr, முழு expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        எனில் (AstConst* மாறிலி constp = VN_CAST(nodepr, Const)) {
            எனில் (constp->இலக்கம்().autoExtend() && !constp->இலக்கம்().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_எண் இலக்கம்(constp, expWidth);
                இலக்கம்.opRepl(constp->இலக்கம்(), expWidth);  // {width{'1}}
                Ast_முனை_கோவை* மாறிலி newp = புதிய AstConst{constp->fileline(), இலக்கம்};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_மரம்(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                திரும்பு உண்மை;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            இல்லையெனில் எனில் (!constp->இலக்கம்().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->இலக்கம்().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_எண் இலக்கம்(constp, expWidth);
                இலக்கம்.opExtendXZ(constp->இலக்கம்(), constp->width());
                Ast_முனை_கோவை* மாறிலி newp = புதிய AstConst{constp->fileline(), இலக்கம்};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_மரம்(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;  // No change
    }
    தருக்க isBaseClassRecurse(மாறிலி AstClass* மாறிலி cls1p, மாறிலி AstClass* மாறிலி cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        எனில் (cls1p == cls2p) திரும்பு உண்மை;
        க்கு (மாறிலி AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            எனில் (isBaseClassRecurse(cls1p, cextp->classp())) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    // Lines to end a message on nodep with the data types of its operands
    நிலையான string warnDTypes(மாறிலி Ast_முனை* nodep, மாறிலி Ast_முனை_DType* lhsDtp,
                             மாறிலி Ast_முனை_DType* rhsDtp, மாறிலி string& lhsName = "Left-hand",
                             மாறிலி string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        எனில் (lhsState == rhsState) lhsState = rhsState = "";
        திரும்பு "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    நிலையான தருக்க unpackedArraySizesMatch(மாறிலி Ast_முனை_DType* lhsDtp, மாறிலி Ast_முனை_DType* rhsDtp) {
        போது (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            மாறிலி AstUnpackArrayDType* மாறிலி lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            மாறிலி AstUnpackArrayDType* மாறிலி rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            எனில் (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                திரும்பு பொய்;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        திரும்பு உண்மை;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    வெறுமை checkUnpackedArrayAssignmentCompatible(மாறிலி Ast_முனை* nodep, மாறிலி T* மாறிலி lhsRefp,
                                                மாறிலி N* மாறிலி rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::மதிப்பு || std::is_same<T, Ast_முனை_Var_Ref>::மதிப்பு)
                && (std::is_same<N, AstVar>::மதிப்பு || std::is_same<N, Ast_முனை_Var_Ref>::மதிப்பு),
            "Unsupported types provided.");
        எனில் (!lhsRefp || !rhsRefp) திரும்பு;
        std::string lhsName;
        std::string rhsName;
        எனில் (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } இல்லையெனில் {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        மாறிலி Ast_முனை_DType* மாறிலி lhsDtp = lhsRefp->dtypep()->skipRefp();
        மாறிலி Ast_முனை_DType* மாறிலி rhsDtp = rhsRefp->dtypep()->skipRefp();
        மாறிலி தருக்க isLhsAggregate = lhsDtp->isAggregateType();
        மாறிலி தருக்க isRhsAggregate = rhsDtp->isAggregateType();
        எனில் (!isLhsAggregate && !isRhsAggregate) திரும்பு;
        எனில் (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            திரும்பு;
        } இல்லையெனில் எனில் (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            திரும்பு;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(பொய்),
                                      rhsDim = rhsDtp->dimensions(பொய்);
        // Check if unpacked array dimensions are matching
        எனில் (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            திரும்பு;
        }

        மாறிலி Ast_முனை_DType* lhsDtpIterp = lhsDtp;
        மாறிலி Ast_முனை_DType* rhsDtpIterp = rhsDtp;
        க்கு (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            எனில் (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                திரும்பு;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        எனில் (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    வெறுமை checkClassAssign(மாறிலி Ast_முனை* nodep, மாறிலி எழுத்து* side, Ast_முனை* rhsp,
                          Ast_முனை_DType* மாறிலி lhsDTypep, தருக்க isOutputArg = பொய்) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        மாறிலி Ast_முனை_DType* மாறிலி lhsRawDTypep = lhsDTypep->skipRefp();
        மாறிலி Ast_முனை_DType* மாறிலி rhsRawDTypep = rhsp->dtypep()->skipRefp();
        எனில் (மாறிலி AstClassRefDType* மாறிலி lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            எனில் (மாறிலி AstClassRefDType* மாறிலி rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                மாறிலி AstClass* மாறிலி dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                மாறிலி AstClass* மாறிலி srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                எனில் (isBaseClassRecurse(dstp, srcp)) திரும்பு;
            } இல்லையெனில் எனில் (rhsp->isNull()) {
                திரும்பு;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } இல்லையெனில் எனில் (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        எனில் (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                புதிய AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, முனை_கோவை), lhsDTypep});
        }
    }
    நிலையான தருக்க similarDTypeRecurse(மாறிலி Ast_முனை_DType* மாறிலி node1p,
                                    மாறிலி Ast_முனை_DType* மாறிலி node2p) {
        திரும்பு node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    வெறுமை iterateCheckFileDesc(Ast_முனை* parentp, Ast_முனை* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_முனை_DType* மாறிலி expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (வெறுமை)underp;  // cppcheck
    }
    வெறுமை iterateCheckReal(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    வெறுமை iterateCheckSigned8(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    வெறுமை iterateCheckSigned32(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    வெறுமை iterateCheckUInt32(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    வெறுமை iterateCheckDelay(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        எனில் (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        எனில் (stage & FINAL) {
            Ast_முனை_DType* expDTypep;
            எனில் (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } இல்லையெனில் {
                File_வரி* மாறிலி newFl = புதிய File_வரி{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, உண்மை);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, பொய்);
        }
        (வெறுமை)underp;  // cppcheck
    }
    வெறுமை iterateCheckString(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    வெறுமை iterateCheckTyped(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp,
                           Ast_முனை_DType* expDTypep, Stage stage) {
        எனில் (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        எனில் (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (வெறுமை)underp;  // cppcheck
    }
    வெறுமை iterateCheckTypedSelfPrelim(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp,
                                     Ast_முனை_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        எனில் (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        எனில் (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (வெறுமை)underp;  // cppcheck
    }
    வெறுமை iterateCheckIntegralSelf(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, உண்மை);
    }
    வெறுமை iterateCheckSelf(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Determ determ,
                          Stage stage, தருக்க integralOnly = பொய்) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        எனில் (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, முனை_கோவை) ? checkCvtUS(VN_AS(underp, முனை_கோவை), integralOnly) : underp;
        Ast_முனை_DType* மாறிலி expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (வெறுமை)underp;  // cppcheck
    }
    வெறுமை iterateCheckSizedSelf(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Determ determ,
                               Stage stage, தருக்க deferUntyped = பொய்) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        எனில் (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, முனை_கோவை) ? checkCvtUS(VN_AS(underp, முனை_கோவை), பொய்) : underp;
        Ast_முனை_DType* மாறிலி expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_முனை_DType* மாறிலி checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        எனில் (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (வெறுமை)underp;  // cppcheck
    }
    வெறுமை iterateCheckAssign(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* rhsp, Stage stage,
                            Ast_முனை_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        எனில் (AstConst* constp = VN_CAST(rhsp, Const)) {
            எனில் (மாறிலி AstUnpackArrayDType* மாறிலி arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                எனில் (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    எனில் (basicp->width() == 8 && constp->இலக்கம்().isFromString()) {
                        AstConsArray* newp = புதிய AstConsArray{
                            constp->fileline(), lhsDTypep,
                            புதிய AstConst{constp->fileline(), AstConst::Widthed_மதிப்பு{}, 8, 0}};
                        க்கு (மாறிலி முழு aindex : arrayp->declRange().seqLoToHi()) {
                            முழு cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_எண் selected{constp, 8};
                            selected.opSel(constp->இலக்கம்(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            எனில் (!selected.isFourState()) {
                                எனில் (மாறிலி uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 புதிய AstConst{constp->fileline(),
                                                              AstConst::Widthed_மதிப்பு{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(தள்ளு_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        மாறிலி தருக்க lhsStream = (VN_IS(parentp, முனை_Assign)
                                && VN_IS(VN_AS(parentp, முனை_Assign)->lhsp(), முனை_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, உண்மை, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (வெறுமை)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_முனை_கோவை* convertUnpackedOrder(Ast_முனை_கோவை* exprp, Ast_முனை_DType* dtypep) {
        மாறிலி AstUnpackArrayDType* மாறிலி arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        எனில் (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) திரும்பு exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_முனை_கோவை* மாறிலி newp = புதிய AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        திரும்பு newp;
    }

    நிலையான தருக்க isLogicalOperator(மாறிலி Ast_முனை* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        திரும்பு VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    வெறுமை iterateCheckBool(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Stage stage) {
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
        Ast_முனை_DType* மாறிலி underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        எனில் (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_முனை* மாறிலி newp
                = புதிய AstNeqD{parentp->fileline(), VN_AS(underp, முனை_கோவை),
                              புதிய AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } இல்லையெனில் எனில் (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_முனை* மாறிலி newp
                = புதிய AstNeqN{parentp->fileline(), VN_AS(underp, முனை_கோவை),
                              புதிய AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } இல்லையெனில் எனில் (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, முனை_கோவை)), underp);  // Changed
        } இல்லையெனில் எனில் (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(புதிய AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(தள்ளு_Deletep(underp), underp);
        } இல்லையெனில் {
            மாறிலி தருக்க bad = widthBad(underp, parentp->findBitDType());
            எனில் (bad) {
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
            எனில் (Ast_கோவை_Stmt* மாறிலி கோவை_Stmtp = VN_CAST(underp, கோவை_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(கோவை_Stmtp->resultp());
                கோவை_Stmtp->dtypeFrom(கோவை_Stmtp->resultp());
            } இல்லையெனில் எனில் (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, முனை_கோவை)), underp);  // Changed
            }
        }
    }

    நிலையான வெறுமை checkEnum(Ast_முனை* nodep, Ast_முனை* parentp, மாறிலி Ast_முனை_DType* targetDTypep,
                          மாறிலி Ast_முனை_DType* sourceDTypep, மாறிலி Ast_முனை_கோவை* sourceExprp) {
        எனில் (மாறிலி AstEnumDType* மாறிலி targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            மாறிலி VCastable castable
                = Ast_முனை::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            எனில் (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    Ast_முனை* iterateCheck(Ast_முனை* parentp, மாறிலி எழுத்து* side, Ast_முனை* underp, Determ determ,
                          Stage stage, Ast_முனை_DType* expDTypep, ExtendRule extendRule,
                          தருக்க warnOn = உண்மை, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        மாறிலி StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        எனில் (rhsAccess.isReadOnly()) {
            எனில் (Ast_முனை_கோவை* மாறிலி exprp = VN_CAST(underp, முனை_கோவை)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        எனில் (VN_IS(underp, முனை_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } இல்லையெனில் எனில் (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } இல்லையெனில் எனில் (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } இல்லையெனில் எனில் (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_முனை* மாறிலி oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, முனை_கோவை));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } இல்லையெனில் எனில் (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_முனை* மாறிலி oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, முனை_கோவை), உண்மை, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } இல்லையெனில் எனில் (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_முனை* மாறிலி oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, முனை_கோவை));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } இல்லையெனில் {
            மாறிலி Ast_முனை_DType* மாறிலி underDtp = underp->dtypep()->skipRefp();
            எனில் (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_முனை* மாறிலி newp
                    = புதிய AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(underp), underp);
                underp = newp;
            }
            மாறிலி Ast_முனை_DType* மாறிலி expDtp = expDTypep->skipRefp();
            எனில் (determ == ASSIGN && VN_IS(expDtp, முனை_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            எனில் (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            மாறிலி AstBasicDType* மாறிலி expBasicp = expDTypep->basicp();
            மாறிலி AstBasicDType* மாறிலி underBasicp = underp->dtypep()->basicp();
            எனில் (expBasicp && underBasicp) {
                எனில் (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    எனில் (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, முனை_கோவை));
                    }
                    எனில் (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_முனை_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                எனில் (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } இல்லையெனில் எனில் (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    எனில் (underp->isSigned() != subDTypep->isSigned()
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
                } இல்லையெனில் {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, முனை_கோவை), expDTypep, extendRule,
                                warnOn);
            } இல்லையெனில் எனில் (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } இல்லையெனில் எனில் (மாறிலி AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                மாறிலி AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                எனில் (VN_IS(underp, Const) && VN_AS(underp, Const)->இலக்கம்().isNull()) {
                    // '= null' is ok
                } இல்லையெனில் எனில் (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } இல்லையெனில் எனில் (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } இல்லையெனில் எனில் (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } இல்லையெனில் {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } இல்லையெனில் {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        திரும்பு underp;
    }

    வெறுமை
    widthCheckSized(Ast_முனை* parentp, மாறிலி எழுத்து* side,
                    Ast_முனை_கோவை* underp,  // Node to be checked or have typecast added in front of
                    Ast_முனை_DType* expDTypep, ExtendRule extendRule, தருக்க warnOn = உண்மை) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        மாறிலி AstBasicDType* மாறிலி expBasicp = expDTypep->basicp();
        மாறிலி AstBasicDType* மாறிலி underBasicp = underp->dtypep()->basicp();
        எனில் (expDTypep == underp->dtypep()) {
            திரும்பு;  // Same type must match
        } இல்லையெனில் எனில் (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#எனில் VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            திரும்பு;
        } இல்லையெனில் {
            மாறிலி முழு expWidth = expDTypep->width();
            முழு expWidthMin = expDTypep->widthMin();
            எனில் (expWidthMin == 0) expWidthMin = expWidth;
            மாறிலி தருக்க bad = widthBad(underp, expDTypep);
            எனில் ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                திரும்பு;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            எனில் (VN_IS(parentp->backp(), CastSize)) warnOn = பொய்;
            எனில் (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = பொய்;
            எனில் (VN_IS(underp, Const) && VN_AS(underp, Const)->இலக்கம்().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = பொய்;
            }
            எனில் ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = பொய்;
            }
            எனில் (bad && warnOn) {
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
            எனில் (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                மாறிலி Ast_முனை_Assign* assignp = VN_CAST(parentp, முனை_Assign);
                மாறிலி AstPin* pinp = VN_CAST(parentp, Pin);
                எனில் (assignp && VN_IS(assignp->lhsp(), முனை_Stream)) {
                } இல்லையெனில் எனில் (pinp && pinp->modVarp()->direction() != VDirection::உள்ளீடு) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } இல்லையெனில் எனில் (assignp && VN_IS(underp, முனை_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    மாறிலி முழு shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* மாறிலி widenedp = புதிய AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(உண்மை);
                    // Shift left so zeros fill on the right
                    Ast_முனை_கோவை* மாறிலி shiftedp = புதிய AstShiftL{
                        underp->fileline(), widenedp,
                        புதிய AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } இல்லையெனில் {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_முனை_கோவை* checkCvtUS(Ast_முனை_கோவை* nodep, தருக்க fatal) {
        எனில் (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            எனில் (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } இல்லையெனில் {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, பொய், 32);
        }
        திரும்பு nodep;
    }

    Ast_முனை_கோவை* spliceCvtD(Ast_முனை_கோவை* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        எனில் (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_முனை_கோவை* newp;
            எனில் (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = புதிய AstISToRD{nodep->fileline(), nodep};
            } இல்லையெனில் {
                newp = புதிய AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            திரும்பு newp;
        } இல்லையெனில் {
            திரும்பு nodep;
        }
    }
    Ast_முனை_கோவை* spliceCvtS(Ast_முனை_கோவை* nodep, தருக்க warnOn, முழு width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        எனில் (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                எனில் (constp->isDouble() && V3_எண்::epsilonIntegral(constp->இலக்கம்().toDouble())) {
                    warnOn = பொய்;
                }
            }
            எனில் (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            திரும்பு newp;
        } இல்லையெனில் {
            திரும்பு nodep;
        }
    }
    Ast_முனை_கோவை* spliceCvtString(Ast_முனை_கோவை* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        எனில் (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            திரும்பு newp;
        } இல்லையெனில் {
            திரும்பு nodep;
        }
    }
    Ast_முனை_Biop* replaceWithUOrSVersion(Ast_முனை_Biop* nodep, தருக்க signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        எனில் (signedFlavorNeeded == nodep->signedFlavor()) திரும்பு nullptr;
        எனில் (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        தேர்வு (nodep->type()) {
        நிலை VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); திரும்பு nullptr;
        நிலை VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); திரும்பு nullptr;
        நிலை VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); திரும்பு nullptr;
        நிலை VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); திரும்பு nullptr;
        நிலை VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); திரும்பு nullptr;
        நிலை VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); திரும்பு nullptr;
        நிலை VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); திரும்பு nullptr;
        நிலை VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); திரும்பு nullptr;
        நிலை VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); திரும்பு nullptr;
        இயல்பு: நிறுத்து;
        }
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_Biop* newp = nullptr;
        தேர்வு (nodep->type()) {
        நிலை VNType::Gt: newp = புதிய AstGtS{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::GtS: newp = புதிய AstGt{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Gte: newp = புதிய AstGteS{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::GteS: newp = புதிய AstGte{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Lt: newp = புதிய AstLtS{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::LtS: newp = புதிய AstLt{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Lte: newp = புதிய AstLteS{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::LteS: newp = புதிய AstLte{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Div: newp = புதிய AstDivS{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::DivS: newp = புதிய AstDiv{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::ModDiv: newp = புதிய AstModDivS{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::ModDivS: newp = புதிய AstModDiv{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Mul: newp = புதிய AstMulS{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::MulS: newp = புதிய AstMul{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::ShiftR: newp = புதிய AstShiftRS{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::ShiftRS: newp = புதிய AstShiftR{fl, lhsp, rhsp}; நிறுத்து;
        இயல்பு:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            நிறுத்து;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு newp;
    }
    Ast_முனை_Biop* replaceWithDVersion(Ast_முனை_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        எனில் (nodep->doubleFlavor()) திரும்பு nullptr;
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        தேர்வு (nodep->type()) {
        நிலை VNType::Add: newp = புதிய AstAddD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Sub: newp = புதிய AstSubD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Pow: newp = புதிய AstPowD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Eq:
        நிலை VNType::EqCase: newp = புதிய AstEqD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Neq:
        நிலை VNType::NeqCase: newp = புதிய AstNeqD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Gt:
        நிலை VNType::GtS: newp = புதிய AstGtD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Gte:
        நிலை VNType::GteS: newp = புதிய AstGteD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Lt:
        நிலை VNType::LtS: newp = புதிய AstLtD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Lte:
        நிலை VNType::LteS: newp = புதிய AstLteD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Div:
        நிலை VNType::DivS: newp = புதிய AstDivD{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Mul:
        நிலை VNType::MulS: newp = புதிய AstMulD{fl, lhsp, rhsp}; நிறுத்து;
        இயல்பு:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            நிறுத்து;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு newp;
    }
    Ast_முனை_Biop* replaceWithNVersion(Ast_முனை_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        எனில் (nodep->stringFlavor()) திரும்பு nullptr;
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        தேர்வு (nodep->type()) {
        நிலை VNType::Eq:
        நிலை VNType::EqCase:
        நிலை VNType::EqWild: newp = புதிய AstEqN{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Neq:
        நிலை VNType::NeqCase:
        நிலை VNType::NeqWild: newp = புதிய AstNeqN{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Gt:
        நிலை VNType::GtS: newp = புதிய AstGtN{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Gte:
        நிலை VNType::GteS: newp = புதிய AstGteN{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Lt:
        நிலை VNType::LtS: newp = புதிய AstLtN{fl, lhsp, rhsp}; நிறுத்து;
        நிலை VNType::Lte:
        நிலை VNType::LteS: newp = புதிய AstLteN{fl, lhsp, rhsp}; நிறுத்து;
        இயல்பு:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            நிறுத்து;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு newp;
    }
    Ast_முனை_Uniop* replaceWithDVersion(Ast_முனை_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        எனில் (nodep->doubleFlavor()) திரும்பு nullptr;
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_முனை_Uniop* newp = nullptr;
        தேர்வு (nodep->type()) {
        நிலை VNType::Negate: newp = புதிய AstNegateD{fl, lhsp}; நிறுத்து;
        இயல்பு:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            நிறுத்து;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    வெறுமை replaceWithSFormat(AstMethodCall* nodep, மாறிலி string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* மாறிலி argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_முனை_Var_Ref* மாறிலி fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_முனை* மாறிலி newp = புதிய AstAssign{
            nodep->fileline(), fromp,
            புதிய AstSFormatF{nodep->fileline(), format, பொய், argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        தள்ளு_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_முனை_DType* iterateEditMoveDTypep(Ast_முனை* parentp, Ast_முனை_DType* dtnodep) {
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
        மாறிலி தருக்க child1 = (parentp->getChildDTypep() == dtnodep);
        மாறிலி தருக்க child2 = (parentp->getChild2DTypep() == dtnodep);
        எனில் (child1 || child2) {
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
        எனில் (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        திரும்பு dtnodep;
    }

    AstConst* dimension_மதிப்பு(File_வரி* fileline, Ast_முனை_DType* nodep, VAttrType attrType,
                             முழு dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_முனை_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        க்கு (முழு i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            எனில் (மாறிலி Ast_முனை_Array_DType* மாறிலி adtypep = VN_CAST(dtypep, முனை_Array_DType)) {
                declRange = adtypep->declRange();
                எனில் (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                தொடர்;
            } இல்லையெனில் எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி adtypep
                       = VN_CAST(dtypep, முனை_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                நிறுத்து;  // Sub elements don't look like arrays and can't iterate into
            } இல்லையெனில் எனில் (மாறிலி AstBasicDType* மாறிலி adtypep = VN_CAST(dtypep, BasicDType)) {
                எனில் (adtypep->isRanged()) declRange = adtypep->declRange();
                நிறுத்து;
            }
            நிறுத்து;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        முழு val = 0;
        தேர்வு (attrType) {
        நிலை VAttrType::DIM_BITS: {
            முழு bits = 1;
            போது (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                எனில் (மாறிலி Ast_முனை_Array_DType* மாறிலி adtypep = VN_CAST(dtypep, முனை_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    தொடர்;
                } இல்லையெனில் எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி adtypep
                           = VN_CAST(dtypep, முனை_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    நிறுத்து;
                } இல்லையெனில் எனில் (மாறிலி AstBasicDType* மாறிலி adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    நிறுத்து;
                }
                நிறுத்து;
            }
            எனில் (dim == 0) {
                val = 0;
            } இல்லையெனில் எனில் (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } இல்லையெனில் {
                val = bits;
            }
            நிறுத்து;  // LCOV_EXCL_LINE
        }
        நிலை VAttrType::DIM_உயர்வு: val = !declRange.ranged() ? 0 : declRange.hi(); நிறுத்து;
        நிலை VAttrType::DIM_இடது: val = !declRange.ranged() ? 0 : declRange.இடது(); நிறுத்து;
        நிலை VAttrType::DIM_கீழ்: val = !declRange.ranged() ? 0 : declRange.lo(); நிறுத்து;
        நிலை VAttrType::DIM_வலது: val = !declRange.ranged() ? 0 : declRange.வலது(); நிறுத்து;
        நிலை VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            நிறுத்து;
        நிலை VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); நிறுத்து;
        இயல்பு: nodep->v3fatalSrc("Missing DIM ATTR type case"); நிறுத்து;
        }
        எனில் (!valp) valp = புதிய AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        திரும்பு valp;
    }
    AstVarRef* dimensionVarRefp(Ast_முனை_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        எனில் (cachedpr) திரும்பு cachedpr->clone_மரம்(பொய்);

        Ast_முனை_Array_DType* மாறிலி vardtypep = புதிய AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), புதிய AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* மாறிலி consp = புதிய AstConsArray{nodep->fileline(), vardtypep};
        க்கு (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_மதிப்பு(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_மரம்(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(உண்மை);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(உண்மை);
        தள்ளு_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        திரும்பு cachedpr->clone_மரம்(பொய்);
    }
    நிலையான uint64_t enum_Max_மதிப்பு(மாறிலி Ast_முனை* errNodep, மாறிலி AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        க்கு (மாறிலி AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            மாறிலி AstConst* மாறிலி vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            எனில் (!vconstp->இலக்கம்().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        எனில் (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            திரும்பு 64;
        }
        திரும்பு maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* மாறிலி nodep, VAttrType attrType, தருக்க assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        எனில் (cachedpr) திரும்பு cachedpr->clone_மரம்(பொய்);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_முனை_DType* basep;
        எனில் (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } இல்லையெனில் எனில் (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } இல்லையெனில் {
            basep = nodep->dtypep();
        }
        Ast_முனை_DType* vardtypep;
        எனில் (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = புதிய AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } இல்லையெனில் {
            vardtypep = புதிய AstUnpackArrayDType{flp, basep, புதிய AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        எனில் (attrType == VAttrType::ENUM_NAME) {
            defaultp = புதிய AstConst{flp, AstConst::String{}, ""};
        } இல்லையெனில் எனில் (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = புதிய AstConst{flp, V3_எண்{nodep, nodep->width(), 0}};
        } இல்லையெனில் எனில் (attrType == VAttrType::ENUM_VALID) {
            defaultp = புதிய AstConst{flp, AstConst::BitFalse{}};
        } இல்லையெனில் {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_முனை_கோவை*> மதிப்புகள்;
        {
            uint64_t ordinal = 0;
            AstEnumItem* மாறிலி firstp = nodep->itemsp();
            மாறிலி AstEnumItem* prevp = firstp;  // Prev must start with last item
            போது (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            க்கு (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* மாறிலி nextp = VN_AS(itemp->nextp(), EnumItem);
                மாறிலி AstConst* மாறிலி vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                எனில் (!vconstp->இலக்கம்().isAnyXZ()) {  // Can 2-state runtime decode
                    மாறிலி uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    எனில் (attrType == VAttrType::ENUM_NAME) {
                        மதிப்புகள்[i] = புதிய AstConst{flp, AstConst::String{},
                                                 V3_எண்::displayedEnumName(itemp)};
                    } இல்லையெனில் எனில் (attrType == VAttrType::ENUM_NEXT) {
                        மதிப்புகள்[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_மரம்(பொய்);  // A const
                    } இல்லையெனில் எனில் (attrType == VAttrType::ENUM_PREV) {
                        மதிப்புகள்[i] = prevp->valuep()->clone_மரம்(பொய்);  // A const
                    } இல்லையெனில் எனில் (attrType == VAttrType::ENUM_VALID) {
                        மதிப்புகள்[i] = புதிய AstConst{flp, AstConst::BitTrue{}};
                    } இல்லையெனில் {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_முனை_கோவை* valuep = nullptr;
        எனில் (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* மாறிலி consp = புதிய AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            க்கு (மாறிலி auto& itr : மதிப்புகள்) {
                V3_எண் சாவி{nodep, nodep->width()};
                சாவி.setQuad(itr.first);
                consp->addItemsp(புதிய AstConsAssocItem{flp, புதிய AstConst{flp, சாவி}, itr.second});
            }
            valuep = consp;
        } இல்லையெனில் {
            AstConsArray* மாறிலி consp = புதிய AstConsArray{flp, vardtypep, defaultp};
            க்கு (uint64_t i = 0; i < (msbdim + 1); ++i) {
                எனில் (மதிப்புகள்[i]) consp->valuep(i, மதிப்புகள்[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(உண்மை);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(உண்மை);
        தள்ளு_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        திரும்பு cachedpr->clone_மரம்(பொய்);
    }

    Ast_முனை_கோவை* enumSelect(Ast_முனை_கோவை* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        எனில் (attrType == VAttrType::ENUM_NAME) {
            எனில் (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const)) {
                Ast_முனை_கோவை* மாறிலி newp = புதிய AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->இலக்கம்().displayedEnumName(adtypep)};
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு newp;
            }
        }
        எனில் (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_கோவை_Stmt* மாறிலி capturep = newEnumCapture(nodep, adtypep);
            Ast_முனை_கோவை* மாறிலி valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            திரும்பு capturep;
        }
        எனில் (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_முனை_கோவை* indexp = புதிய AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            க்கு (மாறிலி AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                மாறிலி AstConst* மாறிலி constp = VN_AS(itemp->valuep(), Const);
                எனில் (constp->இலக்கம்().isAnyXZ()) தொடர்;
                indexp = புதிய AstCond{nodep->fileline(),
                                     புதிய AstEq{nodep->fileline(), nodep->clone_மரம்(பொய்),
                                               itemp->valuep()->clone_மரம்(பொய்)},
                                     புதிய AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* மாறிலி tabRefp = enumVarRefp(adtypep, attrType, பொய், ordinal);
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு newp;
        }
        மாறிலி uint64_t msbdim = enum_Max_மதிப்பு(nodep, adtypep);
        மாறிலி தருக்க assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_முனை_கோவை* newp;
        எனில் (assoc) {
            AstVarRef* மாறிலி tabRefp = enumVarRefp(adtypep, attrType, உண்மை, 0);
            newp = புதிய AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } இல்லையெனில் {
            மாறிலி முழு selwidth = V3_எண்::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* மாறிலி tabRefp
                = enumVarRefp(adtypep, attrType, பொய், (1ULL << selwidth) - 1);
            Ast_முனை_கோவை* மாறிலி boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? புதிய AstGt{nodep->fileline(), nodep->clone_மரம்(பொய்),
                                  புதிய AstConst{nodep->fileline(),
                                               V3_எண்{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = புதிய AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                புதிய AstSel{nodep->fileline(), nodep, 0, selwidth}};
            எனில் (boundp) {
                newp = புதிய AstCond{nodep->fileline(), boundp,
                                   புதிய AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        எனில் (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } இல்லையெனில் எனில் (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } இல்லையெனில் {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        திரும்பு newp;
    }
    Ast_முனை_கோவை* enumTestValid(Ast_முனை_கோவை* valp, AstEnumDType* enumDtp) {
        மாறிலி uint64_t maxval = enum_Max_மதிப்பு(valp, enumDtp);
        மாறிலி தருக்க assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_முனை_கோவை* testp = nullptr;
        File_வரி* மாறிலி fl_novalue = புதிய File_வரி{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, உண்மை);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, உண்மை);
        எனில் (assoc) {
            AstVarRef* மாறிலி tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, உண்மை, 0);
            testp = புதிய AstAssocSel{fl_novalue, tabRefp, valp->clone_மரம்_Pure(பொய்)};
        } இல்லையெனில் {
            மாறிலி முழு selwidth = V3_எண்::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* மாறிலி tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, பொய், (1ULL << selwidth) - 1);
            File_வரி* மாறிலி fl_nowidth = புதிய File_வரி{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, உண்மை);
            testp = புதிய AstCond{
                fl_novalue,
                புதிய AstGt{fl_nowidth, valp->clone_மரம்_Pure(பொய்),
                          புதிய AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                புதிய AstConst{fl_novalue, AstConst::BitFalse{}},
                புதிய AstArraySel{fl_novalue, tabRefp,
                                புதிய AstSel{fl_novalue, valp->clone_மரம்_Pure(பொய்), 0, selwidth}}};
        }
        திரும்பு testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, மாறிலி VNumRange& range) {
        PatVecMap patmap;
        முழு element = range.இடது();
        க்கு (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            எனில் (patp->keyp()) {
                எனில் (மாறிலி AstConst* மாறிலி constp = patternConstKeyp(patp)) {
                    எனில் (constp->இலக்கம்().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } இல்லையெனில் {
                        element = constp->toSInt();
                    }
                }
            }
            மாறிலி தருக்க newEntry = patmap.emplace(element, patp).second;
            எனில் (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.இடது_To_வலது_Inc();
        }
        திரும்பு patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    வெறுமை patConcatConvertRecurse(AstPattern* patternp, Ast_முனை_கோவை* nodep) {
        எனில் (AstConcat* மாறிலி concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } இல்லையெனில் {
            AstPatMember* மாறிலி newp
                = புதிய AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(உண்மை);
            patternp->addItemsp(newp);
        }
    }

    வெறுமை makeOpenArrayShell(Ast_முனை_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_முனை_FTask* மாறிலி oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_முனை_FTask* மாறிலி newTaskp = oldTaskp->clone_மரம்(பொய்);
        newTaskp->dpiOpenChild(உண்மை);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        க்கு (மாறிலி auto& tconnect : tconnects) {
            AstVar* மாறிலி portp = tconnect.first;
            மாறிலி AstArg* மாறிலி argp = tconnect.second;
            மாறிலி Ast_முனை* மாறிலி pinp = argp->exprp();
            எனில் (!pinp) தொடர்;  // Argument error we'll find later
            எனில் (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_முனை_DType* மாறிலி oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_முனை* nodep) {
                    எனில் (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    தருக்க markHasOpenArray(Ast_முனை_FTask* nodep) {
        தருக்க hasOpen = பொய்;
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                எனில் (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(உண்மை);
                    hasOpen = உண்மை;
                }
            }
        }
        திரும்பு hasOpen;
    }
    தருக்க hasOpenArrayDTypeRecurse(Ast_முனை_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        எனில் (VN_IS(nodep, UnsizedArrayDType)) திரும்பு உண்மை;
        எனில் (nodep->subDTypep()) திரும்பு hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        திரும்பு பொய்;
    }
    Ast_முனை_DType* dtypeNotIntAtomOrVecRecurse(Ast_முனை_DType* nodep, தருக்க ranged = பொய்) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        எனில் (AstBasicDType* மாறிலி dtp = VN_CAST(nodep, BasicDType)) {
            எனில் (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                திரும்பு dtp;  // Packed when already packed
            எனில் (dtp->keyword().isIntNumeric()) திரும்பு nullptr;
            திரும்பு dtp;
        } இல்லையெனில் எனில் (AstPackArrayDType* மாறிலி dtp = VN_CAST(nodep, PackArrayDType)) {
            எனில் (ranged) திரும்பு dtp;  // Packed when already packed
            திரும்பு dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), உண்மை);
        }
        திரும்பு nodep;
    }
    Ast_முனை_DType* dtypeNot4StateIntegralRecurse(Ast_முனை_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        எனில் (AstBasicDType* மாறிலி dtp = VN_CAST(nodep, BasicDType)) {
            எனில் (!dtp->keyword().isFourstate()) திரும்பு dtp;
            திரும்பு nullptr;
        } இல்லையெனில் எனில் (Ast_முனை_Array_DType* மாறிலி dtp = VN_CAST(nodep, முனை_Array_DType)) {
            திரும்பு dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } இல்லையெனில் எனில் (Ast_முனை_UOr_Struct_DType* மாறிலி dtp = VN_CAST(nodep, முனை_UOr_Struct_DType)) {
            க்கு (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_முனை_DType* மாறிலி badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                எனில் (badDtp) திரும்பு badDtp;
            }
            திரும்பு nullptr;
        }
        திரும்பு nodep;
    }
    வெறுமை checkForceReleaseLhs(Ast_முனை* nodep, Ast_முனை* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        எனில் (Ast_முனை* மாறிலி selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    வெறுமை assert_At_கோவை(Ast_முனை* nodep) {
        எனில் (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    வெறுமை assertAtStatement(Ast_முனை* nodep) {
        எனில் (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    வெறுமை checkConstantOrReplace(Ast_முனை* nodep, தருக்க noFourState, மாறிலி string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* மாறிலி constp = VN_CAST(nodep, Const);
        எனில் (!constp || (noFourState && constp->இலக்கம்().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(தள்ளு_Deletep(nodep), nodep, constp);
            திரும்பு;
        }
    }
    Ast_முனை* முனை_For_Unsized_Warning(Ast_முனை* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        எனில் (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            திரும்பு nodep->op1p();
        } இல்லையெனில் எனில் (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            திரும்பு nodep->op2p();
        }
        திரும்பு nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_முனை* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        எனில் (VL_LIKELY(!nodep)) திரும்பு nullptr;
        எனில் (auto* மாறிலி refp = VN_CAST(nodep, RefDType)) {
            எனில் (refp->typedefp() == typedefp) திரும்பு refp;
        }
        எனில் (auto* மாறிலி refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) திரும்பு refp;
        எனில் (auto* மாறிலி refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) திரும்பு refp;
        எனில் (auto* மாறிலி refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) திரும்பு refp;
        எனில் (auto* மாறிலி refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) திரும்பு refp;
        திரும்பு nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    நிலையான தருக்க isIfaceArrayDType(மாறிலி Ast_முனை_DType* dtypep) {
        எனில் (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) திரும்பு பொய்;
        மாறிலி AstIfaceRefDType* மாறிலி irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        திரும்பு irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    நிலையான std::string unpackedArraySize(மாறிலி Ast_முனை_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        போது (மாறிலி AstUnpackArrayDType* மாறிலி arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            எனில் (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        திரும்பு size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    நிலையான Ast_முனை_கோவை* newIfaceArrayElement(Ast_முனை_கோவை* fromp, முழு bit) {
        AstConsArray* மாறிலி consp = VN_CAST(fromp, ConsArray);
        எனில் (!consp || !isIfaceArrayDType(consp->dtypep())) திரும்பு nullptr;
        மாறிலி AstUnpackArrayDType* மாறிலி arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        எனில் (bit < 0 || bit >= arrp->elementsConst()) திரும்பு nullptr;
        திரும்பு consp->valuep(bit)->clone_மரம்(பொய்);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_முனை_கோவை* new_Iface_Array_மதிப்பு(மாறிலி Ast_முனை_Var_Ref* refp, Ast_முனை_DType* dtypep,
                                    மாறிலி std::string& name, மாறிலி std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_வரி* மாறிலி flp = refp->fileline();
        AstUnpackArrayDType* மாறிலி arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        எனில் (!arrp) {
            AstVar* மாறிலி varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_முனை_Var_Ref* newp;
            எனில் (மாறிலி AstVarXRef* மாறிலி xrefp = VN_CAST(refp, VarXRef)) {
                newp = புதிய AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } இல்லையெனில் {
                newp = புதிய AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            திரும்பு newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* மாறிலி consp = புதிய AstConsArray{flp, arrp};
        மாறிலி VNumRange range = arrp->declRange();
        க்கு (மாறிலி முழு i : range.seq_இடது_To_வலது()) {
            மாறிலி std::string s = name + "__BRA__" + Ast_முனை::encode_எண்(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_மதிப்பு(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        திரும்பு consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    வெறுமை replaceSplitIfaceArrayRef(Ast_முனை_Var_Ref* nodep) {
        AstVar* மாறிலி varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        எனில் (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_மதிப்பு(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    வெறுமை expandIfaceArrayPinDimensions(AstPin* nodep, மாறிலி Ast_முனை_DType* dtypep,
                                       Ast_முனை_கோவை* conp, மாறிலி std::string& suffix,
                                       AstVar*& elemVarpr) {
        மாறிலி AstUnpackArrayDType* மாறிலி arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        எனில் (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* மாறிலி newp = nodep->clone_மரம்(பொய்);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            எனில் (conp) newp->exprp(conp->clone_மரம்(பொய்));
            nodep->addNextHere(newp);
            திரும்பு;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        மாறிலி VNumRange range = arrp->declRange();
        க்கு (முழு n = range.elements() - 1; n >= 0; --n) {
            மாறிலி முழு i = range.இடது() + n * range.இடது_To_வலது_Inc();
            மாறிலி std::string s = suffix + "__BRA__" + Ast_முனை::encode_எண்(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_முனை_கோவை* மாறிலி elemConnp = [&]() -> Ast_முனை_கோவை* {
                எனில் (!conp) திரும்பு nullptr;
                மாறிலி AstUnpackArrayDType* மாறிலி conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                மாறிலி முழு elements = conArrp->elementsConst();
                // Counted from lo
                Ast_முனை_கோவை* மாறிலி resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                திரும்பு resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    வெறுமை expandIfaceArrayPin(AstPin* nodep) {
        AstVar* மாறிலி portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        க்கு (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_முனை_கோவை* மாறிலி exprp = VN_AS(nodep->exprp(), முனை_கோவை);
        எனில் (exprp) தள்ளு_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        மாறிலி தருக்க connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_முனை* userIterateSubtreeReturnEdits(Ast_முனை* nodep, WidthVP* vup) {
        எனில் (!nodep) திரும்பு nullptr;
        Ast_முனை* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        திரும்பு ret;
    }
    வெறுமை userIterate(Ast_முனை* nodep, WidthVP* vup) {
        எனில் (!nodep) திரும்பு;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    வெறுமை userIterateAndNext(Ast_முனை* nodep, WidthVP* vup) {
        எனில் (!nodep) திரும்பு;
        எனில் (nodep->didWidth()) திரும்பு;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    வெறுமை userIterateChildren(Ast_முனை* nodep, WidthVP* vup) {
        எனில் (!nodep) திரும்பு;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    வெறுமை userIterateChildrenBackwardsConst(Ast_முனை* nodep, WidthVP* vup) {
        எனில் (!nodep) திரும்பு;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

பொது:
    // CONSTRUCTORS
    WidthVisitor(தருக்க paramsOnly,  // [in] TRUE if we are considering parameters only.
                 தருக்க doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_முனை* mainAcceptEdit(Ast_முனை* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        க்கு (AstVar* மாறிலி varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(தள்ளு_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        திரும்பு nodep;
    }
    நிலையான தருக்க lowerAsFixedAggregate(மாறிலி Ast_முனை_DType* மாறிலி dtypep) {
        திரும்பு dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    வெறுமை streamAssignLower(Ast_முனை_Assign* nodep) {
        Ast_முனை_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        எனில் (Ast_முனை_Stream* மாறிலி streamp = VN_CAST(nodep->rhsp(), முனை_Stream)) {
            Ast_முனை_DType* மாறிலி lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            மாறிலி முழு lwidth = lhsDTypeSkippedRefp->widthStream();
            மாறிலி முழு rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            எனில் (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            எனில் (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                மாறிலி முழு queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            எனில் (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    புதிய AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        எனில் (Ast_முனை_Stream* மாறிலி streamp = VN_CAST(nodep->lhsp(), முனை_Stream)) {
            மாறிலி Ast_முனை_DType* மாறிலி rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_முனை_DType* மாறிலி lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            மாறிலி முழு lwidth = lhsStreamDTypep->widthStream();
            மாறிலி முழு rwidth = rhsDTypep->widthStream();
            எனில் (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            எனில் (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_முனை_கோவை* மாறிலி streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_முனை_கோவை* மாறிலி dstp = streamp->lhsp()->unlinkFrBack();
                Ast_முனை_கோவை* srcp = nodep->rhsp()->unlinkFrBack();
                எனில் (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_முனை_கோவை::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } இல்லையெனில் {
                    எனில் (srcp->width() > lwidth) {
                        srcp = புதிய AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(தள்ளு_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(புதிய AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } இல்லையெனில் எனில் (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(புதிய AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = இயல்பு;
};

//######################################################################
// Width class functions

வெறுமை V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        மாறிலி WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        மாறிலி VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{பொய், பொய்};
        (வெறுமை)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (வெறுமை)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("width", 0, dump_மரம்_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_முனை* V3Width::widthParamsEdit(Ast_முனை* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{உண்மை, பொய்};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    திரும்பு nodep;
}

வெறுமை V3Width::streamAssignLowerEdit(Ast_முனை_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{பொய், பொய்};
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
Ast_முனை* V3Width::widthGenerateParamsEdit(
    Ast_முனை* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{உண்மை, உண்மை};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    திரும்பு nodep;
}

Ast_முனை* V3Width::selectNonConstantRecurse(Ast_முனை* nodep, தருக்க inSel) {
    // If node has a non-constant select, return that select
    Ast_முனை* resultp = nullptr;
    எனில் (Ast_முனை_Sel* மாறிலி anodep = VN_CAST(nodep, முனை_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        எனில் (resultp) திரும்பு resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), உண்மை);
    } இல்லையெனில் எனில் (AstSel* மாறிலி anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        எனில் (resultp) திரும்பு resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), உண்மை);
    } இல்லையெனில் எனில் (Ast_முனை_Var_Ref* மாறிலி anodep = VN_CAST(nodep, முனை_Var_Ref)) {
        எனில் (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) திரும்பு anodep;
    } இல்லையெனில் {
        எனில் (Ast_முனை* மாறிலி refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        எனில் (resultp) திரும்பு resultp;
        எனில் (Ast_முனை* மாறிலி refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        எனில் (resultp) திரும்பு resultp;
        எனில் (Ast_முனை* மாறிலி refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        எனில் (resultp) திரும்பு resultp;
        எனில் (Ast_முனை* மாறிலி refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    திரும்பு resultp;
}
