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

वग्ग ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ध्रुव Ast_ग्रन्थिः*, AstClass*> m_cache;

    AstClass* findCached(Ast_ग्रन्थिः* nodep) {
        सचे (!nodep) पटिनिवत्त nullptr;
        ध्रुव auto it = m_cache.find(nodep);
        सचे (it != m_cache.end()) पटिनिवत्त it->second;

        AstClass* classp = nullptr;
        सचे (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } अञ्ञथा सचे (AstClass* ध्रुव parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } अञ्ञथा सचे (AstClassPackage* ध्रुव packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } अञ्ञथा {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        पटिनिवत्त classp;
    }

सार्वजनिक:
    AstClass* find(Ast_ग्रन्थिः* nodep) {
        सचे (AstClass* ध्रुव classp = VN_CAST(nodep, Class)) पटिनिवत्त classp;
        सचे (AstClassPackage* ध्रुव packagep = VN_CAST(nodep, ClassPackage)) {
            पटिनिवत्त packagep->classp();
        }
        पटिनिवत्त findCached(nodep);
    }
};

//######################################################################

गणना Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, ध्रुव Stage& rhs) {
    पटिनिवत्त str << ("-PFB"[static_cast<पूर्णाङ्क>(rhs)]);
}

गणना Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, ध्रुव Determ& rhs) {
    स्थिर constexpr ध्रुव वर्ण* ध्रुव s_det[] = {"SELF", "CNTX", "ASSN"};
    पटिनिवत्त str << s_det[rhs];
}

गणना StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, सन्देशः) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               सन्देशः)

//######################################################################
// Width state, as a visitor of each AstNode

वग्ग WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_ग्रन्थिः_DType* ध्रुव m_dtypep;  // Parent's data type to resolve to
    ध्रुव Stage m_stage;  // If true, report errors
    ध्रुव StreamUse m_streamUse;  // Current expression may be a stream concat

सार्वजनिक:
    WidthVP(Ast_ग्रन्थिः_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        सचे (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { पटिनिवत्त एतत्; }
    तार्किक selfDtm() ध्रुव { पटिनिवत्त m_dtypep == nullptr; }
    StreamUse streamUse() ध्रुव { पटिनिवत्त m_streamUse; }
    Ast_ग्रन्थिः_DType* dtypep() ध्रुव {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        पटिनिवत्त m_dtypep;
    }
    Ast_ग्रन्थिः_DType* dtypeNullp() ध्रुव { पटिनिवत्त m_dtypep; }
    Ast_ग्रन्थिः_DType* dtypeNullSkipRefp() ध्रुव {
        Ast_ग्रन्थिः_DType* dtp = dtypeNullp();
        सचे (dtp) dtp = dtp->skipRefp();
        पटिनिवत्त dtp;
    }
    Ast_ग्रन्थिः_DType* dtypeOverridep(Ast_ग्रन्थिः_DType* ध्रुव defaultp) ध्रुव {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        सचे (!m_dtypep) पटिनिवत्त defaultp;
        सचे (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            पटिनिवत्त defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        पटिनिवत्त m_dtypep;
    }
    पूर्णाङ्क width() ध्रुव {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        पटिनिवत्त m_dtypep->width();
    }
    पूर्णाङ्क widthMin() ध्रुव {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        पटिनिवत्त m_dtypep->widthMin();
    }
    तार्किक prelim() ध्रुव { पटिनिवत्त m_stage & PRELIM; }
    तार्किक final() ध्रुव { पटिनिवत्त m_stage & FINAL; }
    शून्य dump(std::ostream& str) ध्रुव {
        सचे (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } अञ्ञथा {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, ध्रुव WidthVP* vup) {
    सचे (vup) vup->dump(str);
    पटिनिवत्त str;
}

//######################################################################

वग्ग WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    शून्य clearWidthRecurse(Ast_ग्रन्थिः* nodep) {
        अत्थाय (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(मिच्छा);
            सचे (Ast_ग्रन्थिः* ध्रुव refp = nodep->op1p()) clearWidthRecurse(refp);
            सचे (Ast_ग्रन्थिः* ध्रुव refp = nodep->op2p()) clearWidthRecurse(refp);
            सचे (Ast_ग्रन्थिः* ध्रुव refp = nodep->op3p()) clearWidthRecurse(refp);
            सचे (Ast_ग्रन्थिः* ध्रुव refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    आभास ~WidthClearVisitor() = स्वभाव;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_ग्रन्थिः_iterate_instead_of_Ast_ग्रन्थिः_accept

//######################################################################

वग्ग WidthVisitor final : सार्वजनिक VNVisitor {
    // TYPES
    using PatVecMap = std::map<पूर्णाङ्क, AstPatMember*>;
    using DTypeMap = std::map<ध्रुव std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ध्रुव AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    तार्किक m_underFork = मिच्छा;  // Visiting under a fork
    तार्किक m_underSExpr = मिच्छा;  // Visiting under a sequence expression
    तार्किक m_underPackedArray = मिच्छा;  // Visiting under a AstPackArrayDType
    तार्किक m_underMemberSel = मिच्छा;  // Viting under a MemberSel
    Ast_ग्रन्थिः* m_seqUnsupp = nullptr;  // Property has unsupported node
    तार्किक m_hasSExpr = मिच्छा;  // Property has a sequence expression
    ध्रुव AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    ध्रुव AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_ग्रन्थिः_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ग्रन्थिः_Module* m_modep = nullptr;  // Current module
    ध्रुव AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_ग्रन्थिः_Procedure* m_procedurep = nullptr;  // Current final/always
    ध्रुव AstWith* m_withp = nullptr;  // Current 'with' statement
    ध्रुव AstFunc* m_funcp = nullptr;  // Current function
    ध्रुव AstAttrOf* m_attrp = nullptr;  // Current attribute
    ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    ध्रुव तार्किक m_paramsOnly;  // Computing parameter value; limit operation
    ध्रुव तार्किक m_doGenerate;  // Do errors later inside generate statement
    तार्किक m_streamConcat = मिच्छा;  // True if visiting arguments of stream concatenation
    तार्किक m_inParamOverride = मिच्छा;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ध्रुव Ast_ग्रन्थिः_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<ध्रुव Ast_ग्रन्थिः_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<ध्रुव AstVar*> m_curModVars;  // Variables declared in current module

    स्थिर constexpr पूर्णाङ्क ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    गणना ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    गणना वग्ग ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    स्थिर StreamUse streamUseForDeterm(Determ determ) {
        पटिनिवत्त determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() ध्रुव { पटिनिवत्त m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    स्थिर शून्य packIfUnpacked(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
        सचे (AstUnpackArrayDType* ध्रुव unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ध्रुव पूर्णाङ्क elements_अङ्कः = unpackDTypep->arrayUnpackedElements();
            ध्रुव पूर्णाङ्क unpackMinBits = elements_अङ्कः * unpackDTypep->subDTypep()->widthMin();
            ध्रुव पूर्णाङ्क unpackBits = elements_अङ्कः * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(नूतन AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    स्थिर Ast_ग्रन्थिः_DType* fromDTypep(Ast_ग्रन्थिः* fromp) {
        सचे (Ast_ग्रन्थिः_DType* ध्रुव dtypep = VN_CAST(fromp, ग्रन्थिः_DType))
            पटिनिवत्त dtypep->skipRefOrNullp();
        पटिनिवत्त fromp ? fromp->dtypep() : nullptr;
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
    शून्य visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, सच्च, मिच्छा); }
    शून्य visit(AstSub* nodep) override { visit_add_sub_replace(nodep, सच्च, मिच्छा); }
    शून्य visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, सच्च, सच्च); }
    शून्य visit(AstMul* nodep) override { visit_add_sub_replace(nodep, सच्च, मिच्छा); }
    // These can't promote to real
    शून्य visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, मिच्छा, सच्च); }
    शून्य visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, मिच्छा, सच्च); }
    शून्य visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, मिच्छा, मिच्छा); }
    शून्य visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, मिच्छा, सच्च); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    शून्य visit(AstNegate* nodep) override { visit_negate_not(nodep, सच्च); }
    // Unary never real
    शून्य visit(AstNot* nodep) override { visit_negate_not(nodep, मिच्छा); }

    // Real: inputs and output real
    शून्य visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(Ast_ग्रन्थिः_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    शून्य visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    शून्य visit(Ast_ग्रन्थिः_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
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
        सचे (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    शून्य visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        सचे (m_vup && m_vup->prelim()) {
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
        सचे (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    शून्य visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        सचे (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    शून्य visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        सचे (m_vup && m_vup->prelim()) {
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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    शून्य visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            सचे (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } अञ्ञथा {
                nodep->dtypeSetInteger();
            }
        }
    }
    शून्य visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
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
            नूतन AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    स्थिर तार्किक isOverridePattern(ध्रुव Ast_ग्रन्थिः* nodep) {
        सचे (ध्रुव AstPattern* ध्रुव patternp = VN_CAST(nodep, Pattern)) {
            पटिनिवत्त !patternp->childDTypep();
        }
        ध्रुव AstCond* ध्रुव condp = VN_CAST(nodep, Cond);
        पटिनिवत्त condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    शून्य visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ध्रुव पूर्णाङ्क errors = V3Error::error_गणनम्();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = मिच्छा;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->condp(), Const);
            ध्रुव तार्किक known = constp && !constp->अङ्कः().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            सचे (!known && V3Error::error_गणनम्() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            पटिनिवत्त;
        }
        सचे (m_vup->prelim()) {  // First stage evaluation
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
            Ast_ग्रन्थिः_DType* ध्रुव thenDTypep = nodep->thenp()->dtypep();
            Ast_ग्रन्थिः_DType* ध्रुव elseDTypep = nodep->elsep()->dtypep();
            Ast_ग्रन्थिः_DType* ध्रुव thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_ग्रन्थिः_DType* ध्रुव elseDTypeSkippedp = elseDTypep->skipRefp();
            सचे (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } अञ्ञथा सचे (nodep->thenp()->is_Class_Handle_मानम्()
                       || nodep->elsep()->is_Class_Handle_मानम्()) {
                Ast_ग्रन्थिः_DType* commonClassTypep = nullptr;
                सचे (nodep->thenp()->is_Class_Handle_मानम्() && nodep->elsep()->is_Class_Handle_मानम्()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_ग्रन्थिः::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                सचे (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } अञ्ञथा {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } अञ्ञथा सचे (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                सचे (Ast_ग्रन्थिः::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } अञ्ञथा {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    सचे (!Ast_ग्रन्थिः::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } अञ्ञथा सचे (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } अञ्ञथा सचे (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } अञ्ञथा {
                ध्रुव तार्किक isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                तार्किक handled = मिच्छा;
                ध्रुव तार्किक isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_ग्रन्थिः_DType* ध्रुव thenEnumDTypep = thenDTypep->skipRefToEnump();
                सचे (VN_IS(thenDTypeSkippedp, ग्रन्थिः_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_वृक्षः(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = सच्च;
                } अञ्ञथा सचे (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_वृक्षः(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    सचे (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = सच्च;
                    } अञ्ञथा {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(नूतन File_पङ्क्तिः{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, सच्च);
                    }
                }
                सचे (!handled) {
                    ध्रुव पूर्णाङ्क width = std::max(thenDTypep->width(), elseDTypep->width());
                    ध्रुव पूर्णाङ्क widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    ध्रुव VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ध्रुव तार्किक isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    सचे ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } अञ्ञथा सचे ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } अञ्ञथा {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        सचे (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
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
        Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }
        सचे (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* ध्रुव newp = नूतन AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            पटिनिवत्त;
        }

        // Concat handling
        सचे (m_vup->prelim()) {
            सचे (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            सचे (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } अञ्ञथा {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                सचे (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            सचे (ध्रुव AstReplicate* ध्रुव repp = VN_CAST(nodep->lhsp(), Replicate)) {
                सचे (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त;
                }
            }
            सचे (ध्रुव AstReplicate* ध्रुव repp = VN_CAST(nodep->rhsp(), Replicate)) {
                सचे (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त;
                }
            }
        }
        सचे (m_vup->final()) {
            सचे (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nodep->rhsp()->unlinkFrBack();
                सचे (!lhsp->isString()) lhsp = नूतन AstCvtPackString{lhsp->fileline(), lhsp};
                सचे (!rhsp->isString()) rhsp = नूतन AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_ग्रन्थिः* ध्रुव newp = नूतन AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            }
            सचे (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    शून्य visit(AstConcatN* nodep) override {
        सचे (nodep->didWidth()) पटिनिवत्त;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        सचे (m_vup->final()) {
            nodep->didWidth(सच्च);
            सचे (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
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
        सचे (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            सचे (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            सचे (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                पटिनिवत्त;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            पटिनिवत्त;
        }
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            पटिनिवत्त;
        }
        सचे (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            पटिनिवत्त;
        }
        सचे (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            पटिनिवत्त;
        }
        सचे (nodep->fileline()->timingOn()) {
            सचे (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                पटिनिवत्त;
            } अञ्ञथा सचे (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } अञ्ञथा {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        सचे (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    शून्य visit(AstFork* nodep) override {
        सचे (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            पटिनिवत्त;
        }
        सचे (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ध्रुव newp = नूतन AstBegin{nodep->fileline(), nodep->name(), nullptr, मिच्छा};
            nodep->replaceWith(newp);
            सचे (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            सचे (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            सचे (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा सचे (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = सच्च;
            iterateChildren(nodep);
        } अञ्ञथा सचे (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
        } अञ्ञथा {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    शून्य visitWaitOrDisableFork(Ast_ग्रन्थिः* nodep) {
        सचे (nodep->fileline()->timingOn()) {
            सचे (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
                पटिनिवत्त;
            } अञ्ञथा सचे (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    शून्य visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    शून्य visit(AstToLowerN* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    शून्य visit(AstToUpperN* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    शून्य visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->countp(), Const);
            सचे (constp) {
                सचे (constp->अङ्कः().isFourState()
                    || (constp->dtypep()->isSigned() && constp->अङ्कः().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } अञ्ञथा {
                    times = constp->toSInt();
                }
            }

            Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullSkipRefp();
            सचे (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                सचे (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                सचे (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त;
                } अञ्ञथा {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* ध्रुव newp = नूतन AstPattern{
                        nodep->fileline(),
                        नूतन AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    पटिनिवत्त;
                }
            }
            सचे (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            सचे (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } अञ्ञथा {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            सचे ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_ग्रन्थिः* ध्रुव newp
                    = नूतन AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            } अञ्ञथा {
                सचे (!constp) nodep->v3error("Replication value isn't a constant.");
                सचे (times == 0
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
        सचे (m_vup->final()) {
            सचे (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    शून्य visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        सचे (m_vup->final()) {
            सचे (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Dist_Biop* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Dist_Triop* nodep) override {
        सचे (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    तार्किक streamImplicitUseAllowed(ध्रुव Ast_ग्रन्थिः_Stream* nodep) ध्रुव {
        सचे (m_streamConcat) पटिनिवत्त सच्च;
        सचे (m_vup && m_vup->streamUse() != STREAM_USE_NONE) पटिनिवत्त सच्च;

        ध्रुव Ast_ग्रन्थिः* ध्रुव backp = nodep->backp();
        सचे (!backp) पटिनिवत्त सच्च;  // Error elsewhere

        पटिनिवत्त VN_IS(backp, ग्रन्थिः_Assign);
    }
    शून्य visit(Ast_ग्रन्थिः_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            m_streamConcat = सच्च;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = मिच्छा;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->rhsp(), Const)) {
                सचे (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } अञ्ञथा {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            सचे (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, ग्रन्थिः_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, ग्रन्थिः_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } अञ्ञथा सचे (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } अञ्ञथा {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        सचे (m_vup->final()) {
            सचे (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            सचे (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
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
        checkConstantOrReplace(nodep->leftp(), सच्च,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), सच्च,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        सचे (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ध्रुव पूर्णाङ्क width = nodep->elementsConst();
            सचे (nodep->fromBracket() && nodep->वामः_Const() > nodep->दक्षिणः_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } अञ्ञथा सचे (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ध्रुव तार्किक inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ध्रुव तार्किक inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ध्रुव तार्किक inTypeTable = !m_modep;
            ध्रुव Ast_ग्रन्थिः* basep = nodep->backp();
            याव (VN_IS(basep, Range)) basep = basep->backp();
            सचे (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->वामः_Const() << ":" << nodep->दक्षिणः_Const()
                                            << "]");
            }
        }
    }

    शून्य visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        सचे (nodep->didWidth()) पटिनिवत्त;
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), मिच्छा);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            पूर्णाङ्क width = nodep->widthConst();
            सचे (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                पटिनिवत्त;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ध्रुव तार्किक isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ध्रुव तार्किक inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            सचे (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                प्रक्षिप_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(नूतन AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            सचे (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                सचे (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_ग्रन्थिः_DType* ध्रुव subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    मिच्छा /*noerror*/);
                } अञ्ञथा {
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
            पूर्णाङ्क frommsb = nodep->fromp()->width() - 1;
            पूर्णाङ्क fromlsb = 0;
            ध्रुव पूर्णाङ्क elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            सचे (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } अञ्ञथा {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ध्रुव पूर्णाङ्क selwidth = V3_सङ्ख्या::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ग्रन्थिः_DType* ध्रुव selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            सचे (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                सचे (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            सचे (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                सचे (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } अञ्ञथा सचे (!inParameterizedTemplate) {
                    सचे (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } अञ्ञथा {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                सचे (!isWriteSelect) {
                    // Extend it
                    ध्रुव पूर्णाङ्क extendTo = nodep->msbConst() + 1;
                    Ast_ग्रन्थिः_DType* ध्रुव subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    मिच्छा /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            सचे (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                मिच्छा /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    शून्य visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            सचे (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->bitp(), Const)) {
                    ध्रुव पूर्णाङ्क bit = constp->toSInt();
                    Ast_ग्रन्थिः_व्यञ्जकम्* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    सचे (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ध्रुव AstUnpackArrayDType* ध्रुव arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त;
                }
            }
            //
            पूर्णाङ्क frommsb;
            पूर्णाङ्क fromlsb;
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
            सचे (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                सचे (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                सचे (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } अञ्ञथा {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ध्रुव पूर्णाङ्क selwidth = V3_सङ्ख्या::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ग्रन्थिः_DType* ध्रुव selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            सचे (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                सचे (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            सचे (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                सचे (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    सचे (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                मिच्छा /*NOWARN*/);
            }
        }
    }

    शून्य visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ध्रुव AstAssocArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, AssocArrayDType);
            सचे (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->कुञ्चिका_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    शून्य visit(AstAlias* nodep) override {
        सचे (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        ध्रुव auto check_If_व्यञ्जकम्_Ok = [एतत्](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp) {
            सचे (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                पटिनिवत्त मिच्छा;
            }

            ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(exprp, VarRef);
            सचे (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                पटिनिवत्त मिच्छा;
            }
            AstVar* ध्रुव varp = varRefp->varp();
            सचे (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                पटिनिवत्त मिच्छा;
            }
            सचे (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                पटिनिवत्त मिच्छा;
            } अञ्ञथा {
                m_aliasedVars.निवेशय(varp);
            }
            पटिनिवत्त सच्च;
        };

        check_If_व्यञ्जकम्_Ok(nodep->itemsp());
        Ast_ग्रन्थिः_DType* ध्रुव firstItemDtypep = nodep->itemsp()->dtypep();
        अत्थाय (Ast_ग्रन्थिः* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_व्यञ्जकम्_Ok(VN_AS(itemp, ग्रन्थिः_व्यञ्जकम्));
            सचे (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    शून्य visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ध्रुव AstWildcardArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            सचे (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ध्रुव AstBasicDType* ध्रुव basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            सचे (!basicp
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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            सचे (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_ग्रन्थिः_DType* ध्रुव newDtp
                = नूतन AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          नूतन AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            सचे (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ध्रुव पूर्णाङ्क subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ध्रुव VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                सचे ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(नूतन AstConsArray{
                        nodep->fileline(), newDtp,
                        नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त;
                } अञ्ञथा सचे ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            सचे (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ध्रुव fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ध्रुव newp = नूतन AstConsArray{nodep->fileline(), newDtp};
                ध्रुव VNumRange range = nodep->declRange();
                अत्थाय (पूर्णाङ्क n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_वृक्षः(मिच्छा));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
    }

    शून्य visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ग्रन्थिः* ध्रुव selp = V3Width::widthSelNoIterEdit(nodep);
        सचे (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            पटिनिवत्त;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    शून्य visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ग्रन्थिः* ध्रुव selp = V3Width::widthSelNoIterEdit(nodep);
        सचे (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            पटिनिवत्त;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    शून्य visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ग्रन्थिः* ध्रुव selp = V3Width::widthSelNoIterEdit(nodep);
        सचे (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            पटिनिवत्त;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    शून्य visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ग्रन्थिः* ध्रुव selp = V3Width::widthSelNoIterEdit(nodep);
        सचे (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            पटिनिवत्त;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    शून्य visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        सचे (m_vup && m_vup->prelim()) {
            सचे (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } अञ्ञथा सचे (nodep->अङ्कः().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } अञ्ञथा सचे (nodep->अङ्कः().isString()) {
                nodep->dtypeSetString();
            } अञ्ञथा सचे (nodep->अङ्कः().isDouble()) {
                nodep->dtypeChgWidth(nodep->अङ्कः().width(), nodep->अङ्कः().sized()
                                                               ? nodep->अङ्कः().width()
                                                               : nodep->अङ्कः().widthToFit());
            } अञ्ञथा {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->अङ्कः().width(),
                    nodep->अङ्कः().sized() ? nodep->अङ्कः().width() : nodep->अङ्कः().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->अङ्कः().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    शून्य visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        सचे (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    शून्य visit(AstFell* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सच्च);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstFalling* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सच्च);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstFuture* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सच्च);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    शून्य visit(AstPast* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सच्च);
            nodep->dtypeFrom(nodep->exprp());
            सचे (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ सच्च);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->ticksp(), Const);
                सचे (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_वृक्षः();
                } अञ्ञथा सचे (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_वृक्षः();
                } अञ्ञथा सचे (V3AssertCommon::check_Sva_मानम्_Limit(nodep, constp, "$past tick value")) {
                    सचे (constp->toSInt() > 10) {
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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ध्रुव AstConst* ध्रुव minConstp = VN_CAST(nodep->countp(), Const);
            सचे (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } अञ्ञथा सचे (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } अञ्ञथा सचे (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            सचे (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ध्रुव AstConst* ध्रुव maxConstp = VN_CAST(nodep->maxCountp(), Const);
                सचे (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } अञ्ञथा सचे (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } अञ्ञथा सचे (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_ग्रन्थिः_व्यञ्जकम्* propp = nodep->propp();
            सचे (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            सचे (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ध्रुव तार्किक loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ध्रुव तार्किक hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            सचे (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_ग्रन्थिः* ध्रुव boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                पटिनिवत्त;
            }
            सचे (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                पटिनिवत्त;
            }
            ध्रुव AstConst* ध्रुव loConstp = VN_CAST(nodep->loBoundp(), Const);
            ध्रुव AstConst* ध्रुव hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            सचे (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            सचे (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            सचे (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } अञ्ञथा सचे (loConstp) {
                V3AssertCommon::check_Sva_मानम्_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            सचे (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_मानम्_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            सचे (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            तार्किक hasPropertyOp = propp->isMultiCycleSva();
            सचे (!hasPropertyOp) {
                propp->foreach([&](ध्रुव Ast_ग्रन्थिः* np) {
                    सचे (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = सच्च;
                    }
                });
            }
            सचे (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstRising* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सच्च);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstRose* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सच्च);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    शून्य visit(AstSampled* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सच्च);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    शून्य visit(AstSetuphold* nodep) override {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        सचे (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        सचे (nodep->deldatap()) {
            सचे (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } अञ्ञथा {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        सचे (!newp) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            पटिनिवत्त;
        }
        nodep->replaceWith(नूतन AstAlways{newp});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव evp,
                                         Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव delp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = delp->clone_वृक्षः_Pure(मिच्छा);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = evp->clone_वृक्षः_Pure(मिच्छा);
        UASSERT_OBJ(VN_IS(lhsp, ग्रन्थिः_Var_Ref) || VN_IS(lhsp, ग्रन्थिः_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        सचे (Ast_ग्रन्थिः_Var_Ref* varRefp = VN_CAST(lhsp, ग्रन्थिः_Var_Ref)) {
            सचे (varRefp->varp()->direction() == VDirection::निवेशः) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
                पटिनिवत्त nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        सचे (Ast_ग्रन्थिः_Pre_Sel* selp = VN_CAST(lhsp, ग्रन्थिः_Pre_Sel)) {
            सचे (Ast_ग्रन्थिः_Var_Ref* varRefp = VN_CAST(selp->fromp(), ग्रन्थिः_Var_Ref)) {
                सचे (varRefp->varp()->direction() == VDirection::निवेशः) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
                    पटिनिवत्त nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        पटिनिवत्त नूतन AstAssignW{flp, lhsp, rhsp};
    }

    शून्य visit(AstStable* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सच्च);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSteady* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सच्च);
            nodep->dtypeSetBit();
        }
    }

    शून्य visit(Ast_Stmt_व्यञ्जकम्* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    शून्य visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    शून्य visit(AstUntil* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            सचे (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            सचे (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    शून्य visit(AstRand* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            सचे (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } अञ्ञथा {
                nodep->dtypeSetInt();  // Says the spec
            }
            सचे (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    शून्य visit(AstSEventually* nodep) override {
        सचे (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::Widthed_मानम्{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }
        सचे (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSGotoRep* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            सचे (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } अञ्ञथा {
                width_Check_Rep_गणनम्(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSNonConsRep* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            सचे (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } अञ्ञथा {
                width_Check_Rep_गणनम्(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    शून्य width_Check_Rep_गणनम्(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->countp(), Const);
        सचे (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } अञ्ञथा सचे (constp->dtypep()->isSigned() && constp->अङ्कः().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } अञ्ञथा सचे (constp->अङ्कः().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } अञ्ञथा सचे (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    शून्य widthCheckGotoRepRange(T_Rep* nodep, ध्रुव वर्ण* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ध्रुव AstConst* ध्रुव minConstp = VN_CAST(nodep->countp(), Const);
        ध्रुव AstConst* ध्रुव maxConstp = VN_CAST(nodep->maxCountp(), Const);
        सचे (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            पटिनिवत्त;
        }
        सचे (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            पटिनिवत्त;
        }
        सचे (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            पटिनिवत्त;
        }
        सचे (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    शून्य visit(AstSThroughout* nodep) override {
        m_hasSExpr = सच्च;
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = मिच्छा;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = सच्च;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = सच्च;
        m_hasSExpr = सच्च;
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            सचे (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = सच्च;
        m_hasSExpr = सच्च;
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            अत्थाय (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstURandomRange* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    शून्य visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        सचे (VN_IS(nodep->backp(), IsUnbounded)) पटिनिवत्त;  // Ok, leave
        सचे (VN_IS(nodep->backp(), BracketArrayDType)) पटिनिवत्त;  // Ok, leave
        सचे (VN_IS(nodep->backp(), InsideRange)) पटिनिवत्त;  // Ok, leave
        सचे (ध्रुव auto* ध्रुव varp = VN_CAST(nodep->backp(), Var)) {
            सचे (varp->isParam()) पटिनिवत्त;  // Ok, leave
        }
        Ast_ग्रन्थिः* backp = nodep->backp();
        सचे (VN_IS(backp, Sub)) backp = backp->backp();
        सचे (ध्रुव auto* ध्रुव selp = VN_CAST(backp, SelExtract)) {
            सचे (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) पटिनिवत्त;
        }
        सचे (ध्रुव auto* ध्रुव selp = VN_CAST(backp, SelBit)) {
            सचे (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) पटिनिवत्त;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    शून्य visit(AstInferredDisable* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) nodep->dtypeSetBit();
    }
    शून्य visit(AstIsUnbounded* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
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
        सचे (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        सचे (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            सचे (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    शून्य visit(AstCLog2* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    शून्य widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = nodep->valuep();
        ध्रुव std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
            सचे (!constp->अङ्कः().isNegative()) पटिनिवत्त;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } अञ्ञथा सचे (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } अञ्ञथा {
            पटिनिवत्त;
        }
        // Continue with the default weight
        valuep->replaceWith(नूतन AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    शून्य widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    शून्य visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ध्रुव cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        सचे (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            पटिनिवत्त;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        सचे (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            पटिनिवत्त;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        सचे (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            सचे (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } अञ्ञथा {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } अञ्ञथा सचे (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = nodep->valuep();
            सचे (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(नूतन AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            }
            पटिनिवत्त;
        } अञ्ञथा सचे (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            पटिनिवत्त;
        } अञ्ञथा सचे (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->valuep(), Const);
            सचे (!constp || !constp->अङ्कः().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstCoverOption* nodep) override {
        सचे (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            पटिनिवत्त;
        }
        सचे (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            पटिनिवत्त;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        सचे (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    शून्य visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        सचे (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    शून्य visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        सचे (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    शून्य widthCovergroupRanges(Ast_ग्रन्थिः* rangesp, पूर्णाङ्क fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ध्रुव auto fill = [&](Ast_ग्रन्थिः* nodep) {
            सचे (!fillWidth) पटिनिवत्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* exprp = VN_AS(nodep, ग्रन्थिः_व्यञ्जकम्);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        अत्थाय (Ast_ग्रन्थिः *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            सचे (AstInsideRange* ध्रुव rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } अञ्ञथा सचे (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } अञ्ञथा {
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
        सचे (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        सचे (nodep->arraySizep()) {
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
        सचे (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        सचे (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    शून्य visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव countp = nodep->countp()) {
            सचे (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } अञ्ञथा {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    शून्य widthCoverFilter(Ast_ग्रन्थिः_व्यञ्जकम्* filterp, ध्रुव वर्ण* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), ग्रन्थिः_व्यञ्जकम्);
        File_पङ्क्तिः* ध्रुव fl = filterp->fileline();
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव resultp = filterp->dtypep()->skipRefp();
        सचे (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                नूतन AstNeqD{fl, filterp, नूतन AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } अञ्ञथा सचे (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(नूतन AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(filterp), filterp);
        } अञ्ञथा {
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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            सचे (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                पटिनिवत्त;
            }
            checkCvtUS(nodep->lhsp(), मिच्छा);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            सचे (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->अङ्कः().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        सचे (m_vup->final()) {
            Ast_ग्रन्थिः_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_ग्रन्थिः* newp = nullptr;  // No change
            सचे (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = नूतन AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } अञ्ञथा सचे (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = नूतन AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } अञ्ञथा सचे (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = नूतन AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            सचे (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
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
    शून्य visit(Ast_गणनम्_Bits* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ध्रुव पूर्णाङ्क widthMin = V3_सङ्ख्या::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    शून्य visit(Ast_गणनम्_Ones* nodep) override {
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ध्रुव पूर्णाङ्क widthMin = V3_सङ्ख्या::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    शून्य visit(AstCvtPackString* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        सचे (Ast_ग्रन्थिः_DType* ध्रुव dt = nodep->lhsp()->dtypep()) {
            सचे (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    शून्य visit(AstCvtPackedToArray* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    शून्य visit(AstCvtArrayToArray* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    शून्य visit(AstCvtArrayToPacked* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    शून्य visit(AstCvtUnpackToQueue* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    शून्य visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        सचे (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    शून्य visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ध्रुव द्विगुण timePrescale = constp->अङ्कः().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ध्रुव द्विगुण time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य visit(AstEventControl* nodep) override {
        सचे (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }
        सचे (nodep->fileline()->timingOn()) {
            सचे (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                पटिनिवत्त;
            } अञ्ञथा सचे (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } अञ्ञथा {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        सचे (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    स्थिर string typenameExtends(ध्रुव AstClass* classp) {
        // An interface class may extend several, so those are not named
        सचे (!classp || classp->isInterfaceClass()) पटिनिवत्त "";
        अत्थाय (ध्रुव AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            सचे (extendsp->isImplements()) अनुवर्त;
            ध्रुव AstClass* ध्रुव basep = extendsp->classOrNullp();
            पटिनिवत्त basep ? " extends class " + basep->dtypeName(सच्च) : "";
        }
        पटिनिवत्त "";
    }
    शून्य visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        सचे (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        विकल्प (nodep->attrType()) {
        पक्ष VAttrType::DIM_DIMENSIONS:
        पक्ष VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ध्रुव std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(सच्च);
            ध्रुव पूर्णाङ्क val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            भङ्ग;
        }
        पक्ष VAttrType::DIM_BITS_OR_सङ्ख्या: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            सचे (VN_IS(nodep->fromp(), ग्रन्थिः_व्यञ्जकम्)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            } अञ्ञथा {
                Ast_ग्रन्थिः* newp = नूतन AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            पटिनिवत्त;
        }
        पक्ष VAttrType::DIM_BITS:
        पक्ष VAttrType::DIM_उच्चः:
        पक्ष VAttrType::DIM_INCREMENT:
        पक्ष VAttrType::DIM_वामः:
        पक्ष VAttrType::DIM_नीचः:
        पक्ष VAttrType::DIM_दक्षिणः:
        पक्ष VAttrType::DIM_SIZE: {
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            सचे (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ध्रुव Ast_ग्रन्थिः_Module* ध्रुव ownModp = v3Global.rootp()->containingModule(dtypep);
                सचे (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    पटिनिवत्त;
                }
            }

            सचे (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                विकल्प (nodep->attrType()) {
                पक्ष VAttrType::DIM_SIZE: {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ग्रन्थिः_व्यञ्जकम्);
                    Ast_ग्रन्थिः* ध्रुव newp
                        = नूतन AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(सच्च);
                    newp->protect(मिच्छा);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    भङ्ग;
                }
                पक्ष VAttrType::DIM_वामः:
                पक्ष VAttrType::DIM_नीचः: {
                    Ast_ग्रन्थिः* ध्रुव newp = नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    भङ्ग;
                }
                पक्ष VAttrType::DIM_दक्षिणः:
                पक्ष VAttrType::DIM_उच्चः: {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ग्रन्थिः_व्यञ्जकम्);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sizep
                        = नूतन AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(सच्च);
                    sizep->protect(मिच्छा);
                    Ast_ग्रन्थिः* ध्रुव newp
                        = नूतन AstSub{nodep->fileline(), sizep,
                                     नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    भङ्ग;
                }
                पक्ष VAttrType::DIM_INCREMENT: {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
                        = नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    भङ्ग;
                }
                पक्ष VAttrType::DIM_BITS: {
                    सचे (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } अञ्ञथा {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    भङ्ग;
                }
                स्वभाव: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } अञ्ञथा {
                ध्रुव std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(सच्च);
                ध्रुव uint32_t msbdim = dimpair.first + dimpair.second;
                सचे (!nodep->dimp() || msbdim < 1) {
                    सचे (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ग्रन्थिः_व्यञ्जकम्);
                        Ast_ग्रन्थिः* ध्रुव newp = नूतन AstShiftL{
                            nodep->fileline(), नूतन AstLenN{nodep->fileline(), fromp},
                            नूतन AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    } अञ्ञथा {
                        Ast_ग्रन्थिः_DType* ध्रुव baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        ध्रुव पूर्णाङ्क dim = 1;
                        AstConst* ध्रुव newp = dimension_मानम्(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    }
                } अञ्ञथा सचे (VN_IS(nodep->dimp(), Const)) {
                    ध्रुव पूर्णाङ्क dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* ध्रुव newp
                        = dimension_मानम्(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                } अञ्ञथा {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ध्रुव tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dimp = nodep->dimp()->unlinkFrBack();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                }
            }
            भङ्ग;
        }
        पक्ष VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            भङ्ग;
        पक्ष VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_ग्रन्थिः_DType* ध्रुव protoDtp = nodep->fromp()->dtypep();
            Ast_ग्रन्थिः_DType* ध्रुव declDtp = [&]() {
                सचे (m_ftaskp->fvarp()) पटिनिवत्त m_ftaskp->fvarp()->dtypep();
                Ast_ग्रन्थिः_DType* ध्रुव voidp = नूतन AstVoidDType{m_ftaskp->fileline()};
                प्रक्षिप_Deletep(voidp);  // Note voidp used past here
                पटिनिवत्त voidp;
            }();
            सचे (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            भङ्ग;
        }
        पक्ष VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(सच्च);
            सचे (ध्रुव AstClassRefDType* ध्रुव classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            भङ्ग;
        }
        पक्ष VAttrType::TYPEID:
            सचे (Ast_ग्रन्थिः_DType* dtypep = VN_CAST(nodep->fromp(), ग्रन्थिः_DType)) {
                nodep->dtypep(dtypep);
            } अञ्ञथा {
                nodep->dtypep(VN_AS(nodep->fromp(), ग्रन्थिः_व्यञ्जकम्)->dtypep());
            }
            भङ्ग;
        पक्ष VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            भङ्ग;
        स्वभाव: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            भङ्ग;
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
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(सच्च);
    }

    शून्य visit(AstCellRef* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;

        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cellExprp = VN_CAST(nodep->cellp(), ग्रन्थिः_व्यञ्जकम्)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_CAST(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } अञ्ञथा {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(सच्च);
    }

    // DTYPES
    शून्य visit(Ast_ग्रन्थिः_Array_DType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed

        सचे (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ध्रुव basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            सचे (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* ध्रुव newp = नूतन AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        सचे (VN_IS(nodep, PackArrayDType)) m_underPackedArray = सच्च;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        सचे (AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            सचे (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(सच्च);
        } अञ्ञथा {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDTypep = nodep->subDTypep()->skipRefp();
            सचे (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ध्रुव पूर्णाङ्क width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            सचे (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ध्रुव Ast_ग्रन्थिः_DType* baseDTypep = nodep->subDTypep();
                याव (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                ध्रुव AstBasicDType* ध्रुव basicp = nodep->basicp();
                सचे (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(सच्च));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstAssocArrayDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->कुञ्चिका_DTypep(iterateEditMoveDTypep(nodep, nodep->कुञ्चिका_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_ग्रन्थिः_DType* ध्रुव childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_ग्रन्थिः* ध्रुव elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_ग्रन्थिः* newp;
        सचे (VN_IS(elementsp, Unbounded)) {
            newp = नूतन AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_वृक्षः(), elementsp);
        } अञ्ञथा सचे (Ast_ग्रन्थिः_DType* ध्रुव keyp = VN_CAST(elementsp, ग्रन्थिः_DType)) {
            newp = नूतन AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } अञ्ञथा {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_पङ्क्तिः* ध्रुव elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, सच्च);
            // Must be expression that is constant, but we'll determine that later
            newp = नूतन AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                नूतन AstRange{nodep->fileline(), नूतन AstConst(elementsp->fileline(), 0),
                             नूतन AstSub{elementsNewFl, VN_AS(elementsp, ग्रन्थिः_व्यञ्जकम्),
                                        नूतन AstConst(elementsNewFl, 1)},
                             सच्च}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    शून्य visit(AstConstraintRefDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    शून्य visit(AstDynArrayDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstQueueDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        सचे (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_वृक्षः();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstVoidDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstUnsizedArrayDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        सचे (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
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
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstBasicDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        सचे (nodep->generic()) पटिनिवत्त;  // Already perfect
        सचे (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } अञ्ञथा सचे (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } अञ्ञथा सचे (nodep->implicit()) {
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
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstRefDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(सच्च);
        सचे (nodep->typeofp()) {  // type(typeofp_expression)
            सचे (Ast_ग्रन्थिः_DType* typeofDtp = VN_CAST(nodep->typeofp(), ग्रन्थिः_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } अञ्ञथा {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_ग्रन्थिः* ध्रुव typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_वृक्षः(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        सचे (nodep->subDTypep()) {
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
        सचे (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ध्रुव तार्किक inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            सचे (inTemplateModule) {
                nodep->doingWidth(मिच्छा);
                पटिनिवत्त;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(मिच्छा);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    शून्य widthDTypeNameParams(ध्रुव AstTypedef* nodep) {
        ध्रुव VDTypeNameScopes* ध्रुव scopesp = VDTypeNameScopes::currentp();
        सचे (!scopesp) पटिनिवत्त;  // Computing parameters, as by V3Param, so naming as found
        Ast_ग्रन्थिः* scopep = nullptr;
        अत्थाय (ध्रुव Ast_ग्रन्थिः* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            सचे (ध्रुव std::vector<Ast_ग्रन्थिः*>* ध्रुव paramsp = scopesp->paramsp(scopep)) {
                अत्थाय (Ast_ग्रन्थिः* ध्रुव paramp : *paramsp) {
                    सचे (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    शून्य visit(AstTypedef* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        सचे (auto* ध्रुव refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            पटिनिवत्त;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ध्रुव तार्किक declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        सचे (declares) {
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->dtypep();
            सचे (Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } अञ्ञथा सचे (AstEnumDType* ध्रुव edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        सचे (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    शून्य visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        सचे (Ast_ग्रन्थिः_DType* ध्रुव dtp = VN_CAST(nodep->lhsp(), ग्रन्थिः_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } अञ्ञथा {
            सचे (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(नूतन AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_ग्रन्थिः_DType* ध्रुव toDtp = nodep->शिखरम्()->dtypep()->skipRefToEnump();
        Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        ध्रुव auto castable = Ast_ग्रन्थिः::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_ग्रन्थिः* newp;
        सचे (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            पटिनिवत्त;
        } अञ्ञथा सचे (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ध्रुव enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव testp = enumTestValid(nodep->fromp(), enumDtp);
            File_पङ्क्तिः* ध्रुव fl_novalue = नूतन File_पङ्क्तिः{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, सच्च);
            newp = नूतन AstCond{
                fl, testp,
                नूतन Ast_व्यञ्जकम्_Stmt{fl,
                                नूतन AstAssign{fl_novalue, nodep->शिखरम्()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                नूतन AstConst{fl, AstConst::Signed32{}, 1}},
                नूतन AstConst{fl, AstConst::Signed32{}, 0}};
        } अञ्ञथा सचे ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = नूतन Ast_व्यञ्जकम्_Stmt{
                fl,
                नूतन AstAssign{fl, nodep->शिखरम्()->unlinkFrBack(),
                              नूतन AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                नूतन AstConst{fl, AstConst::Signed32{}, 1}};
        } अञ्ञथा सचे (castable == VCastable::INCOMPATIBLE) {
            newp = नूतन AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } अञ्ञथा {
            newp = नूतन AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    शून्य visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        सचे (AstConst* ध्रुव constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } अञ्ञथा सचे (Ast_ग्रन्थिः_DType* ध्रुव refp = VN_CAST(nodep->dtp(), ग्रन्थिः_DType)) {
            refp->unlinkFrBack();
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } अञ्ञथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य visit(AstCast* nodep) override {
        सचे (nodep->didWidth()) पटिनिवत्त;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        सचे (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            सचे (AstSigned* ध्रुव fromp = VN_CAST(nodep->fromp(), Signed)) {
                सचे (VN_IS(fromp->lhsp(), ग्रन्थिः_Stream)) {
                    Ast_ग्रन्थिः* ध्रुव lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_वृक्षः(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_ग्रन्थिः_DType* ध्रुव toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            ध्रुव auto castable = Ast_ग्रन्थिः::computeCastable(toDtp, fromDtp, nodep->fromp());
            तार्किक bad = मिच्छा;
            सचे (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = सच्च;
            } अञ्ञथा सचे (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } अञ्ञथा सचे (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = सच्च;
            } अञ्ञथा सचे (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = सच्च;
            } अञ्ञथा {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_ग्रन्थिः_व्यञ्जकम्* newp = nullptr;
            सचे (bad) {
            } अञ्ञथा सचे (AstBasicDType* ध्रुव basicp = toDtp->basicp()) {
                सचे (!basicp->isString() && fromDtp->isString()) {
                    newp = नूतन AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } अञ्ञथा सचे (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_ग्रन्थिः_DType* ध्रुव origDTypep = nodep->dtypep();
                    सचे (!VN_IS(fromDtp, StreamDType)) {
                        ध्रुव पूर्णाङ्क width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } अञ्ञथा {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 मिच्छा);
                }
                सचे (newp) {
                } अञ्ञथा सचे (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    सचे (nodep->fromp()->isSigned()) {
                        newp = नूतन AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } अञ्ञथा {
                        newp = नूतन AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } अञ्ञथा सचे (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = नूतन AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } अञ्ञथा सचे (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = नूतन AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } अञ्ञथा सचे (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = नूतन AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } अञ्ञथा {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } अञ्ञथा सचे (VN_IS(toDtp, QueueDType)) {
                सचे (VN_IS(fromDtp, BasicDType)) {
                    newp = नूतन AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } अञ्ञथा सचे (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    पूर्णाङ्क srcElementBits = 1;
                    पूर्णाङ्क dstElementBits = 1;
                    सचे (Ast_ग्रन्थिः_DType* ध्रुव elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ध्रुव AstQueueDType* ध्रुव dstQueueDtp = VN_AS(toDtp, QueueDType);
                    सचे (Ast_ग्रन्थिः_DType* ध्रुव elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = नूतन AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  मिच्छा,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } अञ्ञथा सचे (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } अञ्ञथा {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            सचे (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        सचे (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, मिच्छा, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(सच्च);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(सच्च);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            पूर्णाङ्क width = nodep->rhsp()->toSInt();
            सचे (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        सचे (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_ग्रन्थिः* ध्रुव underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    शून्य visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    शून्य castSized(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* underp, पूर्णाङ्क width) {
        ध्रुव AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        सचे (!underDtp) underDtp = underp->dtypep()->basicp();
        सचे (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ध्रुव पूर्णाङ्क calcWidth = std::max(width, underDtp->width());
            Ast_ग्रन्थिः_DType* ध्रुव calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         मिच्छा, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_ग्रन्थिः_DType* ध्रुव outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), outDtp, EXTEND_EXP,
                            मिच्छा);
            VL_DANGLING(underp);
        }
    }
    शून्य visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(Ast_Constraint_व्यञ्जकम्* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    शून्य visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        सचे (nodep->didWidth()) पटिनिवत्त;
        सचे (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            प्रक्षिप_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(सच्च);
            पटिनिवत्त;
        }
        nodep->doingWidth(सच्च);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        सचे (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.प्रक्षिप_back(nodep);
        सचे (nodep->attrsp()) {
            nodep->attrsp()->foreach([एतत्, nodep](AstAttrOf* attrp) {
                सचे (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    सचे (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), ग्रन्थिः_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(प्रक्षिप_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        सचे (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_ग्रन्थिः_DType* dtp = nodep->dtypep();
            Ast_ग्रन्थिः_DType* np = nullptr;
            याव (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                सचे (ध्रुव AstDynArrayDType* ध्रुव unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    सचे (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } अञ्ञथा {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ध्रुव newp
                        = नूतन AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    सचे (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } अञ्ञथा {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } अञ्ञथा {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        सचे (AstWildcardArrayDType* ध्रुव wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        सचे (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(सच्च);
        // Parameters if implicit untyped inherit from what they are assigned to
        ध्रुव AstBasicDType* ध्रुव bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        तार्किक didchk = मिच्छा;
        ध्रुव तार्किक implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        सचे (implicitParam) {
            सचे (nodep->valuep()) {
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
                सचे (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } अञ्ञथा सचे (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } अञ्ञथा {
                    पूर्णाङ्क width = 0;
                    Ast_ग्रन्थिः_DType* ध्रुव मानम्_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(मानम्_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ध्रुव AstBasicDType* ध्रुव मानम्_Bdtypep = मानम्_DTypep->basicp();
                    तार्किक issigned = मिच्छा;
                    सचे (bdtypep->isNosign()) {
                        सचे (मानम्_Bdtypep && मानम्_Bdtypep->isSigned()) issigned = सच्च;
                    } अञ्ञथा {
                        issigned = bdtypep->isSigned();
                    }
                    सचे (मानम्_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        सचे (ध्रुव auto* ध्रुव constp = VN_CAST(nodep->valuep(), Const)) {
                            सचे (constp->अङ्कः().isString()) {
                                width = constp->अङ्कः().toString().length() * 8;
                            }
                        }
                        सचे (width < 8) width = 8;
                    } अञ्ञथा सचे (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } अञ्ञथा {
                        सचे (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ध्रुव तार्किक is_मानम्_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    सचे (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_मानम्_Fourstate));
                    } अञ्ञथा {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_मानम्_Fourstate);
                    }
                    didchk = सच्च;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } अञ्ञथा {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } अञ्ञथा सचे (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        सचे (nodep->isNet()) {
            Ast_ग्रन्थिः_DType* ध्रुव badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            सचे (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        सचे (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(सच्च);
        nodep->doingWidth(मिच्छा);
    }
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        सचे (nodep->didWidth()) पटिनिवत्त;
        सचे (!nodep->varp()) {
            सचे (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, मिच्छा,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                पटिनिवत्त;
            } अञ्ञथा {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        सचे (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        सचे (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            पटिनिवत्त;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        सचे (VN_IS(nodep->backp(), ग्रन्थिः_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        सचे (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(सच्च);
        सचे (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } अञ्ञथा सचे (nodep->access().isWriteOrRW() && nodep->varp()->is_निवेशः()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ध्रुव तार्किक hierRef = !m_curModVars.गणनम्(nodep->varp());
            ध्रुव तार्किक netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            ध्रुव तार्किक contAssign = VN_IS(nodep->abovep(), AssignW);
            ध्रुव तार्किक pinConn = VN_IS(nodep->abovep(), Pin);
            सचे (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } अञ्ञथा सचे (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        सचे (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(सच्च);
    }

    शून्य visit(AstEnumDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_ग्रन्थिः_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_ग्रन्थिः_DType* ध्रुव badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        सचे (badDtp) {
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
        V3_सङ्ख्या अङ्कः(nodep, nodep->width(), 0);
        ध्रुव V3_सङ्ख्या one{nodep, nodep->width(), 1};
        तार्किक wrapAround = मिच्छा;
        std::map<ध्रुव V3_सङ्ख्या, AstEnumItem*> inits;
        अत्थाय (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            सचे (itemp->valuep()) {
                सचे (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_वृक्षः("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                सचे (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_वृक्षः();
                    अनुवर्त;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = मिच्छा;  // Explicit value resets wrap-around tracking
            }
            सचे (!itemp->valuep()) {
                सचे (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                सचे (अङ्कः.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(नूतन AstConst{itemp->fileline(), अङ्कः});
            }

            ध्रुव AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
            सचे (constp->अङ्कः().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            अङ्कः.opAssign(constp->अङ्कः());
            // Look for duplicates
            ध्रुव auto pair = inits.emplace(अङ्कः, itemp);
            सचे (!pair.second) {  // IEEE says illegal
                ध्रुव Ast_ग्रन्थिः* ध्रुव otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ध्रुव तार्किक prevNeg = constp->अङ्कः().isNegative();
            अङ्कः.opAdd(one, constp->अङ्कः());
            सचे (basicp->isSigned()) {
                wrapAround = !prevNeg && अङ्कः.isNegative();
            } अञ्ञथा {
                wrapAround = अङ्कः.isEqZero();
            }
        }
    }
    शून्य visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        सचे (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            तार्किक warnOn = सच्च;
            Ast_ग्रन्थिः_व्यञ्जकम्* valuep = nodep->valuep();
            सचे (ध्रुव AstAdd* ध्रुव anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                सचे (anodep->fileline()->equal_First_पङ्क्तिः_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
                सचे (static_cast<पूर्णाङ्क>(constp->अङ्कः().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = मिच्छा;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(नूतन AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        सचे (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_ग्रन्थिः* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            अत्थाय (; enump; enump = enump->backp()) {
                सचे (VN_IS(enump, EnumDType)) भङ्ग;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    शून्य visit(Ast_ग्रन्थिः_Cons* nodep) override {}  // Widthed when created
    शून्य visit(Ast_ग्रन्थिः_Cons_Item* nodep) override {}  // Widthed when created
    शून्य visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        अत्थाय (Ast_ग्रन्थिः *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ग्रन्थिः_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        सचे (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } अञ्ञथा सचे (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } अञ्ञथा {
            // Take width as maximum across all items
            पूर्णाङ्क width = nodep->exprp()->width();
            पूर्णाङ्क mwidth = nodep->exprp()->widthMin();
            तार्किक isFourstate = nodep->exprp()->dtypep()->isFourstate();
            अत्थाय (ध्रुव Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        अत्थाय (Ast_ग्रन्थिः *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            सचे (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } अञ्ञथा {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        सचे (m_constraintp) {
            तार्किक canLower = सच्च;
            अत्थाय (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                सचे (VN_IS(ditemp->rangep(), InsideRange)) अनुवर्त;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                सचे (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = मिच्छा;
                    भङ्ग;
                }
            }
            सचे (canLower) पटिनिवत्त;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = nullptr;
        अत्थाय (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* inewp
                = insideItem(nodep, nodep->exprp()->clone_वृक्षः_Pure(मिच्छा), itemp->rangep());
            सचे (!inewp) अनुवर्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cmpp
                = नूतन AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            नूतन AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, सच्च);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, सच्च);
            inewp = नूतन AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? नूतन AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        सचे (!newp) newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        अत्थाय (Ast_ग्रन्थिः *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ग्रन्थिः_DType* expDTypep = nullptr;

        सचे (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } अञ्ञथा सचे (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } अञ्ञथा {
            // Take width as maximum across all items
            पूर्णाङ्क width = nodep->exprp()->width();
            पूर्णाङ्क mwidth = nodep->exprp()->widthMin();
            तार्किक isFourstate = nodep->exprp()->dtypep()->isFourstate();
            अत्थाय (ध्रुव Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            ध्रुव VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        अत्थाय (Ast_ग्रन्थिः *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            सचे (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } अञ्ञथा सचे (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* exprp;
        Ast_व्यञ्जकम्_Stmt* व्यञ्जकम्_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        सचे (!m_constraintp && !nodep->exprp()->isPure()) {
            File_पङ्क्तिः* ध्रुव fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_ग्रन्थिः_DType* ध्रुव व्यञ्जकम्_Dtp = nodep->exprp()->dtypep();
            ध्रुव पूर्णाङ्क w = व्यञ्जकम्_Dtp->width();
            Ast_ग्रन्थिः_DType* ध्रुव tempDTypep
                = व्यञ्जकम्_Dtp->widthSized() ? व्यञ्जकम्_Dtp
                                        : nodep->findBitOrLogicDType(w, w, व्यञ्जकम्_Dtp->numeric(),
                                                                     व्यञ्जकम्_Dtp->isFourstate());
            AstVar* ध्रुव varp
                = नूतन AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = नूतन AstVarRef{fl, varp, VAccess::READ};
            व्यञ्जकम्_Stmtp = नूतन Ast_व्यञ्जकम्_Stmt{fl,
                                        नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            सचे (m_ftaskp) {
                varp->funcLocal(सच्च);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } अञ्ञथा {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(व्यञ्जकम्_Stmtp);
        } अञ्ञथा {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = nullptr;
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम् *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), ग्रन्थिः_व्यञ्जकम्);  // Will be unlinking
            Ast_ग्रन्थिः_व्यञ्जकम्* inewp;
            सचे (व्यञ्जकम्_Stmtp) {
                inewp = insideItem(nodep, व्यञ्जकम्_Stmtp, itemp);
                व्यञ्जकम्_Stmtp = nullptr;
            } अञ्ञथा {
                inewp = insideItem(nodep, exprp->clone_वृक्षः_Pure(मिच्छा), itemp);
            }
            सचे (!inewp) अनुवर्त;
            newp = newp ? नूतन AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        सचे (व्यञ्जकम्_Stmtp) VL_DO_DANGLING(व्यञ्जकम्_Stmtp->delete_वृक्षः(), व्यञ्जकम्_Stmtp);
        सचे (!newp) newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* insideItem(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* itemp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव itemDtp = itemp->dtypep()->skipRefp();
        सचे (AstInsideRange* ध्रुव irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            पटिनिवत्त irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } अञ्ञथा सचे (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव inewp = नूतन AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(सच्च);
            पटिनिवत्त inewp;
        } अञ्ञथा सचे (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_वृक्षः(), exprp);
            पटिनिवत्त nullptr;
        }
        पटिनिवत्त AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    शून्य visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        सचे (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } अञ्ञथा {
            सचे (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            सचे (m_vup->final()) {
                Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ध्रुव पूर्णाङ्क expWidth = expDTypep->width();
                ध्रुव तार्किक waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                ध्रुव तार्किक waiveRhs = expWidth == 32
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
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        सचे (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_UOr_Struct_DType* nodep) override {
        सचे (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(मिच्छा);
            पटिनिवत्त;
        }
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(सच्च);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        सचे (ध्रुव AstUnionDType* ध्रुव unionp = VN_CAST(nodep, UnionDType)) {
            सचे (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(मिच्छा);
        // Error checks
        अत्थाय (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ग्रन्थिः_DType* ध्रुव dtp = itemp->subDTypep()->skipRefp();
            सचे (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            सचे ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                प्रक्षिप_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        ध्रुव तार्किक isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ध्रुव तार्किक inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        सचे (VN_IS(nodep, UnionDType) || nodep->packed()) {
            पूर्णाङ्क lsb = 0;
            पूर्णाङ्क width = 0;
            तार्किक first = सच्च;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            अत्थाय (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            अत्थाय (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                सचे (itemp->isFourstate()) nodep->isFourstate(सच्च);
                itemp->lsb(lsb);
                सचे (VN_IS(nodep, UnionDType)) {
                    ध्रुव पूर्णाङ्क itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    सचे (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } अञ्ञथा {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = मिच्छा;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } अञ्ञथा {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(मिच्छा);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    शून्य visit(AstThisRef* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        सचे (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    शून्य visit(AstClassRefDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        userIterateChildren(nodep, nullptr);
    }
    शून्य visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstClassExtends* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        सचे (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    शून्य visit(AstMemberDType* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        सचे (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    शून्य visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        सचे (nodep->didWidth()) पटिनिवत्त;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = सच्च;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        ध्रुव AstMemberSel* ध्रुव fromSel = VN_CAST(nodep->fromp(), MemberSel);
        सचे (AstClocking* ध्रुव clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            सचे (AstVar* ध्रुव varp = memberSelClocking(nodep, clockingp)) {
                सचे (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_वृक्षः(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                सचे (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                सचे (AstIfaceRefDType* ध्रुव adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(सच्च);  // Must not visit again: will confuse scopes
                पटिनिवत्त;
            }
        } अञ्ञथा सचे (Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव adtypep = VN_CAST(fromDtp, ग्रन्थिः_UOr_Struct_DType)) {
            सचे (memberSelStruct(nodep, adtypep)) पटिनिवत्त;
        } अञ्ञथा सचे (AstClassRefDType* ध्रुव adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            सचे (memberSelClass(nodep, adtypep)) पटिनिवत्त;
        } अञ्ञथा सचे (AstIfaceRefDType* ध्रुव adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            सचे (Ast_ग्रन्थिः* foundp = memberSelIface(nodep, adtypep)) {
                सचे (AstClocking* ध्रुव clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                सचे (AstVar* ध्रुव varp = VN_CAST(foundp, Var)) {
                    सचे (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* ध्रुव ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep->fromp(), VarRef);
                    सचे (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ध्रुव newp = नूतन AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(सच्च);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                        पटिनिवत्त;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(सच्च);
                    पटिनिवत्त;
                }
                सचे (AstModport* ध्रुव modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ध्रुव newDtypep = नूतन AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त;
                }
                सचे (AstCell* ध्रुव cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    सचे (VN_IS(cellp->modp(), Iface)) {
                        ध्रुव string viftopName = cellp->name() + "__Viftop";
                        Ast_ग्रन्थिः_Module* ध्रुव parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        अत्थाय (Ast_ग्रन्थिः* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            सचे (AstVar* ध्रुव vp = VN_CAST(itemp, Var)) {
                                सचे (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    भङ्ग;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        सचे (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_ग्रन्थिः_DType* subDtypep = viftopVarp->dtypep();
                        सचे (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            सचे (AstIfaceRefDType* ध्रुव subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ध्रुव newDtypep = नूतन AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(सच्च);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(सच्च);
                        पटिनिवत्त;
                    }
                }
                सचे (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp = VN_CAST(foundp, ग्रन्थिः_FTask)) {
                    AstMethodCall* ध्रुव newp = नूतन AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    पटिनिवत्त;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } अञ्ञथा सचे (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            पटिनिवत्त;
        } अञ्ञथा {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    तार्किक memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        सचे (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* ध्रुव newp = नूतन AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            visit(newp);
            पटिनिवत्त सच्च;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ध्रुव first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        अत्थाय (AstClass* classp = first_classp; classp;) {
            सचे (Ast_ग्रन्थिः* ध्रुव foundp = m_memberMap.findMember(classp, nodep->name())) {
                सचे (AstVar* ध्रुव varp = VN_CAST(foundp, Var)) {
                    सचे (!varp->didWidth()) userIterate(varp, nullptr);
                    सचे (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ध्रुव varRefp
                            = नूतन AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                        पटिनिवत्त सच्च;
                    }
                    सचे (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(सच्च);
                    सचे (nodep->fromp()->same_वृक्षः(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    पटिनिवत्त सच्च;
                }
                सचे (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(नूतन AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त सच्च;
                }
                सचे (AstEnumItemRef* ध्रुव adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_वृक्षः(मिच्छा));
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त सच्च;
                }
                सचे (Ast_ग्रन्थिः_FTask* ftaskp = VN_CAST(foundp, ग्रन्थिः_FTask)) {
                    AstMethodCall* newp = नूतन AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त सच्च;
                }
                सचे (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    पटिनिवत्त सच्च;
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
        अत्थाय (AstClass* classp = first_classp; classp;) {
            अत्थाय (Ast_ग्रन्थिः* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                सचे (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.प्रक्षिप_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        ध्रुव string suggest = speller.best_Candidate_सन्देशः(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        पटिनिवत्त मिच्छा;  // Caller handles error
    }
    तार्किक modportAllowsItem(Ast_ग्रन्थिः* nodep, ध्रुव AstIfaceRefDType* adtypep, ध्रुव Ast_ग्रन्थिः* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ध्रुव AstModport* ध्रुव modportp = adtypep->modportp();
        सचे (!modportp) पटिनिवत्त सच्च;
        सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(itemp, Var)) {
            सचे (varp->isParam()) पटिनिवत्त सच्च;
        } अञ्ञथा सचे (!VN_IS(itemp, ग्रन्थिः_FTask) && !VN_IS(itemp, Clocking)) {
            पटिनिवत्त सच्च;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        सचे (pair.second) {
            अत्थाय (ध्रुव Ast_ग्रन्थिः* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.निवेशय(mpItemp->name());
            }
        }
        सचे (pair.first->second.गणनम्(itemp->name())) पटिनिवत्त सच्च;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        पटिनिवत्त मिच्छा;
    }
    Ast_ग्रन्थिः* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_ग्रन्थिः_Module* ध्रुव ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        अत्थाय (Ast_ग्रन्थिः* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            सचे (itemp->name() == nodep->name()) {
                सचे (!modportAllowsItem(nodep, adtypep, itemp)) {
                    पटिनिवत्त nullptr;  // Caller handles error
                }
                पटिनिवत्त itemp;
            }
            सचे (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.प्रक्षिप_Candidate(itemp->prettyName());
            }
        }
        ध्रुव string suggest = speller.best_Candidate_सन्देशः(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        पटिनिवत्त nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        अत्थाय (Ast_ग्रन्थिः* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            सचे (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                सचे (citemp->varp()->name() == nodep->name()) पटिनिवत्त citemp->varp();
                speller.प्रक्षिप_Candidate(citemp->varp()->prettyName());
            }
        }
        ध्रुव string suggest = speller.best_Candidate_सन्देशः(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        पटिनिवत्त nullptr;  // Caller handles error
    }
    तार्किक memberSelStruct(AstMemberSel* nodep, Ast_ग्रन्थिः_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        सचे (AstMemberDType* ध्रुव memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            सचे (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } अञ्ञथा सचे (adtypep->packed()) {
                AstSel* ध्रुव newp = नूतन AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(सच्च);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } अञ्ञथा {
                AstStructSel* ध्रुव newp = नूतन AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(सच्च);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            पटिनिवत्त सच्च;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        पटिनिवत्त मिच्छा;
    }

    शून्य visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    शून्य visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        सचे (nodep->didWidth()) पटिनिवत्त;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* ध्रुव basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        सचे (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } अञ्ञथा सचे (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } अञ्ञथा सचे (AstEnumDType* ध्रुव adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } अञ्ञथा सचे (AstAssocArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } अञ्ञथा सचे (AstWildcardArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } अञ्ञथा सचे (AstDynArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } अञ्ञथा सचे (AstQueueDType* ध्रुव adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } अञ्ञथा सचे (AstClassRefDType* ध्रुव adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } अञ्ञथा सचे (AstIfaceRefDType* ध्रुव adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } अञ्ञथा सचे (AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } अञ्ञथा सचे (AstConstraintRefDType* ध्रुव adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } अञ्ञथा सचे (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } अञ्ञथा सचे (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } अञ्ञथा {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_ग्रन्थिः_FTask_Ref* nodep, तार्किक required, तार्किक arbReturn,
                              Ast_ग्रन्थिः_DType* returnDtp, Ast_ग्रन्थिः_DType* indexDtp,
                              Ast_ग्रन्थिः_DType* मानम्_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        सचे (AstWith* ध्रुव withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->मानम्_Arg_Refp()->dtypep(मानम्_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            पटिनिवत्त withp;
        }
        सचे (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        पटिनिवत्त nullptr;
    }
    शून्य methodOkArguments(Ast_ग्रन्थिः_FTask_Ref* nodep, पूर्णाङ्क minArg, पूर्णाङ्क maxArg) {
        पूर्णाङ्क narg = 0;
        सचे (AstWith* ध्रुव withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(प्रक्षिप_Deletep(withp->unlinkFrBack()), withp);
        }
        अत्थाय (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ध्रुव तार्किक ok = (narg >= minArg) && (narg <= maxArg);
        सचे (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            अत्थाय (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    नूतन AstArg{nodep->fileline(), "",
                               नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            अत्थाय (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                याव (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_वृक्षः(), argp);
            }
        }
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* methodArg(AstMethodCall* nodep, पूर्णाङ्क arg) {
        AstArg* argp = nodep->argsp();
        अत्थाय (पूर्णाङ्क narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        पटिनिवत्त argp->exprp();
    }

    शून्य methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        सचे (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            सचे (nodep->name() == "num") {
                पूर्णाङ्क वस्तूनि = 0;
                अत्थाय (Ast_ग्रन्थिः* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++वस्तूनि;
                newp = नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, वस्तूनि);
            } अञ्ञथा सचे (nodep->name() == "first") {
                ध्रुव AstEnumItem* itemp = adtypep->itemsp();
                सचे (!itemp) {
                    newp = नूतन AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } अञ्ञथा {
                    newp = VN_AS(itemp->valuep()->clone_वृक्षः(मिच्छा), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } अञ्ञथा सचे (nodep->name() == "last") {
                ध्रुव AstEnumItem* itemp = adtypep->itemsp();
                याव (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                सचे (!itemp) {
                    newp = नूतन AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } अञ्ञथा {
                    newp = VN_AS(itemp->valuep()->clone_वृक्षः(मिच्छा), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अञ्ञथा सचे (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            सचे (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } अञ्ञथा सचे (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } अञ्ञथा सचे (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } अञ्ञथा {
                nodep->v3fatalSrc("Bad case");
            }

            सचे (nodep->name() != "name" && nodep->argsp()) {
                Ast_ग्रन्थिः_व्यञ्जकम्* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, सच्च);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                सचे (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ध्रुव uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                सचे (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* ध्रुव newp = नूतन AstMethodCall{
                        nodep->fileline(),
                        नूतन AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    पटिनिवत्त;
                } अञ्ञथा सचे (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    प्रक्षिप_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* ध्रुव clonep = nodep->clone_वृक्षः(मिच्छा);
                    VN_AS(stepp, Const)->अङ्कः().setLong(1);
                    AstConst* ध्रुव constp = नूतन AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* ध्रुव argp = नूतन AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* ध्रुव newp
                        = नूतन AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    पटिनिवत्त;
                }
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    शून्य methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        सचे (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } अञ्ञथा सचे (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } अञ्ञथा सचे (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव index_exprp = method_Call_Wildcard_Index_व्यञ्जकम्(nodep, adtypep);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } अञ्ञथा सचे (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            सचे (!nodep->argsp()) {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } अञ्ञथा {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव index_exprp = method_Call_Wildcard_Index_व्यञ्जकम्(nodep, adtypep);
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } अञ्ञथा सचे (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } अञ्ञथा सचे (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ध्रुव withp
                = methodWithClause(nodep, मिच्छा, मिच्छा, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ध्रुव withp
                = methodWithClause(nodep, सच्च, मिच्छा, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "map") {
            AstWith* ध्रुव withp
                = methodWithClause(nodep, सच्च, मिच्छा, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        सचे (newp) {
            newp->protect(मिच्छा);
            newp->didWidth(सच्च);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    शून्य methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        सचे (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        सचे (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } अञ्ञथा सचे (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->कुञ्चिका_DTypep(), BOTH);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव index_exprp = method_Call_Assoc_Index_व्यञ्जकम्(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->कुञ्चिका_DTypep(), BOTH);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव index_exprp = method_Call_Assoc_Index_व्यञ्जकम्(nodep, adtypep);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } अञ्ञथा सचे (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            सचे (!nodep->argsp()) {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } अञ्ञथा {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->कुञ्चिका_DTypep(),
                                  BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव index_exprp = method_Call_Assoc_Index_व्यञ्जकम्(nodep, adtypep);
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } अञ्ञथा सचे (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } अञ्ञथा सचे (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ध्रुव withp = methodWithClause(nodep, मिच्छा, मिच्छा, adtypep->subDTypep(),
                                                    adtypep->कुञ्चिका_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ध्रुव withp = methodWithClause(
                nodep, मिच्छा, सच्च, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            सचे (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->कुञ्चिका_DTypep()));
            } अञ्ञथा {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ध्रुव withp = methodWithClause(nodep, सच्च, मिच्छा, nodep->findBitDType(),
                                                    adtypep->कुञ्चिका_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ध्रुव withp = methodWithClause(nodep, सच्च, मिच्छा, nodep->findBitDType(),
                                                    adtypep->कुञ्चिका_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->कुञ्चिका_DTypep()));
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "map") {
            AstWith* ध्रुव withp = methodWithClause(nodep, सच्च, मिच्छा, adtypep->subDTypep(),
                                                    adtypep->कुञ्चिका_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        सचे (newp) {
            newp->protect(मिच्छा);
            newp->didWidth(सच्च);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* method_Call_Assoc_Index_व्यञ्जकम्(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_ग्रन्थिः* ध्रुव index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->कुञ्चिका_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        पटिनिवत्त nodep->argsp()->exprp();
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* method_Call_Wildcard_Index_व्यञ्जकम्(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_ग्रन्थिः* ध्रुव index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        पटिनिवत्त nodep->argsp()->exprp();
    }
    शून्य methodCallLValueRecurse(AstMethodCall* nodep, Ast_ग्रन्थिः* childp, ध्रुव VAccess& access) {
        सचे (AstCMethodHard* ध्रुव ichildp = VN_CAST(childp, CMethodHard)) {
            तार्किक methAt = मिच्छा;
            तार्किक methAtBack = मिच्छा;
            तार्किक methAtWrite = मिच्छा;
            विकल्प (ichildp->method()) {
            पक्ष VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = सच्च;
                भङ्ग;
            पक्ष VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = सच्च;
                भङ्ग;
            पक्ष VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            पक्ष VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            पक्ष VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = सच्च;
                भङ्ग;
            स्वभाव: भङ्ग;
            }
            सचे (methAt || methAtBack || methAtWrite) {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                सचे (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    सचे (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } अञ्ञथा सचे (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                पटिनिवत्त;
            }
        }
        सचे (Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(childp, ग्रन्थिः_Var_Ref)) {
            varrefp->access(access);
        } अञ्ञथा सचे (ध्रुव AstMemberSel* ध्रुव ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } अञ्ञथा सचे (ध्रुव AstStructSel* ध्रुव ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } अञ्ञथा सचे (ध्रुव Ast_ग्रन्थिः_Sel* ध्रुव ichildp = VN_CAST(childp, ग्रन्थिः_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } अञ्ञथा सचे (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } अञ्ञथा {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_ग्रन्थिः_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        सचे (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            सचे (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, मिच्छा, सच्च, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ध्रुव withp = methodWithClause(
                nodep, मिच्छा, सच्च, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            सचे (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } अञ्ञथा {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ध्रुव withp
                = methodWithClause(nodep, सच्च, मिच्छा, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ध्रुव withp
                = methodWithClause(nodep, सच्च, मिच्छा, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ध्रुव withp
                = methodWithClause(nodep, सच्च, मिच्छा, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        पटिनिवत्त newp;
    }
    शून्य methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        सचे (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } अञ्ञथा सचे (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } अञ्ञथा सचे (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } अञ्ञथा सचे (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ध्रुव withp
                = methodWithClause(nodep, मिच्छा, मिच्छा, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे ((newp = methodCallArray(nodep, adtypep))) {
        } अञ्ञथा {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        सचे (newp) {
            newp->protect(मिच्छा);
            newp->didWidth(सच्च);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    शून्य methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        सचे (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अञ्ञथा सचे (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अञ्ञथा सचे (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } अञ्ञथा सचे (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            सचे (!nodep->argsp()) {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } अञ्ञथा {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव index_exprp = method_Call_Queue_Index_व्यञ्जकम्(nodep);
                सचे (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_निष्कासय_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } अञ्ञथा {
                    newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } अञ्ञथा सचे (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव index_exprp = method_Call_Queue_Index_व्यञ्जकम्(nodep);
            AstArg* ध्रुव argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            सचे (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_प्रक्षिप_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } अञ्ञथा {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_निवेशय, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } अञ्ञथा सचे (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अञ्ञथा सचे (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ध्रुव argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ध्रुव withp
                = methodWithClause(nodep, मिच्छा, मिच्छा, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अञ्ञथा सचे ((newp = methodCallArray(nodep, adtypep))) {
        } अञ्ञथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        सचे (newp) {
            newp->protect(मिच्छा);
            newp->didWidth(सच्च);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* method_Call_Queue_Index_व्यञ्जकम्(AstMethodCall* nodep) {
        Ast_ग्रन्थिः* ध्रुव index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        पटिनिवत्त nodep->argsp()->exprp();
    }
    शून्य methodCallWarnTiming(Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep, ध्रुव std::string& className) {
        सचे (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } अञ्ञथा सचे (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    शून्य methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ध्रुव ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        सचे (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), ग्रन्थिः_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            सचे (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                पटिनिवत्त;
            }
            सचे (adtypep->isVirtual()) {
                अत्थाय (Ast_ग्रन्थिः* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    सचे (AstVar* ध्रुव mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            सचे (ftaskp->isStatic()) {
                AstArg* ध्रुव argsp = nodep->argsp();
                सचे (argsp) argsp->unlinkFrBackWithNext();
                Ast_ग्रन्थिः_FTask_Ref* newp = nullptr;
                सचे (VN_IS(ftaskp, Task)) {
                    newp = नूतन AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } अञ्ञथा {
                    newp = नूतन AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            } अञ्ञथा {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                सचे (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            पटिनिवत्त;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    शून्य handleRandomizeArgs(Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep, AstClass* ध्रुव classp) {
        तार्किक hasNonNullArgs = मिच्छा;
        AstConst* nullp = nullptr;
        अत्थाय (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_ग्रन्थिः_व्यञ्जकम्* exprp = argp->exprp();
            सचे (AstConst* ध्रुव constp = VN_CAST(exprp, Const)) {
                सचे (constp->अङ्कः().isNull()) {
                    nullp = constp;
                    अनुवर्त;
                }
            }
            hasNonNullArgs = सच्च;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            सचे (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* fromp = methodCallp->fromp();
                याव (AstMemberSel* ध्रुव memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ध्रुव varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            सचे (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्षः(), argp);
                अनुवर्त;
            }
            याव (exprp) {
                सचे (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } अञ्ञथा {
                    सचे (AstVarRef* ध्रुव varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } अञ्ञथा {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्षः(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                सचे (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            सचे (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } अञ्ञथा सचे (classp->existsMember([&](ध्रुव AstClass*, ध्रुव AstVar* memberVarp) {
                           पटिनिवत्त memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } अञ्ञथा {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्षः(), argp);
            }
        }
        सचे (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    शून्य methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ध्रुव first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        सचे (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, मिच्छा, मिच्छा, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            अत्थाय (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                सचे (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } अञ्ञथा सचे (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        अत्थाय (AstClass* classp = first_classp; classp;) {
            सचे (nodep->fileline()->timingOn()) {
                सचे (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ध्रुव packagep = getItemPackage(classp);
                    // Check if it's std
                    सचे (packagep && packagep->name() == "std") {
                        सचे (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } अञ्ञथा सचे (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } अञ्ञथा सचे (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            सचे (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), ग्रन्थिः_FTask)) {
                userIterate(ftaskp, nullptr);
                सचे (ftaskp->isStatic()) {
                    AstArg* ध्रुव argsp = nodep->argsp();
                    सचे (argsp) argsp->unlinkFrBackWithNext();
                    Ast_ग्रन्थिः_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    सचे (!m_vup) {  // Called as task
                        newp = नूतन AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } अञ्ञथा {
                        newp = नूतन AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                } अञ्ञथा {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    सचे (VN_IS(ftaskp, Task)) {
                        सचे (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    सचे (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                पटिनिवत्त;
            } अञ्ञथा सचे (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(सच्च);
                v3Global.useRandomizeMethods(सच्च);
                AstCMethodHard* ध्रुव newp
                    = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(सच्च);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            } अञ्ञथा सचे (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(सच्च);
                v3Global.useRandomizeMethods(सच्च);
                AstCMethodHard* ध्रुव newp
                    = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(सच्च);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            } अञ्ञथा सचे (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(सच्च);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            अत्थाय (AstClass* classp = first_classp; classp;) {
                अत्थाय (Ast_ग्रन्थिः* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    सचे (VN_IS(itemp, ग्रन्थिः_FTask)) speller.प्रक्षिप_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            ध्रुव string suggest = speller.best_Candidate_सन्देशः(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    शून्य methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        सचे (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            सचे (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } अञ्ञथा {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(सच्च);
        } अञ्ञथा {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        सचे (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } अञ्ञथा {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(सच्च);
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
        सचे (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } अञ्ञथा सचे (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } अञ्ञथा सचे (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } अञ्ञथा सचे (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } अञ्ञथा सचे (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        सचे (methodId) {
            AstWith* ध्रुव withp
                = methodWithClause(nodep, मिच्छा, मिच्छा, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            सचे (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ध्रुव newp
                    = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                सचे (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(मिच्छा);
                newp->didWidth(सच्च);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                पटिनिवत्त;
            } अञ्ञथा {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                Ast_ग्रन्थिः_व्यञ्जकम्* newp = nullptr;
                अत्थाय (पूर्णाङ्क i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arrayRef = nodep->fromp()->clone_वृक्षः_Pure(मिच्छा);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव selector = नूतन AstArraySel{fl, arrayRef, i};
                    सचे (!newp) {
                        newp = selector;
                    } अञ्ञथा {
                        विकल्प (methodId) {
                        पक्ष ARRAY_OR: newp = नूतन AstOr{fl, newp, selector}; भङ्ग;
                        पक्ष ARRAY_AND: newp = नूतन AstAnd{fl, newp, selector}; भङ्ग;
                        पक्ष ARRAY_XOR: newp = नूतन AstXor{fl, newp, selector}; भङ्ग;
                        पक्ष ARRAY_SUM: newp = नूतन AstAdd{fl, newp, selector}; भङ्ग;
                        पक्ष ARRAY_PRODUCT: newp = नूतन AstMul{fl, newp, selector}; भङ्ग;
                        स्वभाव: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            }
        } अञ्ञथा सचे (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(मिच्छा);
            newp->didWidth(सच्च);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    शून्य methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        सचे (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अञ्ञथा {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    शून्य methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        सचे (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अञ्ञथा सचे (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } अञ्ञथा सचे (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } अञ्ञथा सचे (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } अञ्ञथा सचे (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } अञ्ञथा सचे (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } अञ्ञथा सचे (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा सचे (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा सचे (nodep->name() == "compare" || nodep->name() == "icompare") {
            ध्रुव तार्किक ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ध्रुव argp = nodep->argsp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhs = nodep->fromp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhs = argp->exprp()->unlinkFrBack();
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा सचे (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ध्रुव arg0p = nodep->argsp();
            AstArg* ध्रुव arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ग्रन्थिः_Var_Ref* ध्रुव fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* ध्रुव varrefp
                = नूतन AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstAssign{
                nodep->fileline(), fromp, नूतन AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            प्रक्षिप_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } अञ्ञथा सचे (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ध्रुव arg0p = nodep->argsp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->fromp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा सचे (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ध्रुव arg0p = nodep->argsp();
            AstArg* ध्रुव arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->fromp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thsp = arg1p->exprp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा सचे (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            सचे (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } अञ्ञथा सचे (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } अञ्ञथा सचे (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } अञ्ञथा सचे (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } अञ्ञथा सचे (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } अञ्ञथा {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_ग्रन्थिः_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        सचे (AstQueueDType* ध्रुव queuep = m_queueDTypeIndexed[indexDTypep]) {
            पटिनिवत्त queuep;
        } अञ्ञथा {
            AstQueueDType* ध्रुव newp
                = नूतन AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            पटिनिवत्त newp;
        }
    }

    शून्य visit(AstNew* nodep) override {
        सचे (nodep->didWidth()) पटिनिवत्त;
        AstClass* classp = nullptr;
        तार्किक assign = मिच्छा;
        सचे (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = सच्च;
            Ast_ग्रन्थिः* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            सचे (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                सचे (AstClass* ध्रुव clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ध्रुव adtypep
                        = नूतन AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } अञ्ञथा {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            सचे (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, ग्रन्थिः_DType)
                           ? ("data type "s + VN_AS(warnp, ग्रन्थिः_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                पटिनिवत्त;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            सचे (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } अञ्ञथा {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } अञ्ञथा {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        सचे (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    शून्य visit(AstNewCopy* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        AstClassRefDType* ध्रुव refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        सचे (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            पटिनिवत्त;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        सचे (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    शून्य visit(AstNewDynamic* nodep) override {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        AstDynArrayDType* ध्रुव adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        सचे (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            पटिनिवत्त;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        सचे (!VN_IS(nodep->backp(), ग्रन्थिः_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            पटिनिवत्त;
        }
        nodep->dtypep(adtypep);
        सचे (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        सचे (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ध्रुव consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            सचे (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
    }

    शून्य visit(Ast_Tagged_व्यञ्जकम्* nodep) override {
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
        सचे (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            अत्थाय (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = मिच्छा;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                सचे (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), ग्रन्थिः_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                अत्थाय (Ast_ग्रन्थिः *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            पटिनिवत्त;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = मिच्छा;
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        UINFO(9, "PATTERN " << nodep);
        सचे (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        सचे (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_ग्रन्थिः_DType* dtypep = nodep->dtypep();
        सचे (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            सचे (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                पटिनिवत्त;
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            सचे (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                पटिनिवत्त;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            अत्थाय (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ध्रुव पूर्णाङ्क times = visitPatMemberRep(patp);
                अत्थाय (पूर्णाङ्क i = 1; i < times; i++) {
                    AstPatMember* ध्रुव newp = patp->clone_वृक्षः(मिच्छा);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            अत्थाय (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                सचे (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    याव (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव movep = VN_AS(patp->lhssp()->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_ग्रन्थिः* newkeyp = nullptr;
                        सचे (patp->keyp()) newkeyp = patp->keyp()->clone_वृक्षः(सच्च);
                        AstPatMember* ध्रुव newp
                            = नूतन AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            अत्थाय (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ध्रुव nextp = VN_AS(patp->nextp(), PatMember);
                सचे (patp->isDefault()) {
                    सचे (defaultp) {
                        VL_DO_DANGLING(प्रक्षिप_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            याव (ध्रुव AstConstDType* ध्रुव vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            सचे (auto* ध्रुव vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } अञ्ञथा सचे (auto* ध्रुव vdtypep = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } अञ्ञथा सचे (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ध्रुव consp = नूतन AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } अञ्ञथा सचे (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ध्रुव consp = नूतन AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } अञ्ञथा सचे (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } अञ्ञथा सचे (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } अञ्ञथा {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            सचे (defaultp) VL_DO_DANGLING(प्रक्षिप_Deletep(defaultp), defaultp);
        }
    }
    शून्य patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<ध्रुव AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ध्रुव AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            याव (patp) {
                करोहि {
                    सचे (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        सचे (ध्रुव AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            सचे (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                भङ्ग;
                            } अञ्ञथा {
                                ध्रुव std::pair<PatMap::iterator, तार्किक> ret
                                    = patmap.emplace(memp, patp);
                                सचे (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } अञ्ञथा सचे (Ast_ग्रन्थिः_DType* nodedtypep = VN_CAST(patp->keyp(), ग्रन्थिः_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            ध्रुव string dtype = nodedtypep->dtypep()->prettyDTypeName(सच्च);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } अञ्ञथा {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } अञ्ञथा सचे (memp) {
                        // constant expr
                        ध्रुव std::pair<PatMap::iterator, तार्किक> ret = patmap.emplace(memp, patp);
                        सचे (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } अञ्ञथा {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } याव (मिच्छा);

                // Next
                सचे (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = nullptr;
        सचे (vdtypep->packed()) {
            अत्थाय (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ध्रुव auto it = patmap.find(memp);
                सचे (it == patmap.end()) {  // default or default_type assignment
                    सचे (AstStructDType* ध्रुव memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } अञ्ञथा {
                        AstPatMember* ध्रुव patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = मानम्_Concat_pattern_Struct(patp, newp, memp, nodep);
                        सचे (patp) VL_DO_DANGLING(प्रक्षिप_Deletep(patp), patp);
                    }
                } अञ्ञथा {  // member assignment
                    newp = मानम्_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } अञ्ञथा {  // Unpacked
            AstConsStruct* ध्रुव consp = नूतन AstConsStruct{nodep->fileline(), vdtypep};
            अत्थाय (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ध्रुव auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                सचे (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    प्रक्षिप_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    प्रक्षिप_Deletep(patp);  // patp used below
                } अञ्ञथा {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = pattern_Member_मानम्_Iterate(patp);
                consp->addItemsp(नूतन AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        सचे (newp) {
            nodep->replaceWith(newp);
        } अञ्ञथा {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_ग्रन्थिः_व्यञ्जकम्* newp,
                                                 AstPattern* nodep, ध्रुव DTypeMap& dtypemap) {
        अत्थाय (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            सचे (AstStructDType* ध्रुव memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } अञ्ञथा {
                AstPatMember* ध्रुव patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = मानम्_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                सचे (patp) VL_DO_DANGLING(प्रक्षिप_Deletep(patp), patp);
            }
        }
        पटिनिवत्त newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ध्रुव DTypeMap& dtypemap) {
        ध्रुव string memp_DType = memp->virtRefDTypep()->prettyDTypeName(सच्च);
        ध्रुव auto it = dtypemap.find(memp_DType);
        सचे (it != dtypemap.end()) {
            // default_value for data_type
            पटिनिवत्त it->second->clone_वृक्षः(मिच्छा);
        }
        सचे (defaultp) {
            // default_value for any unmatched member yet
            पटिनिवत्त defaultp->clone_वृक्षः(मिच्छा);
        }
        सचे (memp->valuep()) {
            पटिनिवत्त नूतन AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_वृक्षः(मिच्छा), ग्रन्थिः_व्यञ्जकम्),
                                    नूतन AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        पटिनिवत्त nullptr;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* मानम्_Concat_pattern_Struct(AstPatMember* patp, Ast_ग्रन्थिः_व्यञ्जकम्* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        सचे (patp) {
            patp->dtypep(memp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = pattern_Member_मानम्_Iterate(patp);
            सचे (!newp) {
                newp = valuep;
            } अञ्ञथा {
                AstConcat* ध्रुव concatp = नूतन AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, सच्च);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, सच्च);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        पटिनिवत्त newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_ग्रन्थिः_DType* elemDTypep,
                                       ध्रुव DTypeMap& dtypemap = DTypeMap{}) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = defaultp->lhssp()->clone_वृक्षः(मिच्छा);
        Ast_ग्रन्थिः_DType* ध्रुव elemDTypeSkipRefp = elemDTypep->skipRefp();
        ध्रुव AstStructDType* ध्रुव structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ध्रुव तार्किक unpackedStruct = structp && !structp->packed();

        सचे (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            पटिनिवत्त defaultp->clone_वृक्षः(मिच्छा);
        }
        सचे (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            पटिनिवत्त defaultp->clone_वृक्षः(मिच्छा);
        }
        सचे (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        तार्किक wholeElement = unpackedStruct;
        सचे (Ast_ग्रन्थिः_DType* ध्रुव मानम्_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !मानम्_DTypep->skipRefp()->isIntegralOrPacked()
                                          : मानम्_DTypep->similarDType(elemDTypeSkipRefp);
        }
        सचे (wholeElement) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            पटिनिवत्त defaultp->clone_वृक्षः(मिच्छा);
        }

        AstPatMember* ध्रुव nestedDefaultp
            = नूतन AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(सच्च);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        अत्थाय (ध्रुव auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_वृक्षः(मिच्छा));
        AstPattern* ध्रुव recursivePatternp = नूतन AstPattern{defaultp->fileline(), nestedDefaultp};
        पटिनिवत्त नूतन AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_ग्रन्थिः_व्यञ्जकम्* patternDefaultValuep(AstPatMember* defaultp, Ast_ग्रन्थिः_DType* elemDTypep) {
        सचे (!defaultp) पटिनिवत्त nullptr;
        AstPatMember* ध्रुव elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = pattern_Member_मानम्_Iterate(elemDefaultp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(elemDefaultp), elemDefaultp);
        पटिनिवत्त valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    शून्य patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        ध्रुव VNumRange range = arrayDtp->declRange();
        ध्रुव पूर्णाङ्क elements = range.elements();
        AstConsArray* ध्रुव newp = नूतन AstConsArray{nodep->fileline(), arrayDtp};
        पूर्णाङ्क entn = 0;  // Number of elements given so far, from the left
        तार्किक error = मिच्छा;  // Reported an error, so the value is not complete
        अत्थाय (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = pattern_Member_मानम्_Iterate(patp);
            ध्रुव uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            ध्रुव AstUnpackArrayDType* ध्रुव spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            सचे (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_मानानि(index, valuep);
                अनुवर्त;
            }
            ध्रुव VCastable castable = Ast_ग्रन्थिः::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            सचे (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = सच्च;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        सचे (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = सच्च;
        }
        सचे (error) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(newp), newp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } अञ्ञथा {
            nodep->replaceWith(newp);
        }
    }
    शून्य patternArray(AstPattern* nodep, Ast_ग्रन्थिः_Array_DType* arrayDtp, AstPatMember* defaultp) {
        सचे (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            पटिनिवत्त;
        }
        ध्रुव VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.वामः() << " to " << range.दक्षिणः());
        Ast_ग्रन्थिः* newp = nullptr;
        तार्किक missed = मिच्छा;  // Some element missed, so not a complete value
        अत्थाय (पूर्णाङ्क entn = 0, ent = range.वामः(); entn < range.elements();
             ++entn, ent += range.वामः_To_दक्षिणः_Inc()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* valuep = nullptr;
            ध्रुव auto it = patmap.find(ent);
            सचे (it == patmap.end()) {
                सचे (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } अञ्ञथा {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = सच्च;
                }
            } अञ्ञथा {
                AstPatMember* ध्रुव patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_मानम्_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ध्रुव AstUnpackArrayDType* ध्रुव memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                सचे (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                    valuep = नूतन AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            सचे (valuep) {
                सचे (VN_IS(arrayDtp, UnpackArrayDType)) {
                    सचे (!newp) newp = नूतन AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } अञ्ञथा {  // Packed. Convert to concat for now.
                    सचे (!newp) {
                        newp = valuep;
                    } अञ्ञथा {
                        AstConcat* ध्रुव concatp
                            = नूतन AstConcat{valuep->fileline(), VN_AS(newp, ग्रन्थिः_व्यञ्जकम्), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        सचे (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        सचे (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(newp), newp);
            newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        सचे (newp) {
            nodep->replaceWith(newp);
        } अञ्ञथा {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        सचे (VN_IS(patp->keyp(), ग्रन्थिः_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            पटिनिवत्त nullptr;
        }
        सचे (patp->कुञ्चिका_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->कुञ्चिका_Exprp());  // May change
        AstConst* ध्रुव constp = VN_CAST(patp->कुञ्चिका_Exprp(), Const);
        सचे (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        पटिनिवत्त constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_ग्रन्थिः_DType* कुञ्चिका_DTypep) {
        सचे (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            पटिनिवत्त nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        सचे (कुञ्चिका_DTypep->skipRefp()->isIntegralOrPacked() && patp->कुञ्चिका_Exprp()) {
            userIterate(patp->कुञ्चिका_Exprp(), WidthVP{कुञ्चिका_DTypep, PRELIM}.p());
            ध्रुव पूर्णाङ्क width = std::max(patp->कुञ्चिका_Exprp()->width(), कुञ्चिका_DTypep->width());
            Ast_ग्रन्थिः_DType* ध्रुव contextDTypep
                = कुञ्चिका_DTypep->findBitOrLogicDType(width, width, कुञ्चिका_DTypep->numeric(), सच्च);
            iterateCheck(patp, "key", patp->कुञ्चिका_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, मिच्छा);
        }
        AstConst* ध्रुव constp = patternConstKeyp(patp);
        सचे (!constp) पटिनिवत्त nullptr;
        सचे (constp->अङ्कः().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            पटिनिवत्त nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, कुञ्चिका_DTypep, EXTEND_EXP, मिच्छा);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव convp = patp->कुञ्चिका_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        पटिनिवत्त VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_ग्रन्थिः_व्यञ्जकम्* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_ग्रन्थिः_DType* arrayDtp) {
        Ast_ग्रन्थिः_DType* ध्रुव कुञ्चिका_DTypep = VN_AS(arrayDtp, AssocArrayDType)->कुञ्चिका_DTypep();
        सचे (!कुञ्चिका_DTypep->skipRefp()->isAggregateType() || !patp->कुञ्चिका_Exprp()) {
            पटिनिवत्त patternScalarKeyp(patp, कुञ्चिका_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->कुञ्चिका_Exprp(), कुञ्चिका_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->कुञ्चिका_Exprp());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव foldedp = patp->कुञ्चिका_Exprp();
        सचे (!foldedp->is_Constant_वृक्षः()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            पटिनिवत्त nullptr;
        }
        सचे (foldedp->exists([](ध्रुव AstConst* constp) { पटिनिवत्त constp->अङ्कः().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            पटिनिवत्त nullptr;
        }
        पटिनिवत्त foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_ग्रन्थिः_DType* arrayDtp) {
        पटिनिवत्त patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_ग्रन्थिः_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        पटिनिवत्त patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    शून्य sort_Cons_वस्तूनि(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        याव (T_Item* ध्रुव itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.प्रक्षिप_back(itemp);
        }
        ध्रुव auto कुञ्चिका_Of = [](ध्रुव T_Item* itemp) -> ध्रुव V3_सङ्ख्या& {
            ध्रुव Ast_ग्रन्थिः* ध्रुव keyp = itemp->keyp();
            पटिनिवत्त VN_AS(keyp, Const)->अङ्कः();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](ध्रुव T_Item* ap, ध्रुव T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            पटिनिवत्त कुञ्चिका_Of(ap).isLtXZ(कुञ्चिका_Of(bp));
        });
        ध्रुव T_Item* firstp = nullptr;  // First item with the current key
        अत्थाय (T_Item* ध्रुव itemp : itemps) {
            सचे (firstp && कुञ्चिका_Of(firstp).isCaseEq(कुञ्चिका_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } अञ्ञथा {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    शून्य patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_ग्रन्थिः_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        अत्थाय (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ध्रुव auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = pattern_Member_मानम्_Iterate(patp);
            सचे (keyp) {
                consp->addItemsp(नूतन T_Item{patp->fileline(), keyp, valuep});
            } अञ्ञथा {
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ध्रुव AstAssocArrayDType* ध्रुव assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        सचे (!assocDtp || !assocDtp->कुञ्चिका_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_वस्तूनि<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* newConsDynArrayOrQueue(File_पङ्क्तिः* fileline, ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        सचे (VN_IS(dtypep, DynArrayDType)) पटिनिवत्त नूतन AstConsDynArray{fileline};
        सचे (VN_IS(dtypep, QueueDType)) पटिनिवत्त नूतन AstConsQueue{fileline};
        पटिनिवत्त nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    शून्य addConsItem(T_Cons* consp, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        Ast_ग्रन्थिः_DType* ध्रुव arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ध्रुव तार्किक isElement
            = Ast_ग्रन्थिः::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ध्रुव AstQueueDType* ध्रुव subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        सचे (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* ध्रुव subp = Ast_ग्रन्थिः::cast<T_Cons>(valuep);
            सचे (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                सचे (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(प्रक्षिप_Deletep(subp), subp);
                पटिनिवत्त;
            }
            सचे (AstConsQueue* ध्रुव queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_वस्तूनि(consp, queuep);
                पटिनिवत्त;
            }
            सचे (AstConsDynArray* ध्रुव dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_वस्तूनि(consp, dynp);
                पटिनिवत्त;
            }
        }
        T_Item* ध्रुव itemp = नूतन T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        सचे (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } अञ्ञथा {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    शून्य addConsItem(Ast_ग्रन्थिः_व्यञ्जकम्* consp, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        सचे (AstConsDynArray* ध्रुव dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } अञ्ञथा {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    शून्य add_Cons_वस्तूनि(Ast_ग्रन्थिः_व्यञ्जकम्* consp, T_Sub* subp) {
        याव (auto* ध्रुव itemp = subp->itemsp()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(प्रक्षिप_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(subp), subp);
    }
    शून्य patternDynArrayOrQueue(AstPattern* nodep, Ast_ग्रन्थिः_DType* arrayp, AstPatMember* defaultp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ध्रुव AstPatMember* ध्रुव firstp = VN_AS(nodep->itemsp(), PatMember);
        ध्रुव तार्किक keyed = defaultp || (firstp && firstp->keyp());
        सचे (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            पटिनिवत्त;
        }
        ध्रुव वर्ण* ध्रुव kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        सचे (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        अत्थाय (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            सचे (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_मानम्_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    शून्य patternBasic(AstPattern* nodep, Ast_ग्रन्थिः_DType* vdtypep, AstPatMember* defaultp) {
        ध्रुव AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ध्रुव VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = nullptr;
        अत्थाय (ध्रुव पूर्णाङ्क ent : range.seqHiToLo()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* valuep = nullptr;
            ध्रुव auto it = patmap.find(ent);
            सचे (it == patmap.end()) {
                सचे (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } अञ्ञथा {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } अञ्ञथा {
                AstPatMember* ध्रुव patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_मानम्_Iterate(patp);
            }
            सचे (valuep) {  // Packed. Convert to concat for now.
                सचे (!newp) {
                    newp = valuep;
                } अञ्ञथा {
                    AstConcat* ध्रुव concatp = नूतन AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        सचे (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        सचे (newp) {
            nodep->replaceWith(newp);
        } अञ्ञथा {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* pattern_Member_मानम्_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = patp->lhssp()->unlinkFrBack();
        सचे (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            सचे (AstConst* ध्रुव newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        पटिनिवत्त valuep;
    }

    स्थिर शून्य checkEventAssignment(ध्रुव Ast_ग्रन्थिः_Assign* ध्रुव asgnp) {
        string unsupEvtAsgn;
        सचे (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        सचे (VN_IS(asgnp->rhsp(), CReset)) पटिनिवत्त;
        सचे (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        सचे (!unsupEvtAsgn.empty()) {
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

    स्थिर तार्किक usesDynamicScheduler(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        याव (सच्च) {
            AstVarRef* ध्रुव vrefp = VN_CAST(nodep, VarRef);
            सचे (vrefp) पटिनिवत्त usesDynamicScheduler(vrefp);
            सचे (VN_IS(nodep, MemberSel)) {
                पटिनिवत्त सच्च;
            } अञ्ञथा सचे (Ast_ग्रन्थिः_Sel* selp = VN_CAST(nodep, ग्रन्थिः_Sel)) {
                nodep = selp->fromp();
            } अञ्ञथा {
                पटिनिवत्त मिच्छा;
            }
        }
    }

    स्थिर तार्किक usesDynamicScheduler(AstVarRef* vrefp) {
        पटिनिवत्त VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    शून्य visit(AstPatMember* nodep) override {
        Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    पूर्णाङ्क visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        सचे (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->repp(), Const);
            सचे (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } अञ्ञथा {
                times = constp->toUInt();
            }
            सचे (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_वृक्षः();  // Done with replicate before cloning
        }
        पटिनिवत्त times;
    }

    शून्य visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_ग्रन्थिः* propp = nodep->propp();
            याव (AstLogNot* ध्रुव notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ध्रुव untilp = VN_CAST(propp, Until);
            सचे (untilp && VN_IS(nodep->backp(), ग्रन्थिः_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(नूतन AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(प्रक्षिप_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            सचे (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    तार्किक firstNewStatementOkRecurse(Ast_ग्रन्थिः* nodep) {
        सचे (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
            सचे (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) पटिनिवत्त सच्च;
        }
        सचे (AstAssign* ध्रुव aitemp = VN_CAST(nodep, Assign)) {
            सचे (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) पटिनिवत्त सच्च;
        }
        सचे (AstInitialStaticStmt* ध्रुव aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            अत्थाय (Ast_ग्रन्थिः* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                सचे (!firstNewStatementOkRecurse(stmtp)) पटिनिवत्त मिच्छा;
            }
            पटिनिवत्त सच्च;
        }
        सचे (AstInitialAutomaticStmt* ध्रुव aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            अत्थाय (Ast_ग्रन्थिः* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                सचे (!firstNewStatementOkRecurse(stmtp)) पटिनिवत्त मिच्छा;
            }
            पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    शून्य handleCaseType(Ast_ग्रन्थिः* casep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, CaseItem* itemsp) {
        AstAttrOf* ध्रुव exprap = VN_CAST(exprp, AttrOf);
        सचे (!exprap) पटिनिवत्त;
        सचे (exprap->attrType() != VAttrType::TYPEID) पटिनिवत्त;

        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव व्यञ्जकम्_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << व्यञ्जकम्_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_पङ्क्तिः* ध्रुव newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, सच्च);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, सच्च);  // Side effect of transform
        casep->fileline(newfl);
        अत्थाय (CaseItem* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<CaseItem>(itemp->nextp())) {
            सचे (itemp->isDefault()) अनुवर्त;
            तार्किक hit = मिच्छा;
            अत्थाय (Ast_ग्रन्थिः* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ध्रुव AstAttrOf* ध्रुव condAttrp = VN_CAST(condp, AttrOf);
                सचे (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } अञ्ञथा {
                    Ast_ग्रन्थिः_DType* ध्रुव condDtp = condAttrp->dtypep();
                    सचे (Ast_ग्रन्थिः::computeCastable(व्यञ्जकम्_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = सच्च;
                        भङ्ग;
                    }
                }
            }
            प्रक्षिप_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(नूतन AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(नूतन AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    शून्य handleCase(Ast_ग्रन्थिः* casep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_ग्रन्थिः_DType* subDTypep = exprp->dtypep();
        अत्थाय (CaseItem* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<CaseItem>(itemp->nextp())) {
            अत्थाय (Ast_ग्रन्थिः* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                सचे (condp->dtypep() == subDTypep) अनुवर्त;

                सचे (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } अञ्ञथा सचे (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } अञ्ञथा {
                    ध्रुव पूर्णाङ्क width = std::max(subDTypep->width(), condp->width());
                    ध्रुव पूर्णाङ्क mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    ध्रुव तार्किक issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        अत्थाय (CaseItem* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<CaseItem>(itemp->nextp())) {
            अत्थाय (Ast_ग्रन्थिः *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    शून्य visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        अत्थाय (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            अत्थाय (Ast_ग्रन्थिः *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
        अत्थाय (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            अत्थाय (Ast_ग्रन्थिः *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
        Ast_ग्रन्थिः_DType* ध्रुव itemDTypep = nodep->findUInt32DType();
        अत्थाय (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            अत्थाय (Ast_ग्रन्थिः *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    शून्य visit(Ast_ग्रन्थिः_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    शून्य visit(Ast_व्यञ्जकम्_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    शून्य visit(Ast_ग्रन्थिः_Foreach* nodep) override {
        सचे (nodep->didWidth()) पटिनिवत्त;
        nodep->didWidth(सच्च);
        ध्रुव AstForeachHeader* ध्रुव headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_ग्रन्थिः_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        अत्थाय (Ast_ग्रन्थिः *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            ध्रुव तार्किक empty = VN_IS(argsp, Empty);
            AstVar* ध्रुव varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            सचे (varp) varp->usedLoopIdx(सच्च);
            सचे (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
                पटिनिवत्त;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            सचे (VN_IS(fromDtp, ग्रन्थिः_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } अञ्ञथा सचे (AstBasicDType* ध्रुव adtypep = VN_CAST(fromDtp, BasicDType)) {
                सचे (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
                    पटिनिवत्त;
                }
            } अञ्ञथा सचे (ध्रुव AstAssocArrayDType* ध्रुव adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->कुञ्चिका_DTypep());
            } अञ्ञथा {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
                पटिनिवत्त;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_ग्रन्थिः* ध्रुव bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        सचे (AstForeach* ध्रुव loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            पटिनिवत्त;
        }
    }

    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
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
            सचे (ध्रुव AstIfaceRefDType* ध्रुव irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                सचे (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                    पटिनिवत्त;
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
            Ast_ग्रन्थिः_DType* ध्रुव lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            सचे (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtp = lhsDTypep->skipRefp();
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ध्रुव तार्किक lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ध्रुव तार्किक rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                सचे (lhsIsUnpackArray && rhsIsUnpackArray) {
                    सचे (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_ग्रन्थिः_Var_Ref, Ast_ग्रन्थिः_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), ग्रन्थिः_Var_Ref), VN_CAST(nodep->rhsp(), ग्रन्थिः_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                सचे (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), ग्रन्थिः_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        सचे (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        सचे (Ast_ग्रन्थिः* ध्रुव controlp = nodep->timingControlp()) {
            सचे (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_वृक्षः(), controlp);
            } अञ्ञथा सचे (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } अञ्ञथा {
                सचे (nodep->fileline()->timingOn()) {
                    सचे (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } अञ्ञथा {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_वृक्षः(), controlp);
            }
        }
        सचे (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            सचे (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                पटिनिवत्त;
            }
            AstMethodCall* ध्रुव newp
                = नूतन AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            पटिनिवत्त;
        }
        सचे (ध्रुव AstNewDynamic* ध्रुव dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            सचे (!dynp->rhsp()) {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } अञ्ञथा {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(सच्च);
            newp->protect(मिच्छा);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        सचे (AstCMethodHard* ध्रुव slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            सचे (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } अञ्ञथा सचे (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } अञ्ञथा सचे (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } अञ्ञथा {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            सचे (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव msbp = slicep->pinsp()->unlinkFrBack();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* ध्रुव newp
                    = नूतन AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(सच्च);
                newp->protect(मिच्छा);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            }
        }

        सचे (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    शून्य visit(AstRSRule* nodep) override {
        सचे (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
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

    स्थिर तार्किक isFormatNonNumericArg(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        पटिनिवत्त dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType)
                   && !VN_AS(dtypep, ग्रन्थिः_UOr_Struct_DType)->packed());
    }

    शून्य checkFormatNumericArg(Ast_ग्रन्थिः* argp, वर्ण ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        सचे (std::tolower(ch) == 'd') पटिनिवत्त;
        // In contrast, %x of a class reference, causes errors on several simulators.
        सचे (isFormatNonNumericArg(argp->dtypep()))
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
        सचे (m_vup && !m_vup->prelim()) पटिनिवत्त;  // Can be called as statement or function
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        सचे (!nodep->व्यञ्जकम्_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* oldExprsp = nodep->exprsp();
        सचे (nodep->व्यञ्जकम्_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), ग्रन्थिः_व्यञ्जकम्);
        }
        सचे (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        याव (Ast_ग्रन्थिः_व्यञ्जकम्* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), ग्रन्थिः_व्यञ्जकम्);
            सचे (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            सचे (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                अनुवर्त;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = argp->dtypep()->skipRefp();
            सचे (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } अञ्ञथा सचे (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } अञ्ञथा सचे (isFormatNonNumericArg(dtypep)) {
                सचे (AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    सचे (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(सच्च);
                    }
                } अञ्ञथा {
                    // Class handles inside containers print type names, not their members.
                    Ast_ग्रन्थिः_DType* ग्रन्थिः_Dtypep = dtypep;
                    याव (ग्रन्थिः_Dtypep) {
                        ग्रन्थिः_Dtypep = ग्रन्थिः_Dtypep->skipRefp();
                        सचे (Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव uOrStructDTypep
                            = VN_CAST(ग्रन्थिः_Dtypep, ग्रन्थिः_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(सच्च);
                        }
                        ग्रन्थिः_Dtypep = ग्रन्थिः_Dtypep->subDTypep();
                    }
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } अञ्ञथा सचे (nodep->व्यञ्जकम्_Format_2()) {
                सचे (AstEnumDType* ध्रुव enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    अनुवर्त;
                }
            }
            सचे (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            सचे (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } अञ्ञथा {
                nodep->addExprsp(नूतन AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    शून्य visit(AstCReset* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    शून्य visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    शून्य visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    शून्य visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        सचे (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        सचे (!m_paramsOnly) {
            nodep->foreach([एतत्](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    नूतन AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            सचे (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            विकल्प (nodep->displayType()) {
            पक्ष VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); भङ्ग;
            पक्ष VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); भङ्ग;
            पक्ष VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); भङ्ग;
            पक्ष VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); भङ्ग;
            स्वभाव: UASSERT_OBJ(मिच्छा, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
        }
    }
    शून्य visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        सचे (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(AstFEof* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    शून्य visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        सचे (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    शून्य visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    शून्य visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    शून्य visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    शून्य visit(AstFGetC* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    शून्य visit(AstFGetS* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(AstFUngetC* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    शून्य visit(AstFRead* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            सचे (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            सचे (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    शून्य visit(AstFScanF* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(AstSScanF* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(Ast_राशिः_Trace_F* nodep) override { nodep->dtypeSetString(); }
    शून्य visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstSystemF* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(Ast_ग्रन्थिः_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        ध्रुव Ast_ग्रन्थिः_DType* subp = nullptr;
        सचे (ध्रुव AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            सचे (!adtypep->कुञ्चिका_DTypep()->skipRefp()->basicp()
                || !adtypep->कुञ्चिका_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } अञ्ञथा सचे (ध्रुव AstUnpackArrayDType* ध्रुव adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } अञ्ञथा सचे (ध्रुव AstDynArrayDType* ध्रुव adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } अञ्ञथा {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        सचे (subp
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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    शून्य visit(Ast_मानम्_Plus_Args* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        सचे (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        सचे (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        सचे (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        सचे (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
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
    तार्किक pinInstArrayElement(AstPin* nodep, ध्रुव AstRange* rangep, पूर्णाङ्क idx) {
        Ast_ग्रन्थिः_DType* ध्रुव modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_ग्रन्थिः_DType* ध्रुव conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        सचे (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) पटिनिवत्त सच्च;
        // Types don't match, but all dimensions done, still need the usual checks
        सचे (!rangep) पटिनिवत्त मिच्छा;

        ध्रुव AstRange* ध्रुव subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        पूर्णाङ्क subElems = 1;
        अत्थाय (ध्रुव AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ध्रुव पूर्णाङ्क elements = rangep->elementsConst();
        ध्रुव पूर्णाङ्क from_वामः = idx / subElems;

        ध्रुव AstUnpackArrayDType* ध्रुव arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ध्रुव uint32_t conDims = conDTypep->dimensions(मिच्छा).second;
        ध्रुव uint32_t modDims = modDTypep->dimensions(मिच्छा).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        सचे (arrp && conDims != modDims) {
            सचे (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                पटिनिवत्त सच्च;
            }
            // ArraySel index is counted from lo
            ध्रुव पूर्णाङ्क bit = arrp->declRange().ascending() ? from_वामः : elements - 1 - from_वामः;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्)->unlinkFrBack();
            सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(fromp), fromp);
            } अञ्ञथा {
                AstArraySel* ध्रुव selp = नूतन AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(सच्च);
                nodep->exprp(selp);
            }
            पटिनिवत्त pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        सचे (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) पटिनिवत्त मिच्छा;

        // Packed connection
        ध्रुव पूर्णाङ्क modwidth = modDTypep->width();
        ध्रुव पूर्णाङ्क conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        सचे (conwidth == modwidth) पटिनिवत्त मिच्छा;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        सचे (conwidth == modwidth * elements * subElems) {
            सचे (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->वामः_Const() << ":"
                                                     << rangep->दक्षिणः_Const() << "]");
            }
            ध्रुव पूर्णाङ्क selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्)->unlinkFrBack();
            AstSel* ध्रुव selp = नूतन AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_वामः), selwidth};
            selp->didWidth(सच्च);
            nodep->exprp(selp);
            पटिनिवत्त pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        पटिनिवत्त सच्च;
    }

    शून्य visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        सचे (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ध्रुव concatp = VN_CAST(nodep->exprp(), Replicate);
            ध्रुव AstConst* ध्रुव countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            सचे (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* ध्रुव patternp = नूतन AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } अञ्ञथा सचे (!m_paramsOnly) {
            सचे (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            सचे (!nodep->exprp()) {  // No-connect
                सचे (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                पटिनिवत्त;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ध्रुव VDirection pinDirection = nodep->modVarp()->direction();
            सचे (VN_IS(nodep->exprp(), ग्रन्थिः_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_ग्रन्थिः_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_ग्रन्थिः_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            सचे (ध्रुव AstIfaceRefDType* ध्रुव modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ध्रुव AstIfaceRefDType* ध्रुव conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                सचे (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_ग्रन्थिः_DType* subDTypep = modDTypep;
            ध्रुव पूर्णाङ्क modwidth = modDTypep->width();
            पूर्णाङ्क conwidth = conDTypep->width();
            सचे (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } अञ्ञथा {
                सचे (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    सचे (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        सचे (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        पटिनिवत्त;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                सचे (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } अञ्ञथा सचे (nodep->modVarp()->isTristate()) {
                    सचे (modwidth != conwidth) {
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
                } अञ्ञथा सचे (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                सचे ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव व्यञ्जकम्_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                सचे (व्यञ्जकम्_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } अञ्ञथा सचे ((व्यञ्जकम्_Arrayp && !modArrayp) || (!व्यञ्जकम्_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (व्यञ्जकम्_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } अञ्ञथा {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_ग्रन्थिः_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), ग्रन्थिः_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            सचे (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    शून्य visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        सचे (!m_paramsOnly) {
            सचे (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_सन्देशः(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            सचे (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            सचे (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                प्रक्षिप_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    शून्य visit(AstGatePin* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव pinDTypep = nodep->exprp()->dtypep();
            ध्रुव पूर्णाङ्क अङ्कः_Insts = nodep->rangep()->elementsConst();
            ध्रुव पूर्णाङ्क modwidth = अङ्कः_Insts;
            ध्रुव पूर्णाङ्क pinwidth = pinDTypep->width();
            सचे (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_ग्रन्थिः_DType* ध्रुव subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_ग्रन्थिः* ध्रुव newp
                    = नूतन AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(अङ्कः_Insts)};
                nodep->replaceWith(newp);
            } अञ्ञथा {
                सचे (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
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
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        सचे (nodep->didWidth()) पटिनिवत्त;
        सचे (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(सच्च);
            nodep->didWidth(सच्च);
            पटिनिवत्त;
        }
        सचे (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } अञ्ञथा सचे (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        सचे (nodep->classMethod() && nodep->name() == "new") {
            सचे (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            सचे (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_ग्रन्थिः* firstp = nullptr;
            अत्थाय (Ast_ग्रन्थिः* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                सचे (Ast_Stmt_व्यञ्जकम्* ध्रुव sep = VN_CAST(itemp, Stmt_व्यञ्जकम्)) {
                    सचे (AstNew* ध्रुव newp = VN_CAST(sep->exprp(), New)) {
                        सचे (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            भङ्ग;
                        }
                    }
                    अनुवर्त;
                }
                सचे (firstNewStatementOkRecurse(itemp)) अनुवर्त;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(सच्च);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        सचे (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        सचे (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } अञ्ञथा सचे (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } अञ्ञथा सचे (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        सचे (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(सच्च);
        nodep->doingWidth(मिच्छा);
        सचे (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        सचे (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](ध्रुव AstVarRef* varrefp) {
                   पटिनिवत्त varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        अत्थाय (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                सचे (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } अञ्ञथा सचे (AstAttrOf* ध्रुव attrp = VN_CAST(stmtp, AttrOf)) {
                सचे (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                सचे (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        सचे (protop) {
            अत्थाय (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                सचे (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    भङ्ग;
                } अञ्ञथा {
                    AstVar* ध्रुव portp = ports[i];
                    AstAttrOf* ध्रुव rprotop = protos[i];
                    Ast_ग्रन्थिः_DType* ध्रुव declDtp = portp->dtypep();
                    Ast_ग्रन्थिः_DType* ध्रुव protoDtp = rprotop->fromp()->dtypep();
                    सचे (portp->name() != rprotop->name()) {
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
                    } अञ्ञथा सचे (!similarDTypeRecurse(protoDtp, declDtp)) {
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
        सचे (nodep->didWidth()) पटिनिवत्त;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(सच्च);
    }
    शून्य visit(AstProperty* nodep) override {
        सचे (nodep->didWidth()) पटिनिवत्त;
        सचे (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(सच्च);
            nodep->didWidth(सच्च);
            पटिनिवत्त;
        }
        nodep->doingWidth(सच्च);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        अत्थाय (Ast_ग्रन्थिः* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            सचे (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } अञ्ञथा सचे (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } अञ्ञथा सचे (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } अञ्ञथा {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(सच्च);
        nodep->doingWidth(मिच्छा);
    }
    शून्य visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    शून्य visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        सचे (nodep->didWidth()) पटिनिवत्त;
        nodep->dtypeSetBit();
        अत्थाय (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } अञ्ञथा सचे (VN_IS(stmtp, ग्रन्थिः_व्यञ्जकम्)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } अञ्ञथा {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(सच्च);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_ग्रन्थिः* pkgItemp) {
        याव (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        पटिनिवत्त VN_CAST(pkgItemp->backp(), Package);
    }
    शून्य visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_ग्रन्थिः_FTask_Ref*>(nodep));
        सचे (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            सचे (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } अञ्ञथा {
            nodep->dtypeFrom(nodep->taskp());
        }
        सचे (nodep->fileline()->timingOn()) {
            Ast_ग्रन्थिः_Module* ध्रुव classp = nodep->classOrPackagep();
            सचे (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ध्रुव packagep = getItemPackage(classp);
                सचे (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    स्थिर तार्किक areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ध्रुव std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ध्रुव std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        सचे (dims0.size() != dims1.size()) पटिनिवत्त मिच्छा;
        अत्थाय (size_t i = 0; i < dims0.size(); ++i) {
            सचे (dims0[i]->elementsConst() != dims1[i]->elementsConst()) पटिनिवत्त मिच्छा;
        }
        पटिनिवत्त सच्च;
    }
    // Makes sure that port and pin have same size and same datatype
    शून्य checkUnpackedArrayArgs(AstVar* portp, Ast_ग्रन्थिः* pinp) {
        सचे (AstUnpackArrayDType* ध्रुव portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            सचे (AstUnpackArrayDType* ध्रुव pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                सचे (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                ध्रुव AstBasicDType* ध्रुव portBasicp = portDtypep->basicp();
                ध्रुव AstBasicDType* ध्रुव pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ध्रुव तार्किक sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                सचे (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } अञ्ञथा {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    शून्य processFTaskRefArgs(Ast_ग्रन्थिः_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        सचे (nodep->didWidth()) पटिनिवत्त;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        करोहि {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ध्रुव V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            सचे (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            अत्थाय (ध्रुव auto& tconnect : tconnects) {
                ध्रुव AstVar* ध्रुव portp = tconnect.first;
                AstArg* ध्रुव argp = tconnect.second;
                Ast_ग्रन्थिः_व्यञ्जकम्* pinp = argp->exprp();
                सचे (!pinp) अनुवर्त;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                सचे (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_ग्रन्थिः_व्यञ्जकम्* argsp = nullptr;
                    याव (AstArg* ध्रुव nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_वृक्षः();  // Remove the call's Arg wrapper
                    }
                    string format;
                    सचे (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->अङ्कः().toString();
                    } अञ्ञथा {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(प्रक्षिप_Deletep(argp), argp);
                    AstSFormatF* ध्रुव newp
                        = नूतन AstSFormatF{nodep->fileline(), format, मिच्छा, argsp};
                    सचे (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(नूतन AstScopeName{newp->fileline(), सच्च});
                    }
                    handle.relink(नूतन AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    गच्छ reloop;  // so exit early; next loop will correct it
                }  //
                अञ्ञथा सचे (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                सचे (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } याव (मिच्छा);
        // Stage 2
        {
            ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            अत्थाय (ध्रुव auto& tconnect : tconnects) {
                AstVar* ध्रुव portp = tconnect.first;
                ध्रुव AstArg* ध्रुव argp = tconnect.second;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = argp->exprp();
                सचे (!pinp) अनुवर्त;  // Argument error we'll find later
                // Change data types based on above accept completion
                सचे (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                सचे (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            अत्थाय (ध्रुव auto& tconnect : tconnects) {
                ध्रुव AstVar* ध्रुव portp = tconnect.first;
                ध्रुव AstArg* ध्रुव argp = tconnect.second;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = argp->exprp();
                सचे (!pinp) अनुवर्त;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        सचे (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            अत्थाय (ध्रुव auto& tconnect : tconnects) {
                ध्रुव AstVar* ध्रुव portp = tconnect.first;
                ध्रुव AstArg* ध्रुव argp = tconnect.second;
                Ast_ग्रन्थिः_व्यञ्जकम्* pinp = argp->exprp();
                सचे (!pinp) अनुवर्त;  // Argument error we'll find later
                Ast_ग्रन्थिः_DType* ध्रुव portDTypep = portp->dtypep()->skipRefToEnump();
                सचे (VN_IS(pinp, EmptyQueue)) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    सचे (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = नूतन AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव pinDTypep = pinp->dtypep()->skipRefToEnump();
                ध्रुव AstIfaceRefDType* ध्रुव portIfacep
                    = VN_CAST(portDTypep->elemDTypep(सच्च), IfaceRefDType);
                ध्रुव VCastable ifaceCastable
                    = portIfacep ? Ast_ग्रन्थिः::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ सच्च)
                                 : VCastable{VCastable::UNSUPPORTED};
                ध्रुव तार्किक matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                ध्रुव तार्किक pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->अङ्कः().isNull();
                सचे ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    सचे (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } अञ्ञथा {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } अञ्ञथा सचे (portIfacep && portIfacep->isVirtual() && portp->is_निवेशः() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } अञ्ञथा सचे (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                सचे (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    अनुवर्त;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                सचे (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } अञ्ञथा सचे (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } अञ्ञथा {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    शून्य handleStdRandomizeArgs(Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep) {
        AstConst* nullp = nullptr;
        अत्थाय (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = argp->exprp();
            सचे (AstConst* ध्रुव constp = VN_CAST(exprp, Const)) {
                सचे (constp->अङ्कः().isNull()) {
                    nullp = constp;
                    अनुवर्त;
                }
            }
            सचे (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                अनुवर्त;
            }
            सचे (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                अनुवर्त;
            }
            सचे (ध्रुव AstCMethodHard* ध्रुव methodp = VN_CAST(exprp, CMethodHard)) {
                सचे (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    अनुवर्त;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        सचे (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        सचे (nodep->didWidth()) पटिनिवत्त;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        सचे (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(सच्च);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            पटिनिवत्त;  // Handled in V3Randomize
        } अञ्ञथा सचे (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ध्रुव classp = VN_CAST(nodep->classOrPackagep(), Class);
            सचे (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(सच्च);
                Ast_ग्रन्थिः_DType* ध्रुव adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, मिच्छा, मिच्छा, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                अत्थाय (ध्रुव AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                सचे (withp) nodep->withp(withp);
                nodep->didWidth(सच्च);
                पटिनिवत्त;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            सचे (nodep->name() == "randomize") {
                AstClassRefDType* ध्रुव adtypep
                    = नूतन AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, मिच्छा, मिच्छा, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                अत्थाय (ध्रुव AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } अञ्ञथा सचे (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } अञ्ञथा सचे (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(सच्च);
                v3Global.useRandomizeMethods(सच्च);
                AstCExpr* ध्रुव newp
                    = नूतन AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            } अञ्ञथा सचे (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(सच्च);
                v3Global.useRandomizeMethods(सच्च);
                AstCExpr* ध्रुव newp
                    = नूतन AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            } अञ्ञथा {
                UASSERT_OBJ(मिच्छा, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        सचे (nodep->didWidth()) {
            सचे (withp) nodep->withp(withp);
            पटिनिवत्त;
        }
        सचे ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            तार्किक allow = मिच्छा;
            सचे (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                सचे (ध्रुव AstFuncRef* ध्रुव funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } अञ्ञथा सचे (ध्रुव AstTaskRef* ध्रुव taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                सचे (!allow) {
                    ध्रुव AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    सचे (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    ध्रुव AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    सचे (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            सचे (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ध्रुव AstClass* ध्रुव cgClassp = m_containingClassFinder.find(nodep);
                सचे (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ध्रुव AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    सचे (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            सचे (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        सचे (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(नूतन AstScopeName{nodep->fileline(), मिच्छा});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        सचे (withp) nodep->withp(withp);
        nodep->didWidth(सच्च);
        // See steps that follow in visit(AstFuncRef*)
    }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    शून्य visit(AstSenItem* nodep) override {
        सचे (nodep->isComboStar()) पटिनिवत्त;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        सचे (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* ध्रुव concatOrReplp = VN_CAST(nodep->sensp(), ग्रन्थिः_Biop);
            auto* ध्रुव rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            सचे (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* ध्रुव lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(नूतन AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(नूतन AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अञ्ञथा {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            सचे (nodep->edgeType().anEdge()) {
                Ast_ग्रन्थिः_DType* ध्रुव sensDtp = nodep->sensp()->dtypep()->skipRefp();
                सचे (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } अञ्ञथा सचे (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    शून्य visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ध्रुव refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    शून्य visit(AstWait* nodep) override {
        सचे (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }
        सचे (nodep->fileline()->timingOn()) {
            सचे (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                सचे (Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(nodep->condp(), ग्रन्थिः_Var_Ref)) {
                    सचे (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                पटिनिवत्त;
            } अञ्ञथा सचे (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } अञ्ञथा {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        सचे (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } अञ्ञथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->मानम्_Arg_Refp(), nullptr);
        सचे (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } अञ्ञथा {
            सचे (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } अञ्ञथा {
                सचे (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } अञ्ञथा {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            सचे (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } अञ्ञथा {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    शून्य visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        सचे (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } अञ्ञथा {
            nodep->dtypeFrom(m_withp->मानम्_Arg_Refp());
        }
    }
    शून्य visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        सचे (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    शून्य visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        सचे (Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } अञ्ञथा सचे (!nodep->dtypep()) {
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
        nodep->didWidth(सच्च);
    }
    शून्य visitClass(AstClass* nodep) {
        सचे (nodep->didWidthAndSet()) पटिनिवत्त;

        // If the class is std::process
        सचे (nodep->name() == "process") {
            AstPackage* ध्रुव packagep = getItemPackage(nodep);
            सचे (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                सचे (AstVar* ध्रुव varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->getChildDTypep();
                    सचे (!varp->dtypep()) {
                        VL_DO_DANGLING(प्रक्षिप_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* ध्रुव newdtypep = नूतन AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                सचे (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), ग्रन्थिः_FTask)) {
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
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        सचे (AstClass* ध्रुव classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } अञ्ञथा {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([एतत्](ध्रुव AstVar* varp) { m_curModVars.निवेशय(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    शून्य visit_Or_Lu64(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    शून्य visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            सचे (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    नूतन AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            }
        }
    }
    शून्य visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    शून्य visit_Os32_Lr(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    शून्य visit_Ou64_Lr(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            सचे (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ध्रुव newp
                    = नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
    }
    शून्य visit_log_and_or(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    शून्य visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit_red_and_or(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    शून्य visit_red_unknown(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    तार्किक isEquivalentDType(ध्रुव Ast_ग्रन्थिः_DType* lhs, ध्रुव Ast_ग्रन्थिः_DType* rhs) {
        // a) If two types match, they are equivalent.
        सचे (!lhs || !rhs) पटिनिवत्त मिच्छा;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        सचे (lhs == rhs) पटिनिवत्त सच्च;
        // If both are basic types, check if they are the same type
        सचे (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ध्रुव auto* lb = VN_CAST(lhs, BasicDType);
            ध्रुव auto* rb = VN_CAST(rhs, BasicDType);
            सचे (lb->isString() != rb->isString()) पटिनिवत्त मिच्छा;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ध्रुव तार्किक lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ध्रुव तार्किक rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        सचे (lhsIsUnpackArray || rhsIsUnpackArray) {
            सचे (lhsIsUnpackArray && rhsIsUnpackArray) {
                ध्रुव AstUnpackArrayDType* ध्रुव lhsp = VN_CAST(lhs, UnpackArrayDType);
                ध्रुव AstUnpackArrayDType* ध्रुव rhsp = VN_CAST(rhs, UnpackArrayDType);
                ध्रुव पूर्णाङ्क lsz = lhsp->elementsConst();
                ध्रुव पूर्णाङ्क rsz = rhsp->elementsConst();
                सचे (lsz >= 0 && rsz >= 0 && lsz != rsz) पटिनिवत्त मिच्छा;
                पटिनिवत्त isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            पटिनिवत्त मिच्छा;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ध्रुव तार्किक lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ध्रुव तार्किक rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ध्रुव तार्किक lhsIsQueue = VN_IS(lhs, QueueDType);
        ध्रुव तार्किक rhsIsQueue = VN_IS(rhs, QueueDType);
        ध्रुव तार्किक lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ध्रुव तार्किक rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        सचे (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            सचे (ध्रुव AstDynArrayDType* ध्रुव lhsp = VN_CAST(lhs, DynArrayDType)) {
                सचे (ध्रुव AstDynArrayDType* ध्रुव rhsp = VN_CAST(rhs, DynArrayDType)) {
                    पटिनिवत्त isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            सचे (ध्रुव AstQueueDType* ध्रुव lhsp = VN_CAST(lhs, QueueDType)) {
                सचे (ध्रुव AstQueueDType* ध्रुव rhsp = VN_CAST(rhs, QueueDType)) {
                    पटिनिवत्त isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            सचे (ध्रुव AstAssocArrayDType* ध्रुव lhsp = VN_CAST(lhs, AssocArrayDType)) {
                सचे (ध्रुव AstAssocArrayDType* ध्रुव rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    पटिनिवत्त isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->कुञ्चिका_DTypep(), rhsp->कुञ्चिका_DTypep());
                }
            }

            पटिनिवत्त मिच्छा;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        सचे (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            सचे (lhs->width() != rhs->width()) पटिनिवत्त मिच्छा;
            सचे (lhs->isFourstate() != rhs->isFourstate()) पटिनिवत्त मिच्छा;
            सचे (lhs->isSigned() != rhs->isSigned()) पटिनिवत्त मिच्छा;
            पटिनिवत्त सच्च;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        सचे (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) पटिनिवत्त मिच्छा;
        पटिनिवत्त !(VN_IS(lhs, ग्रन्थिः_UOr_Struct_DType) || VN_IS(rhs, ग्रन्थिः_UOr_Struct_DType));
    }

    स्थिर तार्किक isAggregateType(ध्रुव Ast_ग्रन्थिः* nodep) {
        सचे (!nodep) पटिनिवत्त मिच्छा;
        ध्रुव Ast_ग्रन्थिः_DType* dtypep = nodep->dtypep();
        सचे (!dtypep) पटिनिवत्त मिच्छा;
        dtypep = dtypep->skipRefp();
        सचे (!dtypep) पटिनिवत्त मिच्छा;
        पटिनिवत्त dtypep->isAggregateType();
    }

    शून्य visit_cmp_eq_gt(Ast_ग्रन्थिः_Biop* nodep, ध्रुव ComparisonType cmpType) {
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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ध्रुव तार्किक lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            ध्रुव तार्किक rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            सचे (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } अञ्ञथा सचे (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } अञ्ञथा {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            ध्रुव तार्किक isAggrLhs = isAggregateType(nodep->lhsp());
            ध्रुव तार्किक isAggrRhs = isAggregateType(nodep->rhsp());

            सचे ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDType = nodep->lhsp()->dtypep();
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDType = nodep->rhsp()->dtypep();

                सचे (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_ग्रन्थिः* ध्रुव newp
                        = नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } अञ्ञथा सचे (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                सचे (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_ग्रन्थिः* ध्रुव newp
                        = नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त;
                }
                सचे (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } अञ्ञथा सचे (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                सचे (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } अञ्ञथा {
                ध्रुव तार्किक signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                सचे (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ध्रुव पूर्णाङ्क width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ध्रुव पूर्णाङ्क ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                तार्किक warnOn = सच्च;
                सचे (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    सचे ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = मिच्छा;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    सचे ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = मिच्छा;
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
    शून्य visit_cmp_real(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit_cmp_string(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit_cmp_type(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            ध्रुव AstAttrOf* ध्रुव lhsap = VN_AS(nodep->lhsp(), AttrOf);
            ध्रुव AstAttrOf* ध्रुव rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtp = lhsap->dtypep();
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ध्रुव तार्किक invert = VN_IS(nodep, NeqT);
            ध्रुव तार्किक identical
                = Ast_ग्रन्थिः::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ध्रुव तार्किक eq = invert ^ identical;
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }

    शून्य visit_negate_not(Ast_ग्रन्थिः_Uniop* nodep, तार्किक real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            सचे (!real_ok) checkCvtUS(nodep->lhsp(), मिच्छा);
        }
        सचे (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            सचे (Ast_ग्रन्थिः_Uniop* ध्रुव newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                पटिनिवत्त;
            }
        } अञ्ञथा {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        सचे (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        सचे (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            // Some warning suppressions
            तार्किक lhsWarn = सच्च;
            सचे (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                सचे (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = मिच्छा;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    शून्य visit_signed_unsigned(Ast_ग्रन्थिः_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), सच्च);
            ध्रुव पूर्णाङ्क width = nodep->lhsp()->width();
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    शून्य visit_shift(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (शून्य)newp;  // Ununused
    }
    शून्य iterate_shift_prelim(Ast_ग्रन्थिः_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), मिच्छा);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            सचे (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_ग्रन्थिः_Biop* iterate_shift_final(Ast_ग्रन्थिः_Biop* nodep) {
        // Nodep maybe edited
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            सचे (VN_IS(nodep, ShiftRS)) {
                सचे (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            तार्किक warnOn = सच्च;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            सचे (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), ग्रन्थिः_Assign)) warnOn = मिच्छा;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ध्रुव shiftp = VN_CAST(nodep->rhsp(), Const);
            सचे (shiftp && !shiftp->अङ्कः().isFourState() && shiftp->width() <= 32) {
                ध्रुव int64_t shiftVal = shiftp->अङ्कः().toSQuad();
                सचे (VN_IS(nodep, ShiftL)) {
                    सचे (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = मिच्छा;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            सचे (nodep->rhsp()->width() > 32) {
                सचे (shiftp && shiftp->अङ्कः().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_सङ्ख्या अङ्कः{shiftp, 32, shiftp->अङ्कः()};
                    Ast_ग्रन्थिः* ध्रुव shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(नूतन AstConst{shiftrhsp->fileline(), अङ्कः});
                    VL_DO_DANGLING(shiftrhsp->delete_वृक्षः(), shiftrhsp);
                }
            }
        }
        पटिनिवत्त nodep;  // May edit
    }

    शून्य visit_boolexpr_and_or(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), मिच्छा);
            checkCvtUS(nodep->rhsp(), मिच्छा);
            ध्रुव पूर्णाङ्क width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            ध्रुव पूर्णाङ्क mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            ध्रुव तार्किक expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        सचे (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    शून्य visit_add_sub_replace(Ast_ग्रन्थिः_Biop* nodep, तार्किक real_ok, ध्रुव तार्किक modOrDiv) {
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
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            सचे (!real_ok) {
                checkCvtUS(nodep->lhsp(), मिच्छा);
                checkCvtUS(nodep->rhsp(), मिच्छा);
            }
            सचे (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                सचे (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                पटिनिवत्त;
            } अञ्ञथा सचे (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            } अञ्ञथा {
                ध्रुव पूर्णाङ्क width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ध्रुव पूर्णाङ्क mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                ध्रुव तार्किक expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        सचे (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            सचे (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            तार्किक lhsWarn = सच्च;
            तार्किक rhsWarn = सच्च;
            सचे (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                सचे (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = मिच्छा;
                सचे (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = मिच्छा;
                सचे (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = मिच्छा;  // do_increment + ...
                सचे (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = मिच्छा;  // ... + do_increment
            } अञ्ञथा सचे (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                सचे (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = मिच्छा;
                सचे (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = मिच्छा;
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
    शून्य visit_real_add_sub(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    शून्य visit_real_neg_ceil(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_व्यञ्जकम्(nodep);
        सचे (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    शून्य visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->व्यञ्जकम्_Format_2(), nodep, "Assumes constant format");
        तार्किक inPct = मिच्छा;
        Ast_ग्रन्थिः_व्यञ्जकम्* argp = nodep->exprsp();
        string newFormat;
        अत्थाय (वर्ण ch : nodep->text()) {
            सचे (!inPct && ch == '%') {
                inPct = सच्च;
                newFormat += ch;
            } अञ्ञथा सचे (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } अञ्ञथा सचे (!inPct) {  // Normal text
                newFormat += ch;
            } अञ्ञथा {
                inPct = मिच्छा;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nextp = argp ? VN_AS(argp->nextp(), ग्रन्थिः_व्यञ्जकम्) : nullptr;
                AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_ग्रन्थिः_व्यञ्जकम्* subargp = fargp ? fargp->exprp() : argp;
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                विकल्प (ch) {
                पक्ष '?':  // Unspecified by user, guess
                    सचे (dtypep->isDouble()) {
                        ch = 'g';
                    } अञ्ञथा सचे (dtypep->isString()) {
                        ch = 's';
                    } अञ्ञथा सचे (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } अञ्ञथा {
                        ch = 'p';
                    }
                    argp = nextp;
                    भङ्ग;
                पक्ष '%': भङ्ग;  // %% - just output a %
                पक्ष 'm': भङ्ग;  // %m - auto insert "name"
                पक्ष 'l': भङ्ग;  // %m - auto insert "library"
                पक्ष 'd':
                    सचे (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        सचे (subargp->isDouble()) spliceCvtS(subargp, सच्च, 64);
                        argp = nextp;
                    }
                    भङ्ग;
                पक्ष 'b':  // FALLTHRU
                पक्ष 'o':  // FALLTHRU
                पक्ष 'x':
                    सचे (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    भङ्ग;
                पक्ष 't':  // Decimal/float time
                    सचे (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    भङ्ग;
                पक्ष 'e':  // FALLTHRU
                पक्ष 'f':  // FALLTHRU
                पक्ष 'g':
                    सचे (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        सचे (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    भङ्ग;
                पक्ष 'p':  // FALLTHRU
                पक्ष 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    सचे (subargp && !fargp) {
                        सचे (AstEnumDType* ध्रुव enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    भङ्ग;
                स्वभाव:  // Most operators, just move to next argument
                    argp = nextp;
                    भङ्ग;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    स्थिर AstEnumDType* formatEnumDType(Ast_ग्रन्थिः_व्यञ्जकम्* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        सचे (!enumDtp) {
            सचे (ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        पटिनिवत्त enumDtp;
    }

    Ast_व्यञ्जकम्_Stmt* newEnumCapture(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, AstEnumDType* dtypep) {
        File_पङ्क्तिः* ध्रुव flp = valuep->fileline();
        AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(सच्च);
        सचे (m_ftaskp) {
            varp->funcLocal(सच्च);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } अञ्ञथा {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_व्यञ्जकम्_Stmt* ध्रुव exprp = नूतन Ast_व्यञ्जकम्_Stmt{
            flp, नूतन AstAssign{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            नूतन AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        पटिनिवत्त exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, AstEnumDType* dtypep) {
        सचे (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_ग्रन्थिः_व्यञ्जकम्* keyp;
        सचे (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_वृक्षः(मिच्छा);
        } अञ्ञथा {
            Ast_व्यञ्जकम्_Stmt* ध्रुव capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_वृक्षः_Pure(मिच्छा);
        }
        ध्रुव VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ध्रुव argp = नूतन AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        पटिनिवत्त argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    तार्किक widthBad(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_DType* expDTypep) {
        ध्रुव पूर्णाङ्क expWidth = expDTypep->width();
        पूर्णाङ्क expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        सचे (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) पटिनिवत्त मिच्छा;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        सचे (expWidthMin == 0) expWidthMin = expWidth;
        सचे (nodep->dtypep()->width() == expWidth) पटिनिवत्त मिच्छा;
        सचे (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) पटिनिवत्त सच्च;
        सचे (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) पटिनिवत्त सच्च;
        पटिनिवत्त मिच्छा;
    }

    शून्य fixWidthExtend(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, Ast_ग्रन्थिः_DType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<पूर्णाङ्क>(extendRule) << ") old: " << nodep);
        सचे (extendRule == EXTEND_OFF) पटिनिवत्त;
        AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        ध्रुव पूर्णाङ्क expWidth = expDTypep->width();
        सचे (constp && !constp->अङ्कः().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_सङ्ख्या अङ्कः{nodep, expWidth, constp->अङ्कः()};
            अङ्कः.isSigned(मिच्छा);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{nodep->fileline(), अङ्कः};
            constp->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } अञ्ञथा सचे (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(सच्च);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } अञ्ञथा {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            तार्किक doSigned = मिच्छा;
            विकल्प (extendRule) {
            पक्ष EXTEND_ZERO: doSigned = मिच्छा; भङ्ग;
            पक्ष EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); भङ्ग;
            पक्ष EXTEND_LHS: doSigned = nodep->isSigned(); भङ्ग;
            स्वभाव: nodep->v3fatalSrc("bad case");
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
                = (doSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(सच्च);
            linker.relink(newp);
            nodep = newp;
        }
        सचे (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = spliceCvtD(nodep);
            nodep = newp;
        }
        सचे (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } अञ्ञथा {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    शून्य fixWidthReduce(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ध्रुव पूर्णाङ्क expWidth = 1;
        ध्रुव पूर्णाङ्क expSigned = मिच्छा;
        सचे (nodep->width() == expWidth) पटिनिवत्त;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        सचे (constp) {
            V3_सङ्ख्या अङ्कः(nodep, expWidth);
            अङ्कः.opRedOr(constp->अङ्कः());
            अङ्कः.isSigned(expSigned);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{nodep->fileline(), अङ्कः};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_वृक्षः(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } अञ्ञथा {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ग्रन्थिः_व्यञ्जकम्* newp;
            Ast_ग्रन्थिः_DType* ध्रुव dtypeSkipp = nodep->dtypep()->skipRefp();
            सचे (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = नूतन AstNeq{nodep->fileline(),
                                  नूतन AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } अञ्ञथा {
                // But, AstRedOr can be faster on wide
                newp = नूतन AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    तार्किक fixAutoExtend(Ast_ग्रन्थिः_व्यञ्जकम्*& nodepr, पूर्णाङ्क expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        सचे (AstConst* ध्रुव constp = VN_CAST(nodepr, Const)) {
            सचे (constp->अङ्कः().autoExtend() && !constp->अङ्कः().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_सङ्ख्या अङ्कः(constp, expWidth);
                अङ्कः.opRepl(constp->अङ्कः(), expWidth);  // {width{'1}}
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{constp->fileline(), अङ्कः};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_वृक्षः(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                पटिनिवत्त सच्च;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            अञ्ञथा सचे (!constp->अङ्कः().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->अङ्कः().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_सङ्ख्या अङ्कः(constp, expWidth);
                अङ्कः.opExtendXZ(constp->अङ्कः(), constp->width());
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{constp->fileline(), अङ्कः};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_वृक्षः(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                पटिनिवत्त सच्च;
            }
        }
        पटिनिवत्त मिच्छा;  // No change
    }
    तार्किक isBaseClassRecurse(ध्रुव AstClass* ध्रुव cls1p, ध्रुव AstClass* ध्रुव cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        सचे (cls1p == cls2p) पटिनिवत्त सच्च;
        अत्थाय (ध्रुव AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            सचे (isBaseClassRecurse(cls1p, cextp->classp())) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }
    // Lines to end a message on nodep with the data types of its operands
    स्थिर string warnDTypes(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव Ast_ग्रन्थिः_DType* lhsDtp,
                             ध्रुव Ast_ग्रन्थिः_DType* rhsDtp, ध्रुव string& lhsName = "Left-hand",
                             ध्रुव string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        सचे (lhsState == rhsState) lhsState = rhsState = "";
        पटिनिवत्त "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    स्थिर तार्किक unpackedArraySizesMatch(ध्रुव Ast_ग्रन्थिः_DType* lhsDtp, ध्रुव Ast_ग्रन्थिः_DType* rhsDtp) {
        याव (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ध्रुव AstUnpackArrayDType* ध्रुव lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ध्रुव AstUnpackArrayDType* ध्रुव rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            सचे (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                पटिनिवत्त मिच्छा;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        पटिनिवत्त सच्च;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    शून्य checkUnpackedArrayAssignmentCompatible(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव T* ध्रुव lhsRefp,
                                                ध्रुव N* ध्रुव rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::मानम् || std::is_same<T, Ast_ग्रन्थिः_Var_Ref>::मानम्)
                && (std::is_same<N, AstVar>::मानम् || std::is_same<N, Ast_ग्रन्थिः_Var_Ref>::मानम्),
            "Unsupported types provided.");
        सचे (!lhsRefp || !rhsRefp) पटिनिवत्त;
        std::string lhsName;
        std::string rhsName;
        सचे (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } अञ्ञथा {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtp = lhsRefp->dtypep()->skipRefp();
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDtp = rhsRefp->dtypep()->skipRefp();
        ध्रुव तार्किक isLhsAggregate = lhsDtp->isAggregateType();
        ध्रुव तार्किक isRhsAggregate = rhsDtp->isAggregateType();
        सचे (!isLhsAggregate && !isRhsAggregate) पटिनिवत्त;
        सचे (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            पटिनिवत्त;
        } अञ्ञथा सचे (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            पटिनिवत्त;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(मिच्छा),
                                      rhsDim = rhsDtp->dimensions(मिच्छा);
        // Check if unpacked array dimensions are matching
        सचे (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            पटिनिवत्त;
        }

        ध्रुव Ast_ग्रन्थिः_DType* lhsDtpIterp = lhsDtp;
        ध्रुव Ast_ग्रन्थिः_DType* rhsDtpIterp = rhsDtp;
        अत्थाय (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            सचे (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                पटिनिवत्त;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        सचे (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    शून्य checkClassAssign(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* side, Ast_ग्रन्थिः* rhsp,
                          Ast_ग्रन्थिः_DType* ध्रुव lhsDTypep, तार्किक isOutputArg = मिच्छा) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsRawDTypep = lhsDTypep->skipRefp();
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsRawDTypep = rhsp->dtypep()->skipRefp();
        सचे (ध्रुव AstClassRefDType* ध्रुव lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            सचे (ध्रुव AstClassRefDType* ध्रुव rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ध्रुव AstClass* ध्रुव dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ध्रुव AstClass* ध्रुव srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                सचे (isBaseClassRecurse(dstp, srcp)) पटिनिवत्त;
            } अञ्ञथा सचे (rhsp->isNull()) {
                पटिनिवत्त;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } अञ्ञथा सचे (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        सचे (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                नूतन AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, ग्रन्थिः_व्यञ्जकम्), lhsDTypep});
        }
    }
    स्थिर तार्किक similarDTypeRecurse(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव node1p,
                                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव node2p) {
        पटिनिवत्त node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    शून्य iterateCheckFileDesc(Ast_ग्रन्थिः* parentp, Ast_ग्रन्थिः* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_ग्रन्थिः_DType* ध्रुव expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckReal(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    शून्य iterateCheckSigned8(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    शून्य iterateCheckSigned32(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    शून्य iterateCheckUInt32(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    शून्य iterateCheckDelay(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        सचे (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        सचे (stage & FINAL) {
            Ast_ग्रन्थिः_DType* expDTypep;
            सचे (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } अञ्ञथा {
                File_पङ्क्तिः* ध्रुव newFl = नूतन File_पङ्क्तिः{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, सच्च);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, मिच्छा);
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckString(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    शून्य iterateCheckTyped(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp,
                           Ast_ग्रन्थिः_DType* expDTypep, Stage stage) {
        सचे (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        सचे (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckTypedSelfPrelim(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp,
                                     Ast_ग्रन्थिः_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        सचे (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        सचे (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckIntegralSelf(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, सच्च);
    }
    शून्य iterateCheckSelf(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Determ determ,
                          Stage stage, तार्किक integralOnly = मिच्छा) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        सचे (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, ग्रन्थिः_व्यञ्जकम्) ? checkCvtUS(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), integralOnly) : underp;
        Ast_ग्रन्थिः_DType* ध्रुव expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckSizedSelf(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Determ determ,
                               Stage stage, तार्किक deferUntyped = मिच्छा) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        सचे (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, ग्रन्थिः_व्यञ्जकम्) ? checkCvtUS(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), मिच्छा) : underp;
        Ast_ग्रन्थिः_DType* ध्रुव expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_ग्रन्थिः_DType* ध्रुव checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        सचे (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckAssign(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* rhsp, Stage stage,
                            Ast_ग्रन्थिः_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        सचे (AstConst* constp = VN_CAST(rhsp, Const)) {
            सचे (ध्रुव AstUnpackArrayDType* ध्रुव arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                सचे (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    सचे (basicp->width() == 8 && constp->अङ्कः().isFromString()) {
                        AstConsArray* newp = नूतन AstConsArray{
                            constp->fileline(), lhsDTypep,
                            नूतन AstConst{constp->fileline(), AstConst::Widthed_मानम्{}, 8, 0}};
                        अत्थाय (ध्रुव पूर्णाङ्क aindex : arrayp->declRange().seqLoToHi()) {
                            पूर्णाङ्क cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_सङ्ख्या selected{constp, 8};
                            selected.opSel(constp->अङ्कः(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            सचे (!selected.isFourState()) {
                                सचे (ध्रुव uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 नूतन AstConst{constp->fileline(),
                                                              AstConst::Widthed_मानम्{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        ध्रुव तार्किक lhsStream = (VN_IS(parentp, ग्रन्थिः_Assign)
                                && VN_IS(VN_AS(parentp, ग्रन्थिः_Assign)->lhsp(), ग्रन्थिः_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, सच्च, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (शून्य)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_ग्रन्थिः_व्यञ्जकम्* convertUnpackedOrder(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_DType* dtypep) {
        ध्रुव AstUnpackArrayDType* ध्रुव arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        सचे (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) पटिनिवत्त exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        पटिनिवत्त newp;
    }

    स्थिर तार्किक isLogicalOperator(ध्रुव Ast_ग्रन्थिः* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        पटिनिवत्त VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    शून्य iterateCheckBool(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
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
        Ast_ग्रन्थिः_DType* ध्रुव underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        सचे (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन AstNeqD{parentp->fileline(), VN_AS(underp, ग्रन्थिः_व्यञ्जकम्),
                              नूतन AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } अञ्ञथा सचे (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन AstNeqN{parentp->fileline(), VN_AS(underp, ग्रन्थिः_व्यञ्जकम्),
                              नूतन AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } अञ्ञथा सचे (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्)), underp);  // Changed
        } अञ्ञथा सचे (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(नूतन AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(underp), underp);
        } अञ्ञथा {
            ध्रुव तार्किक bad = widthBad(underp, parentp->findBitDType());
            सचे (bad) {
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
            सचे (Ast_व्यञ्जकम्_Stmt* ध्रुव व्यञ्जकम्_Stmtp = VN_CAST(underp, व्यञ्जकम्_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(व्यञ्जकम्_Stmtp->resultp());
                व्यञ्जकम्_Stmtp->dtypeFrom(व्यञ्जकम्_Stmtp->resultp());
            } अञ्ञथा सचे (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्)), underp);  // Changed
            }
        }
    }

    स्थिर शून्य checkEnum(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* parentp, ध्रुव Ast_ग्रन्थिः_DType* targetDTypep,
                          ध्रुव Ast_ग्रन्थिः_DType* sourceDTypep, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* sourceExprp) {
        सचे (ध्रुव AstEnumDType* ध्रुव targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            ध्रुव VCastable castable
                = Ast_ग्रन्थिः::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            सचे (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    Ast_ग्रन्थिः* iterateCheck(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Determ determ,
                          Stage stage, Ast_ग्रन्थिः_DType* expDTypep, ExtendRule extendRule,
                          तार्किक warnOn = सच्च, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ध्रुव StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        सचे (rhsAccess.isReadOnly()) {
            सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_CAST(underp, ग्रन्थिः_व्यञ्जकम्)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        सचे (VN_IS(underp, ग्रन्थिः_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } अञ्ञथा सचे (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } अञ्ञथा सचे (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } अञ्ञथा सचे (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_ग्रन्थिः* ध्रुव oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } अञ्ञथा सचे (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_ग्रन्थिः* ध्रुव oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), सच्च, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } अञ्ञथा सचे (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_ग्रन्थिः* ध्रुव oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } अञ्ञथा {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव underDtp = underp->dtypep()->skipRefp();
            सचे (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_ग्रन्थिः* ध्रुव newp
                    = नूतन AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(underp), underp);
                underp = newp;
            }
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव expDtp = expDTypep->skipRefp();
            सचे (determ == ASSIGN && VN_IS(expDtp, ग्रन्थिः_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            सचे (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ध्रुव AstBasicDType* ध्रुव expBasicp = expDTypep->basicp();
            ध्रुव AstBasicDType* ध्रुव underBasicp = underp->dtypep()->basicp();
            सचे (expBasicp && underBasicp) {
                सचे (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    सचे (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, ग्रन्थिः_व्यञ्जकम्));
                    }
                    सचे (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_ग्रन्थिः_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                सचे (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } अञ्ञथा सचे (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    सचे (underp->isSigned() != subDTypep->isSigned()
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
                } अञ्ञथा {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), expDTypep, extendRule,
                                warnOn);
            } अञ्ञथा सचे (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } अञ्ञथा सचे (ध्रुव AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ध्रुव AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                सचे (VN_IS(underp, Const) && VN_AS(underp, Const)->अङ्कः().isNull()) {
                    // '= null' is ok
                } अञ्ञथा सचे (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } अञ्ञथा सचे (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } अञ्ञथा सचे (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } अञ्ञथा {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } अञ्ञथा {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        पटिनिवत्त underp;
    }

    शून्य
    widthCheckSized(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side,
                    Ast_ग्रन्थिः_व्यञ्जकम्* underp,  // Node to be checked or have typecast added in front of
                    Ast_ग्रन्थिः_DType* expDTypep, ExtendRule extendRule, तार्किक warnOn = सच्च) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ध्रुव AstBasicDType* ध्रुव expBasicp = expDTypep->basicp();
        ध्रुव AstBasicDType* ध्रुव underBasicp = underp->dtypep()->basicp();
        सचे (expDTypep == underp->dtypep()) {
            पटिनिवत्त;  // Same type must match
        } अञ्ञथा सचे (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#सचे VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            पटिनिवत्त;
        } अञ्ञथा {
            ध्रुव पूर्णाङ्क expWidth = expDTypep->width();
            पूर्णाङ्क expWidthMin = expDTypep->widthMin();
            सचे (expWidthMin == 0) expWidthMin = expWidth;
            ध्रुव तार्किक bad = widthBad(underp, expDTypep);
            सचे ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                पटिनिवत्त;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            सचे (VN_IS(parentp->backp(), CastSize)) warnOn = मिच्छा;
            सचे (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = मिच्छा;
            सचे (VN_IS(underp, Const) && VN_AS(underp, Const)->अङ्कः().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = मिच्छा;
            }
            सचे ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = मिच्छा;
            }
            सचे (bad && warnOn) {
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
            सचे (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ध्रुव Ast_ग्रन्थिः_Assign* assignp = VN_CAST(parentp, ग्रन्थिः_Assign);
                ध्रुव AstPin* pinp = VN_CAST(parentp, Pin);
                सचे (assignp && VN_IS(assignp->lhsp(), ग्रन्थिः_Stream)) {
                } अञ्ञथा सचे (pinp && pinp->modVarp()->direction() != VDirection::निवेशः) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } अञ्ञथा सचे (assignp && VN_IS(underp, ग्रन्थिः_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ध्रुव पूर्णाङ्क shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* ध्रुव widenedp = नूतन AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(सच्च);
                    // Shift left so zeros fill on the right
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव shiftedp = नूतन AstShiftL{
                        underp->fileline(), widenedp,
                        नूतन AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } अञ्ञथा {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_ग्रन्थिः_व्यञ्जकम्* checkCvtUS(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, तार्किक fatal) {
        सचे (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            सचे (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } अञ्ञथा {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, मिच्छा, 32);
        }
        पटिनिवत्त nodep;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* spliceCvtD(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        सचे (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ग्रन्थिः_व्यञ्जकम्* newp;
            सचे (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = नूतन AstISToRD{nodep->fileline(), nodep};
            } अञ्ञथा {
                newp = नूतन AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            पटिनिवत्त newp;
        } अञ्ञथा {
            पटिनिवत्त nodep;
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* spliceCvtS(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, तार्किक warnOn, पूर्णाङ्क width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        सचे (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                सचे (constp->isDouble() && V3_सङ्ख्या::epsilonIntegral(constp->अङ्कः().toDouble())) {
                    warnOn = मिच्छा;
                }
            }
            सचे (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            पटिनिवत्त newp;
        } अञ्ञथा {
            पटिनिवत्त nodep;
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* spliceCvtString(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        सचे (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            पटिनिवत्त newp;
        } अञ्ञथा {
            पटिनिवत्त nodep;
        }
    }
    Ast_ग्रन्थिः_Biop* replaceWithUOrSVersion(Ast_ग्रन्थिः_Biop* nodep, तार्किक signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        सचे (signedFlavorNeeded == nodep->signedFlavor()) पटिनिवत्त nullptr;
        सचे (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        विकल्प (nodep->type()) {
        पक्ष VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); पटिनिवत्त nullptr;
        पक्ष VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); पटिनिवत्त nullptr;
        पक्ष VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); पटिनिवत्त nullptr;
        पक्ष VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); पटिनिवत्त nullptr;
        पक्ष VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); पटिनिवत्त nullptr;
        पक्ष VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); पटिनिवत्त nullptr;
        पक्ष VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); पटिनिवत्त nullptr;
        पक्ष VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); पटिनिवत्त nullptr;
        पक्ष VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); पटिनिवत्त nullptr;
        स्वभाव: भङ्ग;
        }
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_Biop* newp = nullptr;
        विकल्प (nodep->type()) {
        पक्ष VNType::Gt: newp = नूतन AstGtS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::GtS: newp = नूतन AstGt{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gte: newp = नूतन AstGteS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::GteS: newp = नूतन AstGte{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lt: newp = नूतन AstLtS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::LtS: newp = नूतन AstLt{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lte: newp = नूतन AstLteS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::LteS: newp = नूतन AstLte{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Div: newp = नूतन AstDivS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::DivS: newp = नूतन AstDiv{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::ModDiv: newp = नूतन AstModDivS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::ModDivS: newp = नूतन AstModDiv{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Mul: newp = नूतन AstMulS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::MulS: newp = नूतन AstMul{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::ShiftR: newp = नूतन AstShiftRS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::ShiftRS: newp = नूतन AstShiftR{fl, lhsp, rhsp}; भङ्ग;
        स्वभाव:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            भङ्ग;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        पटिनिवत्त newp;
    }
    Ast_ग्रन्थिः_Biop* replaceWithDVersion(Ast_ग्रन्थिः_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        सचे (nodep->doubleFlavor()) पटिनिवत्त nullptr;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        विकल्प (nodep->type()) {
        पक्ष VNType::Add: newp = नूतन AstAddD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Sub: newp = नूतन AstSubD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Pow: newp = नूतन AstPowD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Eq:
        पक्ष VNType::EqCase: newp = नूतन AstEqD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Neq:
        पक्ष VNType::NeqCase: newp = नूतन AstNeqD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gt:
        पक्ष VNType::GtS: newp = नूतन AstGtD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gte:
        पक्ष VNType::GteS: newp = नूतन AstGteD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lt:
        पक्ष VNType::LtS: newp = नूतन AstLtD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lte:
        पक्ष VNType::LteS: newp = नूतन AstLteD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Div:
        पक्ष VNType::DivS: newp = नूतन AstDivD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Mul:
        पक्ष VNType::MulS: newp = नूतन AstMulD{fl, lhsp, rhsp}; भङ्ग;
        स्वभाव:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            भङ्ग;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        पटिनिवत्त newp;
    }
    Ast_ग्रन्थिः_Biop* replaceWithNVersion(Ast_ग्रन्थिः_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        सचे (nodep->stringFlavor()) पटिनिवत्त nullptr;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        विकल्प (nodep->type()) {
        पक्ष VNType::Eq:
        पक्ष VNType::EqCase:
        पक्ष VNType::EqWild: newp = नूतन AstEqN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Neq:
        पक्ष VNType::NeqCase:
        पक्ष VNType::NeqWild: newp = नूतन AstNeqN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gt:
        पक्ष VNType::GtS: newp = नूतन AstGtN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gte:
        पक्ष VNType::GteS: newp = नूतन AstGteN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lt:
        पक्ष VNType::LtS: newp = नूतन AstLtN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lte:
        पक्ष VNType::LteS: newp = नूतन AstLteN{fl, lhsp, rhsp}; भङ्ग;
        स्वभाव:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            भङ्ग;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        पटिनिवत्त newp;
    }
    Ast_ग्रन्थिः_Uniop* replaceWithDVersion(Ast_ग्रन्थिः_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        सचे (nodep->doubleFlavor()) पटिनिवत्त nullptr;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_Uniop* newp = nullptr;
        विकल्प (nodep->type()) {
        पक्ष VNType::Negate: newp = नूतन AstNegateD{fl, lhsp}; भङ्ग;
        स्वभाव:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            भङ्ग;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        पटिनिवत्त newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    शून्य replaceWithSFormat(AstMethodCall* nodep, ध्रुव string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ध्रुव argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_ग्रन्थिः_Var_Ref* ध्रुव fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_ग्रन्थिः* ध्रुव newp = नूतन AstAssign{
            nodep->fileline(), fromp,
            नूतन AstSFormatF{nodep->fileline(), format, मिच्छा, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        प्रक्षिप_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_ग्रन्थिः_DType* iterateEditMoveDTypep(Ast_ग्रन्थिः* parentp, Ast_ग्रन्थिः_DType* dtnodep) {
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
        ध्रुव तार्किक child1 = (parentp->getChildDTypep() == dtnodep);
        ध्रुव तार्किक child2 = (parentp->getChild2DTypep() == dtnodep);
        सचे (child1 || child2) {
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
        सचे (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        पटिनिवत्त dtnodep;
    }

    AstConst* dimension_मानम्(File_पङ्क्तिः* fileline, Ast_ग्रन्थिः_DType* nodep, VAttrType attrType,
                             पूर्णाङ्क dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_ग्रन्थिः_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        अत्थाय (पूर्णाङ्क i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            सचे (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव adtypep = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                declRange = adtypep->declRange();
                सचे (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                अनुवर्त;
            } अञ्ञथा सचे (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव adtypep
                       = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                भङ्ग;  // Sub elements don't look like arrays and can't iterate into
            } अञ्ञथा सचे (ध्रुव AstBasicDType* ध्रुव adtypep = VN_CAST(dtypep, BasicDType)) {
                सचे (adtypep->isRanged()) declRange = adtypep->declRange();
                भङ्ग;
            }
            भङ्ग;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        पूर्णाङ्क val = 0;
        विकल्प (attrType) {
        पक्ष VAttrType::DIM_BITS: {
            पूर्णाङ्क bits = 1;
            याव (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                सचे (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव adtypep = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    अनुवर्त;
                } अञ्ञथा सचे (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव adtypep
                           = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    भङ्ग;
                } अञ्ञथा सचे (ध्रुव AstBasicDType* ध्रुव adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    भङ्ग;
                }
                भङ्ग;
            }
            सचे (dim == 0) {
                val = 0;
            } अञ्ञथा सचे (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } अञ्ञथा {
                val = bits;
            }
            भङ्ग;  // LCOV_EXCL_LINE
        }
        पक्ष VAttrType::DIM_उच्चः: val = !declRange.ranged() ? 0 : declRange.hi(); भङ्ग;
        पक्ष VAttrType::DIM_वामः: val = !declRange.ranged() ? 0 : declRange.वामः(); भङ्ग;
        पक्ष VAttrType::DIM_नीचः: val = !declRange.ranged() ? 0 : declRange.lo(); भङ्ग;
        पक्ष VAttrType::DIM_दक्षिणः: val = !declRange.ranged() ? 0 : declRange.दक्षिणः(); भङ्ग;
        पक्ष VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            भङ्ग;
        पक्ष VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); भङ्ग;
        स्वभाव: nodep->v3fatalSrc("Missing DIM ATTR type case"); भङ्ग;
        }
        सचे (!valp) valp = नूतन AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        पटिनिवत्त valp;
    }
    AstVarRef* dimensionVarRefp(Ast_ग्रन्थिः_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        सचे (cachedpr) पटिनिवत्त cachedpr->clone_वृक्षः(मिच्छा);

        Ast_ग्रन्थिः_Array_DType* ध्रुव vardtypep = नूतन AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), नूतन AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* ध्रुव consp = नूतन AstConsArray{nodep->fileline(), vardtypep};
        अत्थाय (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_मानम्(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_वृक्षः(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(सच्च);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(सच्च);
        प्रक्षिप_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        पटिनिवत्त cachedpr->clone_वृक्षः(मिच्छा);
    }
    स्थिर uint64_t enum_Max_मानम्(ध्रुव Ast_ग्रन्थिः* errNodep, ध्रुव AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        अत्थाय (ध्रुव AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ध्रुव AstConst* ध्रुव vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            सचे (!vconstp->अङ्कः().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        सचे (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            पटिनिवत्त 64;
        }
        पटिनिवत्त maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ध्रुव nodep, VAttrType attrType, तार्किक assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        सचे (cachedpr) पटिनिवत्त cachedpr->clone_वृक्षः(मिच्छा);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_DType* basep;
        सचे (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } अञ्ञथा सचे (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } अञ्ञथा {
            basep = nodep->dtypep();
        }
        Ast_ग्रन्थिः_DType* vardtypep;
        सचे (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = नूतन AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } अञ्ञथा {
            vardtypep = नूतन AstUnpackArrayDType{flp, basep, नूतन AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        सचे (attrType == VAttrType::ENUM_NAME) {
            defaultp = नूतन AstConst{flp, AstConst::String{}, ""};
        } अञ्ञथा सचे (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = नूतन AstConst{flp, V3_सङ्ख्या{nodep, nodep->width(), 0}};
        } अञ्ञथा सचे (attrType == VAttrType::ENUM_VALID) {
            defaultp = नूतन AstConst{flp, AstConst::BitFalse{}};
        } अञ्ञथा {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_ग्रन्थिः_व्यञ्जकम्*> मानानि;
        {
            uint64_t ordinal = 0;
            AstEnumItem* ध्रुव firstp = nodep->itemsp();
            ध्रुव AstEnumItem* prevp = firstp;  // Prev must start with last item
            याव (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            अत्थाय (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* ध्रुव nextp = VN_AS(itemp->nextp(), EnumItem);
                ध्रुव AstConst* ध्रुव vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                सचे (!vconstp->अङ्कः().isAnyXZ()) {  // Can 2-state runtime decode
                    ध्रुव uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    सचे (attrType == VAttrType::ENUM_NAME) {
                        मानानि[i] = नूतन AstConst{flp, AstConst::String{},
                                                 V3_सङ्ख्या::displayedEnumName(itemp)};
                    } अञ्ञथा सचे (attrType == VAttrType::ENUM_NEXT) {
                        मानानि[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_वृक्षः(मिच्छा);  // A const
                    } अञ्ञथा सचे (attrType == VAttrType::ENUM_PREV) {
                        मानानि[i] = prevp->valuep()->clone_वृक्षः(मिच्छा);  // A const
                    } अञ्ञथा सचे (attrType == VAttrType::ENUM_VALID) {
                        मानानि[i] = नूतन AstConst{flp, AstConst::BitTrue{}};
                    } अञ्ञथा {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = nullptr;
        सचे (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ध्रुव consp = नूतन AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            अत्थाय (ध्रुव auto& itr : मानानि) {
                V3_सङ्ख्या कुञ्चिका{nodep, nodep->width()};
                कुञ्चिका.setQuad(itr.first);
                consp->addItemsp(नूतन AstConsAssocItem{flp, नूतन AstConst{flp, कुञ्चिका}, itr.second});
            }
            valuep = consp;
        } अञ्ञथा {
            AstConsArray* ध्रुव consp = नूतन AstConsArray{flp, vardtypep, defaultp};
            अत्थाय (uint64_t i = 0; i < (msbdim + 1); ++i) {
                सचे (मानानि[i]) consp->valuep(i, मानानि[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(सच्च);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(सच्च);
        प्रक्षिप_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        पटिनिवत्त cachedpr->clone_वृक्षः(मिच्छा);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* enumSelect(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        सचे (attrType == VAttrType::ENUM_NAME) {
            सचे (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->अङ्कः().displayedEnumName(adtypep)};
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त newp;
            }
        }
        सचे (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_व्यञ्जकम्_Stmt* ध्रुव capturep = newEnumCapture(nodep, adtypep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            पटिनिवत्त capturep;
        }
        सचे (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_ग्रन्थिः_व्यञ्जकम्* indexp = नूतन AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            अत्थाय (ध्रुव AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ध्रुव AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
                सचे (constp->अङ्कः().isAnyXZ()) अनुवर्त;
                indexp = नूतन AstCond{nodep->fileline(),
                                     नूतन AstEq{nodep->fileline(), nodep->clone_वृक्षः(मिच्छा),
                                               itemp->valuep()->clone_वृक्षः(मिच्छा)},
                                     नूतन AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ध्रुव tabRefp = enumVarRefp(adtypep, attrType, मिच्छा, ordinal);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त newp;
        }
        ध्रुव uint64_t msbdim = enum_Max_मानम्(nodep, adtypep);
        ध्रुव तार्किक assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_ग्रन्थिः_व्यञ्जकम्* newp;
        सचे (assoc) {
            AstVarRef* ध्रुव tabRefp = enumVarRefp(adtypep, attrType, सच्च, 0);
            newp = नूतन AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } अञ्ञथा {
            ध्रुव पूर्णाङ्क selwidth = V3_सङ्ख्या::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ध्रुव tabRefp
                = enumVarRefp(adtypep, attrType, मिच्छा, (1ULL << selwidth) - 1);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? नूतन AstGt{nodep->fileline(), nodep->clone_वृक्षः(मिच्छा),
                                  नूतन AstConst{nodep->fileline(),
                                               V3_सङ्ख्या{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = नूतन AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                नूतन AstSel{nodep->fileline(), nodep, 0, selwidth}};
            सचे (boundp) {
                newp = नूतन AstCond{nodep->fileline(), boundp,
                                   नूतन AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        सचे (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } अञ्ञथा सचे (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } अञ्ञथा {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        पटिनिवत्त newp;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* enumTestValid(Ast_ग्रन्थिः_व्यञ्जकम्* valp, AstEnumDType* enumDtp) {
        ध्रुव uint64_t maxval = enum_Max_मानम्(valp, enumDtp);
        ध्रुव तार्किक assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_ग्रन्थिः_व्यञ्जकम्* testp = nullptr;
        File_पङ्क्तिः* ध्रुव fl_novalue = नूतन File_पङ्क्तिः{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, सच्च);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, सच्च);
        सचे (assoc) {
            AstVarRef* ध्रुव tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, सच्च, 0);
            testp = नूतन AstAssocSel{fl_novalue, tabRefp, valp->clone_वृक्षः_Pure(मिच्छा)};
        } अञ्ञथा {
            ध्रुव पूर्णाङ्क selwidth = V3_सङ्ख्या::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ध्रुव tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, मिच्छा, (1ULL << selwidth) - 1);
            File_पङ्क्तिः* ध्रुव fl_nowidth = नूतन File_पङ्क्तिः{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, सच्च);
            testp = नूतन AstCond{
                fl_novalue,
                नूतन AstGt{fl_nowidth, valp->clone_वृक्षः_Pure(मिच्छा),
                          नूतन AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                नूतन AstConst{fl_novalue, AstConst::BitFalse{}},
                नूतन AstArraySel{fl_novalue, tabRefp,
                                नूतन AstSel{fl_novalue, valp->clone_वृक्षः_Pure(मिच्छा), 0, selwidth}}};
        }
        पटिनिवत्त testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, ध्रुव VNumRange& range) {
        PatVecMap patmap;
        पूर्णाङ्क element = range.वामः();
        अत्थाय (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            सचे (patp->keyp()) {
                सचे (ध्रुव AstConst* ध्रुव constp = patternConstKeyp(patp)) {
                    सचे (constp->अङ्कः().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } अञ्ञथा {
                        element = constp->toSInt();
                    }
                }
            }
            ध्रुव तार्किक newEntry = patmap.emplace(element, patp).second;
            सचे (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.वामः_To_दक्षिणः_Inc();
        }
        पटिनिवत्त patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    शून्य patConcatConvertRecurse(AstPattern* patternp, Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (AstConcat* ध्रुव concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } अञ्ञथा {
            AstPatMember* ध्रुव newp
                = नूतन AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(सच्च);
            patternp->addItemsp(newp);
        }
    }

    शून्य makeOpenArrayShell(Ast_ग्रन्थिः_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_ग्रन्थिः_FTask* ध्रुव oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_ग्रन्थिः_FTask* ध्रुव newTaskp = oldTaskp->clone_वृक्षः(मिच्छा);
        newTaskp->dpiOpenChild(सच्च);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            AstVar* ध्रुव portp = tconnect.first;
            ध्रुव AstArg* ध्रुव argp = tconnect.second;
            ध्रुव Ast_ग्रन्थिः* ध्रुव pinp = argp->exprp();
            सचे (!pinp) अनुवर्त;  // Argument error we'll find later
            सचे (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_ग्रन्थिः_DType* ध्रुव oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_ग्रन्थिः* nodep) {
                    सचे (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    तार्किक markHasOpenArray(Ast_ग्रन्थिः_FTask* nodep) {
        तार्किक hasOpen = मिच्छा;
        अत्थाय (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                सचे (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(सच्च);
                    hasOpen = सच्च;
                }
            }
        }
        पटिनिवत्त hasOpen;
    }
    तार्किक hasOpenArrayDTypeRecurse(Ast_ग्रन्थिः_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        सचे (VN_IS(nodep, UnsizedArrayDType)) पटिनिवत्त सच्च;
        सचे (nodep->subDTypep()) पटिनिवत्त hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        पटिनिवत्त मिच्छा;
    }
    Ast_ग्रन्थिः_DType* dtypeNotIntAtomOrVecRecurse(Ast_ग्रन्थिः_DType* nodep, तार्किक ranged = मिच्छा) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        सचे (AstBasicDType* ध्रुव dtp = VN_CAST(nodep, BasicDType)) {
            सचे (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                पटिनिवत्त dtp;  // Packed when already packed
            सचे (dtp->keyword().isIntNumeric()) पटिनिवत्त nullptr;
            पटिनिवत्त dtp;
        } अञ्ञथा सचे (AstPackArrayDType* ध्रुव dtp = VN_CAST(nodep, PackArrayDType)) {
            सचे (ranged) पटिनिवत्त dtp;  // Packed when already packed
            पटिनिवत्त dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), सच्च);
        }
        पटिनिवत्त nodep;
    }
    Ast_ग्रन्थिः_DType* dtypeNot4StateIntegralRecurse(Ast_ग्रन्थिः_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        सचे (AstBasicDType* ध्रुव dtp = VN_CAST(nodep, BasicDType)) {
            सचे (!dtp->keyword().isFourstate()) पटिनिवत्त dtp;
            पटिनिवत्त nullptr;
        } अञ्ञथा सचे (Ast_ग्रन्थिः_Array_DType* ध्रुव dtp = VN_CAST(nodep, ग्रन्थिः_Array_DType)) {
            पटिनिवत्त dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } अञ्ञथा सचे (Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव dtp = VN_CAST(nodep, ग्रन्थिः_UOr_Struct_DType)) {
            अत्थाय (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_ग्रन्थिः_DType* ध्रुव badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                सचे (badDtp) पटिनिवत्त badDtp;
            }
            पटिनिवत्त nullptr;
        }
        पटिनिवत्त nodep;
    }
    शून्य checkForceReleaseLhs(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        सचे (Ast_ग्रन्थिः* ध्रुव selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    शून्य assert_At_व्यञ्जकम्(Ast_ग्रन्थिः* nodep) {
        सचे (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    शून्य assertAtStatement(Ast_ग्रन्थिः* nodep) {
        सचे (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    शून्य checkConstantOrReplace(Ast_ग्रन्थिः* nodep, तार्किक noFourState, ध्रुव string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        सचे (!constp || (noFourState && constp->अङ्कः().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(प्रक्षिप_Deletep(nodep), nodep, constp);
            पटिनिवत्त;
        }
    }
    Ast_ग्रन्थिः* ग्रन्थिः_For_Unsized_Warning(Ast_ग्रन्थिः* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        सचे (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            पटिनिवत्त nodep->op1p();
        } अञ्ञथा सचे (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            पटिनिवत्त nodep->op2p();
        }
        पटिनिवत्त nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_ग्रन्थिः* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        सचे (VL_LIKELY(!nodep)) पटिनिवत्त nullptr;
        सचे (auto* ध्रुव refp = VN_CAST(nodep, RefDType)) {
            सचे (refp->typedefp() == typedefp) पटिनिवत्त refp;
        }
        सचे (auto* ध्रुव refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) पटिनिवत्त refp;
        सचे (auto* ध्रुव refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) पटिनिवत्त refp;
        सचे (auto* ध्रुव refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) पटिनिवत्त refp;
        सचे (auto* ध्रुव refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) पटिनिवत्त refp;
        पटिनिवत्त nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    स्थिर तार्किक isIfaceArrayDType(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        सचे (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) पटिनिवत्त मिच्छा;
        ध्रुव AstIfaceRefDType* ध्रुव irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        पटिनिवत्त irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    स्थिर std::string unpackedArraySize(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        याव (ध्रुव AstUnpackArrayDType* ध्रुव arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            सचे (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        पटिनिवत्त size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newIfaceArrayElement(Ast_ग्रन्थिः_व्यञ्जकम्* fromp, पूर्णाङ्क bit) {
        AstConsArray* ध्रुव consp = VN_CAST(fromp, ConsArray);
        सचे (!consp || !isIfaceArrayDType(consp->dtypep())) पटिनिवत्त nullptr;
        ध्रुव AstUnpackArrayDType* ध्रुव arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        सचे (bit < 0 || bit >= arrp->elementsConst()) पटिनिवत्त nullptr;
        पटिनिवत्त consp->valuep(bit)->clone_वृक्षः(मिच्छा);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_ग्रन्थिः_व्यञ्जकम्* new_Iface_Array_मानम्(ध्रुव Ast_ग्रन्थिः_Var_Ref* refp, Ast_ग्रन्थिः_DType* dtypep,
                                    ध्रुव std::string& name, ध्रुव std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_पङ्क्तिः* ध्रुव flp = refp->fileline();
        AstUnpackArrayDType* ध्रुव arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        सचे (!arrp) {
            AstVar* ध्रुव varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_ग्रन्थिः_Var_Ref* newp;
            सचे (ध्रुव AstVarXRef* ध्रुव xrefp = VN_CAST(refp, VarXRef)) {
                newp = नूतन AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } अञ्ञथा {
                newp = नूतन AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            पटिनिवत्त newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ध्रुव consp = नूतन AstConsArray{flp, arrp};
        ध्रुव VNumRange range = arrp->declRange();
        अत्थाय (ध्रुव पूर्णाङ्क i : range.seq_वामः_To_दक्षिणः()) {
            ध्रुव std::string s = name + "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_मानम्(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        पटिनिवत्त consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    शून्य replaceSplitIfaceArrayRef(Ast_ग्रन्थिः_Var_Ref* nodep) {
        AstVar* ध्रुव varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        सचे (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_मानम्(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    शून्य expandIfaceArrayPinDimensions(AstPin* nodep, ध्रुव Ast_ग्रन्थिः_DType* dtypep,
                                       Ast_ग्रन्थिः_व्यञ्जकम्* conp, ध्रुव std::string& suffix,
                                       AstVar*& elemVarpr) {
        ध्रुव AstUnpackArrayDType* ध्रुव arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        सचे (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ध्रुव newp = nodep->clone_वृक्षः(मिच्छा);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            सचे (conp) newp->exprp(conp->clone_वृक्षः(मिच्छा));
            nodep->addNextHere(newp);
            पटिनिवत्त;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ध्रुव VNumRange range = arrp->declRange();
        अत्थाय (पूर्णाङ्क n = range.elements() - 1; n >= 0; --n) {
            ध्रुव पूर्णाङ्क i = range.वामः() + n * range.वामः_To_दक्षिणः_Inc();
            ध्रुव std::string s = suffix + "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elemConnp = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                सचे (!conp) पटिनिवत्त nullptr;
                ध्रुव AstUnpackArrayDType* ध्रुव conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ध्रुव पूर्णाङ्क elements = conArrp->elementsConst();
                // Counted from lo
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                पटिनिवत्त resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    शून्य expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ध्रुव portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        अत्थाय (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_AS(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्);
        सचे (exprp) प्रक्षिप_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ध्रुव तार्किक connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_ग्रन्थिः* userIterateSubtreeReturnEdits(Ast_ग्रन्थिः* nodep, WidthVP* vup) {
        सचे (!nodep) पटिनिवत्त nullptr;
        Ast_ग्रन्थिः* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        पटिनिवत्त ret;
    }
    शून्य userIterate(Ast_ग्रन्थिः* nodep, WidthVP* vup) {
        सचे (!nodep) पटिनिवत्त;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    शून्य userIterateAndNext(Ast_ग्रन्थिः* nodep, WidthVP* vup) {
        सचे (!nodep) पटिनिवत्त;
        सचे (nodep->didWidth()) पटिनिवत्त;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    शून्य userIterateChildren(Ast_ग्रन्थिः* nodep, WidthVP* vup) {
        सचे (!nodep) पटिनिवत्त;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    शून्य userIterateChildrenBackwardsConst(Ast_ग्रन्थिः* nodep, WidthVP* vup) {
        सचे (!nodep) पटिनिवत्त;
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
    Ast_ग्रन्थिः* mainAcceptEdit(Ast_ग्रन्थिः* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        अत्थाय (AstVar* ध्रुव varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        पटिनिवत्त nodep;
    }
    स्थिर तार्किक lowerAsFixedAggregate(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        पटिनिवत्त dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    शून्य streamAssignLower(Ast_ग्रन्थिः_Assign* nodep) {
        Ast_ग्रन्थिः_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        सचे (Ast_ग्रन्थिः_Stream* ध्रुव streamp = VN_CAST(nodep->rhsp(), ग्रन्थिः_Stream)) {
            Ast_ग्रन्थिः_DType* ध्रुव lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ध्रुव पूर्णाङ्क lwidth = lhsDTypeSkippedRefp->widthStream();
            ध्रुव पूर्णाङ्क rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            सचे (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            सचे (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                ध्रुव पूर्णाङ्क queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            सचे (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    नूतन AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        सचे (Ast_ग्रन्थिः_Stream* ध्रुव streamp = VN_CAST(nodep->lhsp(), ग्रन्थिः_Stream)) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_ग्रन्थिः_DType* ध्रुव lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            ध्रुव पूर्णाङ्क lwidth = lhsStreamDTypep->widthStream();
            ध्रुव पूर्णाङ्क rwidth = rhsDTypep->widthStream();
            सचे (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            सचे (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dstp = streamp->lhsp()->unlinkFrBack();
                Ast_ग्रन्थिः_व्यञ्जकम्* srcp = nodep->rhsp()->unlinkFrBack();
                सचे (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } अञ्ञथा {
                    सचे (srcp->width() > lwidth) {
                        srcp = नूतन AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(प्रक्षिप_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(नूतन AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } अञ्ञथा सचे (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(नूतन AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = स्वभाव;
};

//######################################################################
// Width class functions

शून्य V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ध्रुव WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ध्रुव VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{मिच्छा, मिच्छा};
        (शून्य)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (शून्य)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("width", 0, dump_वृक्षः_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_ग्रन्थिः* V3Width::widthParamsEdit(Ast_ग्रन्थिः* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{सच्च, मिच्छा};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    पटिनिवत्त nodep;
}

शून्य V3Width::streamAssignLowerEdit(Ast_ग्रन्थिः_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{मिच्छा, मिच्छा};
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
Ast_ग्रन्थिः* V3Width::widthGenerateParamsEdit(
    Ast_ग्रन्थिः* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{सच्च, सच्च};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    पटिनिवत्त nodep;
}

Ast_ग्रन्थिः* V3Width::selectNonConstantRecurse(Ast_ग्रन्थिः* nodep, तार्किक inSel) {
    // If node has a non-constant select, return that select
    Ast_ग्रन्थिः* resultp = nullptr;
    सचे (Ast_ग्रन्थिः_Sel* ध्रुव anodep = VN_CAST(nodep, ग्रन्थिः_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        सचे (resultp) पटिनिवत्त resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), सच्च);
    } अञ्ञथा सचे (AstSel* ध्रुव anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        सचे (resultp) पटिनिवत्त resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), सच्च);
    } अञ्ञथा सचे (Ast_ग्रन्थिः_Var_Ref* ध्रुव anodep = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
        सचे (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) पटिनिवत्त anodep;
    } अञ्ञथा {
        सचे (Ast_ग्रन्थिः* ध्रुव refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        सचे (resultp) पटिनिवत्त resultp;
        सचे (Ast_ग्रन्थिः* ध्रुव refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        सचे (resultp) पटिनिवत्त resultp;
        सचे (Ast_ग्रन्थिः* ध्रुव refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        सचे (resultp) पटिनिवत्त resultp;
        सचे (Ast_ग्रन्थिः* ध्रुव refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    पटिनिवत्त resultp;
}
