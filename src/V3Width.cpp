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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Width.h"

#समावेश "V3Assert.h"
#समावेश "V3Ast.h"
#समावेश "V3Begin.h"
#समावेश "V3Const.h"
#समावेश "V3ConstPool.h"
#समावेश "V3Error.h"
#समावेश "V3Global.h"
#समावेश "V3LinkLValue.h"
#समावेश "V3MemberMap.h"
#समावेश "V3Number.h"
#समावेश "V3Randomize.h"
#समावेश "V3String.h"
#समावेश "V3Task.h"
#समावेश "V3UniqueNames.h"
#समावेश "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#घोषणा VERILATOR_V3WIDTH_CPP_
#समावेश "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<स्थिर Ast_गाँठ*, AstClass*> m_cache;

    AstClass* findCached(Ast_गाँठ* nodep) {
        अगर (!nodep) वापस nullptr;
        स्थिर स्वतः it = m_cache.find(nodep);
        अगर (it != m_cache.end()) वापस it->second;

        AstClass* classp = nullptr;
        अगर (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } अन्यथा अगर (AstClass* स्थिर parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } अन्यथा अगर (AstClassPackage* स्थिर packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } अन्यथा {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        वापस classp;
    }

खुला:
    AstClass* find(Ast_गाँठ* nodep) {
        अगर (AstClass* स्थिर classp = VN_CAST(nodep, Class)) वापस classp;
        अगर (AstClassPackage* स्थिर packagep = VN_CAST(nodep, ClassPackage)) {
            वापस packagep->classp();
        }
        वापस findCached(nodep);
    }
};

//######################################################################

क्रमागत Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ब_प्रवाह& चालक<<(std::ब_प्रवाह& str, स्थिर Stage& rhs) {
    वापस str << ("-PFB"[static_cast<पूर्णांक>(rhs)]);
}

क्रमागत Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ब_प्रवाह& चालक<<(std::ब_प्रवाह& str, स्थिर Determ& rhs) {
    अटल constexpr स्थिर अक्षर* स्थिर s_det[] = {"SELF", "CNTX", "ASSN"};
    वापस str << s_det[rhs];
}

क्रमागत StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#घोषणा v3widthWarn(lhs, rhs, संदेश) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               संदेश)

//######################################################################
// Width state, as a visitor of each AstNode

श्रेणी WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_गाँठ_DType* स्थिर m_dtypep;  // Parent's data type to resolve to
    स्थिर Stage m_stage;  // If true, report errors
    स्थिर StreamUse m_streamUse;  // Current expression may be a stream concat

खुला:
    WidthVP(Ast_गाँठ_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        अगर (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { वापस यह; }
    bool selfDtm() स्थिर { वापस m_dtypep == nullptr; }
    StreamUse streamUse() स्थिर { वापस m_streamUse; }
    Ast_गाँठ_DType* dtypep() स्थिर {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        वापस m_dtypep;
    }
    Ast_गाँठ_DType* dtypeNullp() स्थिर { वापस m_dtypep; }
    Ast_गाँठ_DType* dtypeNullSkipRefp() स्थिर {
        Ast_गाँठ_DType* dtp = dtypeNullp();
        अगर (dtp) dtp = dtp->skipRefp();
        वापस dtp;
    }
    Ast_गाँठ_DType* dtypeOverridep(Ast_गाँठ_DType* स्थिर defaultp) स्थिर {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        अगर (!m_dtypep) वापस defaultp;
        अगर (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            वापस defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        वापस m_dtypep;
    }
    पूर्णांक width() स्थिर {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        वापस m_dtypep->width();
    }
    पूर्णांक widthMin() स्थिर {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        वापस m_dtypep->widthMin();
    }
    bool prelim() स्थिर { वापस m_stage & PRELIM; }
    bool final() स्थिर { वापस m_stage & FINAL; }
    व्योम dump(std::ब_प्रवाह& str) स्थिर {
        अगर (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } अन्यथा {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ब_प्रवाह& चालक<<(std::ब_प्रवाह& str, स्थिर WidthVP* vup) {
    अगर (vup) vup->dump(str);
    वापस str;
}

//######################################################################

श्रेणी WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    व्योम clearWidthRecurse(Ast_गाँठ* nodep) {
        क्रम (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(false);
            अगर (Ast_गाँठ* स्थिर refp = nodep->op1p()) clearWidthRecurse(refp);
            अगर (Ast_गाँठ* स्थिर refp = nodep->op2p()) clearWidthRecurse(refp);
            अगर (Ast_गाँठ* स्थिर refp = nodep->op3p()) clearWidthRecurse(refp);
            अगर (Ast_गाँठ* स्थिर refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

खुला:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    भव ~WidthClearVisitor() = शेष;
};

//######################################################################

#घोषणा accept in_Width_Visitor_use_Ast_गाँठ_iterate_instead_of_Ast_गाँठ_accept

//######################################################################

श्रेणी WidthVisitor final : खुला VNVisitor {
    // TYPES
    using PatVecMap = std::map<पूर्णांक, AstPatMember*>;
    using DTypeMap = std::map<स्थिर std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enum_अस्थायी_Names;  // Captured enum expressions
    V3UniqueNames m_inside_अस्थायी_Names;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<स्थिर AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    bool m_underFork = false;  // Visiting under a fork
    bool m_underSExpr = false;  // Visiting under a sequence expression
    bool m_underPackedArray = false;  // Visiting under a AstPackArrayDType
    bool m_underMemberSel = false;  // Viting under a MemberSel
    Ast_गाँठ* m_seqUnsupp = nullptr;  // Property has unsupported node
    bool m_hasSExpr = false;  // Property has a sequence expression
    स्थिर AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    स्थिर AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाँठ_Module* m_modep = nullptr;  // Current module
    स्थिर AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_गाँठ_Procedure* m_procedurep = nullptr;  // Current final/always
    स्थिर AstWith* m_withp = nullptr;  // Current 'with' statement
    स्थिर AstFunc* m_funcp = nullptr;  // Current function
    स्थिर AstAttrOf* m_attrp = nullptr;  // Current attribute
    स्थिर Ast_गाँठ_व्यंजक* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    स्थिर bool m_paramsOnly;  // Computing parameter value; limit operation
    स्थिर bool m_doGenerate;  // Do errors later inside generate statement
    bool m_streamConcat = false;  // True if visiting arguments of stream concatenation
    bool m_inParamOverride = false;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<स्थिर Ast_गाँठ_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<स्थिर Ast_गाँठ_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<स्थिर AstVar*> m_curModVars;  // Variables declared in current module

    अटल constexpr पूर्णांक ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    क्रमागत ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    क्रमागत श्रेणी ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    अटल StreamUse streamUseForDeterm(Determ determ) {
        वापस determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() स्थिर { वापस m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    अटल व्योम packIfUnpacked(Ast_गाँठ_व्यंजक* स्थिर nodep) {
        अगर (AstUnpackArrayDType* स्थिर unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            स्थिर पूर्णांक elements_अंक = unpackDTypep->arrayUnpackedElements();
            स्थिर पूर्णांक unpackMinBits = elements_अंक * unpackDTypep->subDTypep()->widthMin();
            स्थिर पूर्णांक unpackBits = elements_अंक * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(नया AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    अटल Ast_गाँठ_DType* fromDTypep(Ast_गाँठ* fromp) {
        अगर (Ast_गाँठ_DType* स्थिर dtypep = VN_CAST(fromp, गाँठ_DType))
            वापस dtypep->skipRefOrNullp();
        वापस fromp ? fromp->dtypep() : nullptr;
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
    व्योम visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    व्योम visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    व्योम visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    व्योम visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    व्योम visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    व्योम visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    व्योम visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    व्योम visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    व्योम visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    व्योम visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    व्योम visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    व्योम visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    व्योम visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    व्योम visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    व्योम visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    व्योम visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    व्योम visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    व्योम visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    व्योम visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    व्योम visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    व्योम visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    व्योम visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    व्योम visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    व्योम visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    व्योम visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    व्योम visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    व्योम visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    व्योम visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    व्योम visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    व्योम visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    व्योम visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    व्योम visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    व्योम visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    व्योम visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    व्योम visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    व्योम visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    व्योम visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    व्योम visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    व्योम visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    व्योम visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    व्योम visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    व्योम visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    व्योम visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    व्योम visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    व्योम visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    व्योम visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    व्योम visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    व्योम visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, true, false); }
    व्योम visit(AstSub* nodep) override { visit_add_sub_replace(nodep, true, false); }
    व्योम visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, true, true); }
    व्योम visit(AstMul* nodep) override { visit_add_sub_replace(nodep, true, false); }
    // These can't promote to real
    व्योम visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, false, true); }
    व्योम visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, false, true); }
    व्योम visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, false, false); }
    व्योम visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, false, true); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    व्योम visit(AstNegate* nodep) override { visit_negate_not(nodep, true); }
    // Unary never real
    व्योम visit(AstNot* nodep) override { visit_negate_not(nodep, false); }

    // Real: inputs and output real
    व्योम visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    व्योम visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    व्योम visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    व्योम visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    व्योम visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    व्योम visit(Ast_गाँठ_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    व्योम visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    व्योम visit(Ast_गाँठ_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    व्योम visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    व्योम visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    व्योम visit(AstShiftL* nodep) override { visit_shift(nodep); }
    व्योम visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    व्योम visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    व्योम visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    व्योम visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    व्योम visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    व्योम visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    व्योम visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        अगर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    व्योम visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        अगर (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    व्योम visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        अगर (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    व्योम visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        अगर (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    व्योम visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        अगर (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    व्योम visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    व्योम visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            अगर (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } अन्यथा {
                nodep->dtypeSetInteger();
            }
        }
    }
    व्योम visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    व्योम visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    व्योम visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    व्योम visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    व्योम visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    व्योम visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            नया AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    अटल bool isOverridePattern(स्थिर Ast_गाँठ* nodep) {
        अगर (स्थिर AstPattern* स्थिर patternp = VN_CAST(nodep, Pattern)) {
            वापस !patternp->childDTypep();
        }
        स्थिर AstCond* स्थिर condp = VN_CAST(nodep, Cond);
        वापस condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    व्योम visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_व्यंजक(nodep);
        अगर (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            स्थिर पूर्णांक errors = V3Error::error_गिनती();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = false;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            स्थिर AstConst* स्थिर constp = VN_CAST(nodep->condp(), Const);
            स्थिर bool known = constp && !constp->अंक().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            अगर (!known && V3Error::error_गिनती() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_गाँठ_व्यंजक* स्थिर keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            वापस;
        }
        अगर (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            user_Iterate_And_अग्र(nodep->thenp(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            user_Iterate_And_अग्र(nodep->elsep(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_गाँठ_DType* स्थिर thenDTypep = nodep->thenp()->dtypep();
            Ast_गाँठ_DType* स्थिर elseDTypep = nodep->elsep()->dtypep();
            Ast_गाँठ_DType* स्थिर thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_गाँठ_DType* स्थिर elseDTypeSkippedp = elseDTypep->skipRefp();
            अगर (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } अन्यथा अगर (nodep->thenp()->is_Class_Handle_मूल्य()
                       || nodep->elsep()->is_Class_Handle_मूल्य()) {
                Ast_गाँठ_DType* commonClassTypep = nullptr;
                अगर (nodep->thenp()->is_Class_Handle_मूल्य() && nodep->elsep()->is_Class_Handle_मूल्य()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_गाँठ::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                अगर (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } अन्यथा {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } अन्यथा अगर (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                अगर (Ast_गाँठ::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } अन्यथा {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    अगर (!Ast_गाँठ::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } अन्यथा अगर (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } अन्यथा अगर (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } अन्यथा {
                स्थिर bool is_Any_परिणाम_Fourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                bool handled = false;
                स्थिर bool isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_गाँठ_DType* स्थिर thenEnumDTypep = thenDTypep->skipRefToEnump();
                अगर (VN_IS(thenDTypeSkippedp, गाँठ_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_वृक्ष(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = true;
                } अन्यथा अगर (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_वृक्ष(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    अगर (is_Any_परिणाम_Fourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = true;
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
                        nodep->fileline(नया File_पंक्ति{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, true);
                    }
                }
                अगर (!handled) {
                    स्थिर पूर्णांक width = std::max(thenDTypep->width(), elseDTypep->width());
                    स्थिर पूर्णांक widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    स्थिर VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    स्थिर bool isFourstate = is_Any_परिणाम_Fourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    अगर ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } अन्यथा अगर ((width == thenDTypeSkippedp->width()
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
        अगर (m_vup->final()) {
            Ast_गाँठ_DType* स्थिर expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाँठ_DType* स्थिर subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    व्योम visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_गाँठ_DType* स्थिर vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        अगर (Ast_गाँठ_व्यंजक* स्थिर newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        अगर (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* स्थिर newp = नया AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            वापस;
        }

        // Concat handling
        अगर (m_vup->prelim()) {
            अगर (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            अगर (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } अन्यथा {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                अगर (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_गाँठ_व्यंजक::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            अगर (स्थिर AstReplicate* स्थिर repp = VN_CAST(nodep->lhsp(), Replicate)) {
                अगर (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
            }
            अगर (स्थिर AstReplicate* स्थिर repp = VN_CAST(nodep->rhsp(), Replicate)) {
                अगर (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
            }
        }
        अगर (m_vup->final()) {
            अगर (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_गाँठ_व्यंजक* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_गाँठ_व्यंजक* rhsp = nodep->rhsp()->unlinkFrBack();
                अगर (!lhsp->isString()) lhsp = नया AstCvtPackString{lhsp->fileline(), lhsp};
                अगर (!rhsp->isString()) rhsp = नया AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_गाँठ* स्थिर newp = नया AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            }
            अगर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाँठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    व्योम visit(AstConcatN* nodep) override {
        अगर (nodep->didWidth()) वापस;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        अगर (m_vup->final()) {
            nodep->didWidth(true);
            अगर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाँठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    व्योम visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    व्योम visit(AstDelay* nodep) override {
        अगर (nodep->isCycleDelay() && m_underSExpr) {
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            अगर (nodep->rhsp() && !nodep->isUnbounded()) {
                user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            अगर (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                वापस;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            वापस;
        }
        अगर (Ast_गाँठ_व्यंजक* स्थिर fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            वापस;
        }
        अगर (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        अगर (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        अगर (nodep->fileline()->timingOn()) {
            अगर (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterate_And_अग्र_Null(nodep->stmtsp());
                वापस;
            } अन्यथा अगर (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } अन्यथा {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        अगर (nodep->stmtsp()) nodep->add_अग्र_Here(nodep->stmtsp()->unlink_Fr_Back_With_अग्र());
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    व्योम visit(AstFork* nodep) override {
        अगर (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        अगर (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* स्थिर newp = नया AstBegin{nodep->fileline(), nodep->name(), nullptr, false};
            nodep->replaceWith(newp);
            अगर (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlink_Fr_Back_With_अग्र());
            अगर (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlink_Fr_Back_With_अग्र());
            अगर (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlink_Fr_Back_With_अग्र());
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा अगर (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = true;
            iterateChildren(nodep);
        } अन्यथा अगर (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    व्योम visitWaitOrDisableFork(Ast_गाँठ* nodep) {
        अगर (nodep->fileline()->timingOn()) {
            अगर (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                वापस;
            } अन्यथा अगर (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    व्योम visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    व्योम visit(AstToLowerN* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    व्योम visit(AstToUpperN* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    व्योम visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t बार = 1;  // IEEE replicate value is integral

            स्थिर AstConst* स्थिर constp = VN_CAST(nodep->countp(), Const);
            अगर (constp) {
                अगर (constp->अंक().isFourState()
                    || (constp->dtypep()->isSigned() && constp->अंक().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } अन्यथा {
                    बार = constp->toSInt();
                }
            }

            Ast_गाँठ_DType* स्थिर vdtypep = m_vup->dtypeNullSkipRefp();
            अगर (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                अगर (बार != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                अगर (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    user_Iterate_And_अग्र(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                } अन्यथा {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* स्थिर newp = नया AstPattern{
                        nodep->fileline(),
                        नया AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    वापस;
                }
            }
            अगर (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            अगर (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } अन्यथा {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            अगर ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_गाँठ* स्थिर newp
                    = नया AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            } अन्यथा {
                अगर (!constp) nodep->v3error("Replication value isn't a constant.");
                अगर (बार == 0
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
        अगर (m_vup->final()) {
            अगर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाँठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    व्योम visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        अगर (m_vup->final()) {
            अगर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाँठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    व्योम visit(Ast_गाँठ_Dist_Biop* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    व्योम visit(Ast_गाँठ_Dist_Triop* nodep) override {
        अगर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    bool streamImplicitUseAllowed(स्थिर Ast_गाँठ_Stream* nodep) स्थिर {
        अगर (m_streamConcat) वापस true;
        अगर (m_vup && m_vup->streamUse() != STREAM_USE_NONE) वापस true;

        स्थिर Ast_गाँठ* स्थिर backp = nodep->backp();
        अगर (!backp) वापस true;  // Error elsewhere

        वापस VN_IS(backp, गाँठ_Assign);
    }
    व्योम visit(Ast_गाँठ_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            m_streamConcat = true;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = false;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep->rhsp(), Const)) {
                अगर (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } अन्यथा {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            स्थिर Ast_गाँठ_DType* स्थिर lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            अगर (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, गाँठ_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, गाँठ_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } अन्यथा अगर (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } अन्यथा {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_गाँठ_व्यंजक::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        अगर (m_vup->final()) {
            अगर (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            अगर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाँठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    व्योम visit(AstRange* nodep) override {
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
        अगर (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            स्थिर पूर्णांक width = nodep->elementsConst();
            अगर (nodep->fromBracket() && nodep->बायाँ_Const() > nodep->दायाँ_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } अन्यथा अगर (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::षष्ठ << width << std::डेसी);
            }
            // Note width() not set on range; use elementsConst()
            स्थिर bool inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            स्थिर bool inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            स्थिर bool inTypeTable = !m_modep;
            स्थिर Ast_गाँठ* basep = nodep->backp();
            जबतक (VN_IS(basep, Range)) basep = basep->backp();
            अगर (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->बायाँ_Const() << ":" << nodep->दायाँ_Const()
                                            << "]");
            }
        }
    }

    व्योम visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        अगर (nodep->didWidth()) वापस;
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            user_Iterate_And_अग्र(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), false);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            पूर्णांक width = nodep->widthConst();
            अगर (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                वापस;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            स्थिर bool isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            स्थिर bool inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            अगर (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                धकेलो_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(नया AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            अगर (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                अगर (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_गाँठ_DType* स्थिर subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    false /*noerror*/);
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
            स्थिर पूर्णांक elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            अगर (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } अन्यथा {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            स्थिर पूर्णांक selwidth = V3_संख्या::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_गाँठ_DType* स्थिर selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            user_Iterate_And_अग्र(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            अगर (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                अगर (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            अगर (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                अगर (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } अन्यथा अगर (!inParameterizedTemplate) {
                    अगर (nodep->declRange().ranged()) {
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
                अगर (!isWriteSelect) {
                    // Extend it
                    स्थिर पूर्णांक extendTo = nodep->msbConst() + 1;
                    Ast_गाँठ_DType* स्थिर subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    false /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            अगर (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                false /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    व्योम visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            अगर (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep->bitp(), Const)) {
                    स्थिर पूर्णांक bit = constp->toSInt();
                    Ast_गाँठ_व्यंजक* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    अगर (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        स्थिर AstUnpackArrayDType* स्थिर arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
            }
            //
            पूर्णांक frommsb;
            पूर्णांक fromlsb;
            स्थिर Ast_गाँठ_DType* स्थिर fromDtp = nodep->fromp()->dtypep()->skipRefp();
            अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                अगर (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                अगर (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } अन्यथा {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            स्थिर पूर्णांक selwidth = V3_संख्या::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_गाँठ_DType* स्थिर selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            अगर (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                अगर (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            अगर (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                अगर (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    अगर (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                false /*NOWARN*/);
            }
        }
    }

    व्योम visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            स्थिर Ast_गाँठ_DType* स्थिर fromDtp = nodep->fromp()->dtypep()->skipRefp();
            स्थिर AstAssocArrayDType* स्थिर adtypep = VN_CAST(fromDtp, AssocArrayDType);
            अगर (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->कुंजी_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    व्योम visit(AstAlias* nodep) override {
        अगर (!nodep->didWidthAndSet()) {
            user_Iterate_And_अग्र(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        स्थिर स्वतः check_If_व्यंजक_Ok = [यह](स्थिर Ast_गाँठ_व्यंजक* स्थिर exprp) {
            अगर (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                वापस false;
            }

            स्थिर AstVarRef* स्थिर varRefp = VN_CAST(exprp, VarRef);
            अगर (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                वापस false;
            }
            AstVar* स्थिर varp = varRefp->varp();
            अगर (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                वापस false;
            }
            अगर (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                वापस false;
            } अन्यथा {
                m_aliasedVars.जोड़ो(varp);
            }
            वापस true;
        };

        check_If_व्यंजक_Ok(nodep->itemsp());
        Ast_गाँठ_DType* स्थिर firstItemDtypep = nodep->itemsp()->dtypep();
        क्रम (Ast_गाँठ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_व्यंजक_Ok(VN_AS(itemp, गाँठ_व्यंजक));
            अगर (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    व्योम visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            स्थिर Ast_गाँठ_DType* स्थिर fromDtp = nodep->fromp()->dtypep()->skipRefp();
            स्थिर AstWildcardArrayDType* स्थिर adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            अगर (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            स्थिर AstBasicDType* स्थिर basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            अगर (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    व्योम visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            स्थिर Ast_गाँठ_DType* स्थिर fromDtp = nodep->fromp()->dtypep()->skipRefp();
            स्थिर AstUnpackArrayDType* स्थिर adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            अगर (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_गाँठ_DType* स्थिर newDtp
                = नया AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          नया AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            अगर (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                स्थिर पूर्णांक subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                स्थिर VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                अगर ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(नया AstConsArray{
                        nodep->fileline(), newDtp,
                        नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                } अन्यथा अगर ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            अगर (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* स्थिर fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* स्थिर newp = नया AstConsArray{nodep->fileline(), newDtp};
                स्थिर VNumRange range = nodep->declRange();
                क्रम (पूर्णांक n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_वृक्ष(false));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            }
        }
    }

    व्योम visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_अग्र(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_गाँठ* स्थिर selp = V3Width::widthSelNoIterEdit(nodep);
        अगर (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            वापस;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    व्योम visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_अग्र(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_गाँठ* स्थिर selp = V3Width::widthSelNoIterEdit(nodep);
        अगर (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            वापस;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    व्योम visit(AstSelPlus* nodep) override {
        user_Iterate_And_अग्र(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_गाँठ* स्थिर selp = V3Width::widthSelNoIterEdit(nodep);
        अगर (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            वापस;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    व्योम visit(AstSelMinus* nodep) override {
        user_Iterate_And_अग्र(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_गाँठ* स्थिर selp = V3Width::widthSelNoIterEdit(nodep);
        अगर (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            वापस;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    व्योम visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        अगर (nodep->didWidthAndSet()) वापस;
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        अगर (nodep->didWidthAndSet()) वापस;
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        अगर (nodep->didWidthAndSet()) वापस;
        अगर (m_vup && m_vup->prelim()) {
            अगर (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } अन्यथा अगर (nodep->अंक().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } अन्यथा अगर (nodep->अंक().isString()) {
                nodep->dtypeSetString();
            } अन्यथा अगर (nodep->अंक().isDouble()) {
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
    व्योम visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        अगर (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    व्योम visit(AstFell* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstFalling* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstFuture* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    व्योम visit(AstPast* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeFrom(nodep->exprp());
            अगर (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ true);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                स्थिर AstConst* स्थिर constp = VN_CAST(nodep->ticksp(), Const);
                अगर (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_वृक्ष();
                } अन्यथा अगर (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_वृक्ष();
                } अन्यथा अगर (V3AssertCommon::check_Sva_मूल्य_Limit(nodep, constp, "$past tick value")) {
                    अगर (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    व्योम visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            स्थिर AstConst* स्थिर minConstp = VN_CAST(nodep->countp(), Const);
            अगर (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } अन्यथा अगर (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } अन्यथा अगर (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            अगर (nodep->maxCountp()) {
                user_Iterate_And_अग्र(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                स्थिर AstConst* स्थिर maxConstp = VN_CAST(nodep->maxCountp(), Const);
                अगर (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } अन्यथा अगर (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } अन्यथा अगर (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_गाँठ_व्यंजक* propp = nodep->propp();
            अगर (!VN_IS(nodep->loBoundp(), Unbounded)) {
                user_Iterate_And_अग्र(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            अगर (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                user_Iterate_And_अग्र(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            स्थिर bool loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            स्थिर bool hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            अगर (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_गाँठ* स्थिर boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                वापस;
            }
            अगर (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                वापस;
            }
            स्थिर AstConst* स्थिर loConstp = VN_CAST(nodep->loBoundp(), Const);
            स्थिर AstConst* स्थिर hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            अगर (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            अगर (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            अगर (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } अन्यथा अगर (loConstp) {
                V3AssertCommon::check_Sva_मूल्य_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            अगर (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_मूल्य_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            अगर (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            bool hasPropertyOp = propp->isMultiCycleSva();
            अगर (!hasPropertyOp) {
                propp->foreach([&](स्थिर Ast_गाँठ* np) {
                    अगर (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = true;
                    }
                });
            }
            अगर (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstRising* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstRose* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    व्योम visit(AstSampled* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    व्योम visit(AstSetuphold* nodep) override {
        File_पंक्ति* स्थिर flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        अगर (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        अगर (nodep->deldatap()) {
            अगर (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } अन्यथा {
                newp->add_अग्र_Here(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        अगर (!newp) {
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        nodep->replaceWith(नया AstAlways{newp});
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_पंक्ति* स्थिर flp, Ast_गाँठ_व्यंजक* स्थिर evp,
                                         Ast_गाँठ_व्यंजक* स्थिर delp) {
        Ast_गाँठ_व्यंजक* स्थिर lhsp = delp->clone_वृक्ष_Pure(false);
        Ast_गाँठ_व्यंजक* स्थिर rhsp = evp->clone_वृक्ष_Pure(false);
        UASSERT_OBJ(VN_IS(lhsp, गाँठ_Var_Ref) || VN_IS(lhsp, गाँठ_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        अगर (Ast_गाँठ_Var_Ref* varRefp = VN_CAST(lhsp, गाँठ_Var_Ref)) {
            अगर (varRefp->varp()->direction() == VDirection::निवेश) {
                VL_DO_DANGLING(धकेलो_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(धकेलो_Deletep(rhsp), rhsp);
                वापस nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        अगर (Ast_गाँठ_Pre_Sel* selp = VN_CAST(lhsp, गाँठ_Pre_Sel)) {
            अगर (Ast_गाँठ_Var_Ref* varRefp = VN_CAST(selp->fromp(), गाँठ_Var_Ref)) {
                अगर (varRefp->varp()->direction() == VDirection::निवेश) {
                    VL_DO_DANGLING(धकेलो_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(धकेलो_Deletep(rhsp), rhsp);
                    वापस nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        वापस नया AstAssignW{flp, lhsp, rhsp};
    }

    व्योम visit(AstStable* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstSteady* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeSetBit();
        }
    }

    व्योम visit(Ast_Stmt_व्यंजक* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    व्योम visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    व्योम visit(AstUntil* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            अगर (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            अगर (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    व्योम visit(AstRand* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            अगर (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } अन्यथा {
                nodep->dtypeSetInt();  // Says the spec
            }
            अगर (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    व्योम visit(AstSEventually* nodep) override {
        अगर (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            वापस;
        }
        अगर (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstSGotoRep* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_अग्र(nodep->countp(), WidthVP{SELF, BOTH}.p());
            अगर (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } अन्यथा {
                width_Check_Rep_गिनती(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstSNonConsRep* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_अग्र(nodep->countp(), WidthVP{SELF, BOTH}.p());
            अगर (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } अन्यथा {
                width_Check_Rep_गिनती(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    ढाँचा <typename T_Rep>
    व्योम width_Check_Rep_गिनती(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        स्थिर AstConst* स्थिर constp = VN_CAST(nodep->countp(), Const);
        अगर (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } अन्यथा अगर (constp->dtypep()->isSigned() && constp->अंक().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } अन्यथा अगर (constp->अंक().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } अन्यथा अगर (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    ढाँचा <typename T_Rep>
    व्योम widthCheckGotoRepRange(T_Rep* nodep, स्थिर अक्षर* kind) {
        user_Iterate_And_अग्र(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        स्थिर AstConst* स्थिर minConstp = VN_CAST(nodep->countp(), Const);
        स्थिर AstConst* स्थिर maxConstp = VN_CAST(nodep->maxCountp(), Const);
        अगर (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            वापस;
        }
        अगर (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            वापस;
        }
        अगर (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            वापस;
        }
        अगर (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    व्योम visit(AstSThroughout* nodep) override {
        m_hasSExpr = true;
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = false;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = true;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = true;
        m_hasSExpr = true;
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            अगर (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = true;
        m_hasSExpr = true;
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            क्रम (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                user_Iterate_And_अग्र(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstURandomRange* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_गाँठ_DType* स्थिर expDTypep = nodep->findUInt32DType();
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    व्योम visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        अगर (VN_IS(nodep->backp(), IsUnbounded)) वापस;  // Ok, leave
        अगर (VN_IS(nodep->backp(), BracketArrayDType)) वापस;  // Ok, leave
        अगर (VN_IS(nodep->backp(), InsideRange)) वापस;  // Ok, leave
        अगर (स्थिर स्वतः* स्थिर varp = VN_CAST(nodep->backp(), Var)) {
            अगर (varp->isParam()) वापस;  // Ok, leave
        }
        Ast_गाँठ* backp = nodep->backp();
        अगर (VN_IS(backp, Sub)) backp = backp->backp();
        अगर (स्थिर स्वतः* स्थिर selp = VN_CAST(backp, SelExtract)) {
            अगर (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) वापस;
        }
        अगर (स्थिर स्वतः* स्थिर selp = VN_CAST(backp, SelBit)) {
            अगर (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) वापस;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    व्योम visit(AstInferredDisable* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) nodep->dtypeSetBit();
    }
    व्योम visit(AstIsUnbounded* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    व्योम visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        अगर (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        अगर (m_vup->final()) {
            Ast_गाँठ_DType* स्थिर expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            अगर (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    व्योम visit(AstCLog2* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    ढाँचा <typename T_Option>
    व्योम widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_गाँठ_व्यंजक* स्थिर valuep = nodep->valuep();
        स्थिर std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(valuep, Const)) {
            अगर (!constp->अंक().isNegative()) वापस;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } अन्यथा अगर (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } अन्यथा {
            वापस;
        }
        // Continue with the default weight
        valuep->replaceWith(नया AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    ढाँचा <typename T_Option>
    व्योम widthCoverInt(T_Option* nodep) {
        user_Iterate_And_अग्र(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    व्योम visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* स्थिर cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        अगर (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            वापस;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        अगर (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            वापस;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        अगर (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            अगर (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << प_पूर्ण);
            } अन्यथा {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } अन्यथा अगर (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_गाँठ_व्यंजक* स्थिर valuep = nodep->valuep();
            अगर (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(नया AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
            }
            वापस;
        } अन्यथा अगर (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            वापस;
        } अन्यथा अगर (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            स्थिर AstConst* स्थिर constp = VN_CAST(nodep->valuep(), Const);
            अगर (!constp || !constp->अंक().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstCoverOption* nodep) override {
        अगर (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            वापस;
        }
        अगर (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            वापस;
        }
        user_Iterate_And_अग्र(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstCoverCross* nodep) override {
        user_Iterate_And_अग्र(nodep->itemsp(), nullptr);
        अगर (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_अग्र(nodep->optionsp(), nullptr);
        user_Iterate_And_अग्र(nodep->binsp(), nullptr);
    }
    व्योम visit(AstCoverCrossBin* nodep) override {
        user_Iterate_And_अग्र(nodep->selectp(), nullptr);
        अगर (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    व्योम visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        user_Iterate_And_अग्र(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        user_Iterate_And_अग्र(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        अगर (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_अग्र(nodep->optionsp(), nullptr);
    }
    व्योम widthCovergroupRanges(Ast_गाँठ* rangesp, पूर्णांक fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        स्थिर स्वतः fill = [&](Ast_गाँठ* nodep) {
            अगर (!fillWidth) वापस;
            Ast_गाँठ_व्यंजक* exprp = VN_AS(nodep, गाँठ_व्यंजक);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        क्रम (Ast_गाँठ *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            अगर (AstInsideRange* स्थिर rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } अन्यथा अगर (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } अन्यथा {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    व्योम visit(AstCoverBinsof* nodep) override {
        user_Iterate_And_अग्र(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    व्योम visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        अगर (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        अगर (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            user_Iterate_And_अग्र(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        user_Iterate_And_अग्र(nodep->transp(), m_vup);
    }
    व्योम visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        user_Iterate_And_अग्र(nodep->itemp(), nullptr);
        अगर (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        अगर (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    व्योम visit(AstCoverCrossWith* nodep) override {
        user_Iterate_And_अग्र(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        user_Iterate_And_अग्र(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        अगर (Ast_गाँठ_व्यंजक* स्थिर countp = nodep->countp()) {
            अगर (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } अन्यथा {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                user_Iterate_And_अग्र(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    व्योम widthCoverFilter(Ast_गाँठ_व्यंजक* filterp, स्थिर अक्षर* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), गाँठ_व्यंजक);
        File_पंक्ति* स्थिर fl = filterp->fileline();
        स्थिर Ast_गाँठ_DType* स्थिर resultp = filterp->dtypep()->skipRefp();
        अगर (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                नया AstNeqD{fl, filterp, नया AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } अन्यथा अगर (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(नया AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(filterp), filterp);
        } अन्यथा {
            fixWidthReduce(filterp);
        }
    }
    व्योम visit(AstCoverTransSet* nodep) override { user_Iterate_And_अग्र(nodep->itemsp(), m_vup); }
    व्योम visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    व्योम visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            अगर (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                वापस;
            }
            checkCvtUS(nodep->lhsp(), false);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            अगर (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->अंक().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        अगर (m_vup->final()) {
            Ast_गाँठ_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाँठ_DType* स्थिर lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_गाँठ* newp = nullptr;  // No change
            अगर (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = नया AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } अन्यथा अगर (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = नया AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } अन्यथा अगर (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = नया AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            अगर (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            }
        }
    }
    व्योम visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(Ast_गिनती_Bits* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            स्थिर पूर्णांक widthMin = V3_संख्या::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    व्योम visit(Ast_गिनती_Ones* nodep) override {
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            स्थिर पूर्णांक widthMin = V3_संख्या::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    व्योम visit(AstCvtPackString* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        अगर (Ast_गाँठ_DType* स्थिर dt = nodep->lhsp()->dtypep()) {
            अगर (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    व्योम visit(AstCvtPackedToArray* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    व्योम visit(AstCvtArrayToArray* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    व्योम visit(AstCvtArrayToPacked* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    व्योम visit(AstCvtUnpackToQueue* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    व्योम visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        अगर (m_vup->final()) {
            user_Iterate_And_अग्र(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    व्योम visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        स्थिर AstConst* स्थिर constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        स्थिर द्विगुन timePrescale = constp->अंक().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        स्थिर द्विगुन समय = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* स्थिर newp = नया AstConst{nodep->fileline(), AstConst::RealDouble{}, समय};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstEventControl* nodep) override {
        अगर (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }
        अगर (nodep->fileline()->timingOn()) {
            अगर (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                वापस;
            } अन्यथा अगर (v3Global.opt.timing().isSetFalse()) {
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
        अगर (nodep->stmtsp()) nodep->add_अग्र_Here(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    अटल string typenameExtends(स्थिर AstClass* classp) {
        // An interface class may extend several, so those are not named
        अगर (!classp || classp->isInterfaceClass()) वापस "";
        क्रम (स्थिर AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            अगर (extendsp->isImplements()) जारी;
            स्थिर AstClass* स्थिर basep = extendsp->classOrNullp();
            वापस basep ? " extends class " + basep->dtypeName(true) : "";
        }
        वापस "";
    }
    व्योम visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        अगर (nodep->dimp()) user_Iterate_And_अग्र(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        चयन (nodep->attrType()) {
        हाल VAttrType::DIM_DIMENSIONS:
        हाल VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_गाँठ_DType* स्थिर dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            स्थिर std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(true);
            स्थिर पूर्णांक val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(नया AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            अवरोध;
        }
        हाल VAttrType::DIM_BITS_OR_संख्या: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            अगर (VN_IS(nodep->fromp(), गाँठ_व्यंजक)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा {
                Ast_गाँठ* newp = नया AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                user_Iterate_And_अग्र(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            वापस;
        }
        हाल VAttrType::DIM_BITS:
        हाल VAttrType::DIM_ऊपरी:
        हाल VAttrType::DIM_INCREMENT:
        हाल VAttrType::DIM_बायाँ:
        हाल VAttrType::DIM_निचला:
        हाल VAttrType::DIM_दायाँ:
        हाल VAttrType::DIM_आकार: {
            Ast_गाँठ_DType* स्थिर dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            अगर (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                स्थिर Ast_गाँठ_Module* स्थिर ownModp = v3Global.rootp()->containingModule(dtypep);
                अगर (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    वापस;
                }
            }

            अगर (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                चयन (nodep->attrType()) {
                हाल VAttrType::DIM_आकार: {
                    Ast_गाँठ_व्यंजक* स्थिर fromp = VN_AS(nodep->fromp()->unlinkFrBack(), गाँठ_व्यंजक);
                    Ast_गाँठ* स्थिर newp
                        = नया AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_आकार};
                    newp->dtypeSetInt();
                    newp->didWidth(true);
                    newp->protect(false);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                    अवरोध;
                }
                हाल VAttrType::DIM_बायाँ:
                हाल VAttrType::DIM_निचला: {
                    Ast_गाँठ* स्थिर newp = नया AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    अवरोध;
                }
                हाल VAttrType::DIM_दायाँ:
                हाल VAttrType::DIM_ऊपरी: {
                    Ast_गाँठ_व्यंजक* स्थिर fromp = VN_AS(nodep->fromp()->unlinkFrBack(), गाँठ_व्यंजक);
                    Ast_गाँठ_व्यंजक* स्थिर sizep
                        = नया AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_आकार};
                    sizep->dtypeSetInt();
                    sizep->didWidth(true);
                    sizep->protect(false);
                    Ast_गाँठ* स्थिर newp
                        = नया AstSub{nodep->fileline(), sizep,
                                     नया AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                    अवरोध;
                }
                हाल VAttrType::DIM_INCREMENT: {
                    Ast_गाँठ_व्यंजक* स्थिर newp
                        = नया AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    अवरोध;
                }
                हाल VAttrType::DIM_BITS: {
                    अगर (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } अन्यथा {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    अवरोध;
                }
                शेष: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } अन्यथा {
                स्थिर std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(true);
                स्थिर uint32_t msbdim = dimpair.first + dimpair.second;
                अगर (!nodep->dimp() || msbdim < 1) {
                    अगर (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_गाँठ_व्यंजक* स्थिर fromp = VN_AS(nodep->fromp()->unlinkFrBack(), गाँठ_व्यंजक);
                        Ast_गाँठ* स्थिर newp = नया AstShiftL{
                            nodep->fileline(), नया AstLenN{nodep->fileline(), fromp},
                            नया AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    } अन्यथा {
                        Ast_गाँठ_DType* स्थिर baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        स्थिर पूर्णांक dim = 1;
                        AstConst* स्थिर newp = dimension_मूल्य(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    }
                } अन्यथा अगर (VN_IS(nodep->dimp(), Const)) {
                    स्थिर पूर्णांक dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* स्थिर newp
                        = dimension_मूल्य(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                } अन्यथा {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* स्थिर tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_गाँठ_व्यंजक* स्थिर dimp = nodep->dimp()->unlinkFrBack();
                    Ast_गाँठ_व्यंजक* स्थिर newp = नया AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                }
            }
            अवरोध;
        }
        हाल VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            अवरोध;
        हाल VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_गाँठ_DType* स्थिर protoDtp = nodep->fromp()->dtypep();
            Ast_गाँठ_DType* स्थिर declDtp = [&]() {
                अगर (m_ftaskp->fvarp()) वापस m_ftaskp->fvarp()->dtypep();
                Ast_गाँठ_DType* स्थिर voidp = नया AstVoidDType{m_ftaskp->fileline()};
                धकेलो_Deletep(voidp);  // Note voidp used past here
                वापस voidp;
            }();
            अगर (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            अवरोध;
        }
        हाल VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_गाँठ_DType* स्थिर dtypep = nodep->fromp()->dtypep();
            string परिणाम = dtypep->prettyDTypeName(true);
            अगर (स्थिर AstClassRefDType* स्थिर classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                परिणाम += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << परिणाम << "' from " << dtypep);
            Ast_गाँठ* स्थिर newp = नया AstConst{nodep->fileline(), AstConst::String{}, परिणाम};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            अवरोध;
        }
        हाल VAttrType::TYPEID:
            अगर (Ast_गाँठ_DType* dtypep = VN_CAST(nodep->fromp(), गाँठ_DType)) {
                nodep->dtypep(dtypep);
            } अन्यथा {
                nodep->dtypep(VN_AS(nodep->fromp(), गाँठ_व्यंजक)->dtypep());
            }
            अवरोध;
        हाल VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            अवरोध;
        शेष: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            अवरोध;
        }
        }
    }
    व्योम visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    व्योम visit(AstCellArrayRef* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        user_Iterate_And_अग्र(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        user_Iterate_And_अग्र(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(true);
    }

    व्योम visit(AstCellRef* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;

        अगर (Ast_गाँठ_व्यंजक* स्थिर cellExprp = VN_CAST(nodep->cellp(), गाँठ_व्यंजक)) {
            user_Iterate_And_अग्र(cellExprp, WidthVP{SELF, PRELIM}.p());
            user_Iterate_And_अग्र(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        अगर (Ast_गाँठ_व्यंजक* स्थिर exprp = VN_CAST(nodep->exprp(), गाँठ_व्यंजक)) {
            user_Iterate_And_अग्र(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            user_Iterate_And_अग्र(exprp, WidthVP{SELF, FINAL}.p());
        } अन्यथा {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(true);
    }

    // DTYPES
    व्योम visit(Ast_गाँठ_Array_DType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed

        अगर (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* स्थिर basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            अगर (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* स्थिर newp = नया AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        अगर (VN_IS(nodep, PackArrayDType)) m_underPackedArray = true;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        user_Iterate_And_अग्र(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        अगर (AstUnpackArrayDType* स्थिर adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            अगर (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(true);
        } अन्यथा {
            स्थिर Ast_गाँठ_DType* स्थिर elemDTypep = nodep->subDTypep()->skipRefp();
            अगर (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            स्थिर पूर्णांक width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            अगर (!VL_RESTORER_पूर्व(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                स्थिर Ast_गाँठ_DType* baseDTypep = nodep->subDTypep();
                जबतक (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                स्थिर AstBasicDType* स्थिर basicp = nodep->basicp();
                अगर (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(true));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    व्योम visit(AstAssocArrayDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->कुंजी_DTypep(iterateEditMoveDTypep(nodep, nodep->कुंजी_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    व्योम visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_गाँठ_DType* स्थिर childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_गाँठ* स्थिर elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_गाँठ* newp;
        अगर (VN_IS(elementsp, Unbounded)) {
            newp = नया AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_वृक्ष(), elementsp);
        } अन्यथा अगर (Ast_गाँठ_DType* स्थिर keyp = VN_CAST(elementsp, गाँठ_DType)) {
            newp = नया AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } अन्यथा {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_पंक्ति* स्थिर elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, true);
            // Must be expression that is constant, but we'll determine that later
            newp = नया AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                नया AstRange{nodep->fileline(), नया AstConst(elementsp->fileline(), 0),
                             नया AstSub{elementsNewFl, VN_AS(elementsp, गाँठ_व्यंजक),
                                        नया AstConst(elementsNewFl, 1)},
                             true}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    व्योम visit(AstConstraintRefDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    व्योम visit(AstDynArrayDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    व्योम visit(AstQueueDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        user_Iterate_And_अग्र(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        अगर (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_वृक्ष();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    व्योम visit(AstVoidDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    व्योम visit(AstUnsizedArrayDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        अगर (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    व्योम visit(AstWildcardArrayDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    व्योम visit(AstBasicDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        अगर (nodep->generic()) वापस;  // Already perfect
        अगर (nodep->rangep()) {
            user_Iterate_And_अग्र(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } अन्यथा अगर (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } अन्यथा अगर (nodep->implicit()) {
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
    व्योम visit(AstConstDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    व्योम visit(AstRefDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(true);
        अगर (nodep->typeofp()) {  // type(typeofp_expression)
            अगर (Ast_गाँठ_DType* typeofDtp = VN_CAST(nodep->typeofp(), गाँठ_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } अन्यथा {
                // Type comes from expression's type, e.g. "type(variable)"
                user_Iterate_And_अग्र(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_गाँठ* स्थिर typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_वृक्ष(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        अगर (nodep->subDTypep()) {
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
        अगर (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            स्थिर bool inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            अगर (inTemplateModule) {
                nodep->doingWidth(false);
                वापस;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(false);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    व्योम widthDTypeNameParams(स्थिर AstTypedef* nodep) {
        स्थिर VDTypeNameScopes* स्थिर scopesp = VDTypeNameScopes::currentp();
        अगर (!scopesp) वापस;  // Computing parameters, as by V3Param, so naming as found
        Ast_गाँठ* scopep = nullptr;
        क्रम (स्थिर Ast_गाँठ* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            अगर (स्थिर std::vector<Ast_गाँठ*>* स्थिर paramsp = scopesp->paramsp(scopep)) {
                क्रम (Ast_गाँठ* स्थिर paramp : *paramsp) {
                    अगर (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    व्योम visit(AstTypedef* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        अगर (स्वतः* स्थिर refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        स्थिर bool declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        अगर (declares) {
            Ast_गाँठ_DType* स्थिर dtypep = nodep->dtypep();
            अगर (Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep = VN_CAST(dtypep, गाँठ_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } अन्यथा अगर (AstEnumDType* स्थिर edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    व्योम visit(AstParamTypeDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        अगर (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    व्योम visit(AstRequireDType* nodep) override {
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        अगर (Ast_गाँठ_DType* स्थिर dtp = VN_CAST(nodep->lhsp(), गाँठ_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } अन्यथा {
            अगर (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(नया AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_गाँठ_DType* स्थिर toDtp = nodep->शीर्ष()->dtypep()->skipRefToEnump();
        Ast_गाँठ_DType* स्थिर fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_पंक्ति* स्थिर fl = nodep->fileline();
        स्थिर स्वतः castable = Ast_गाँठ::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_गाँठ* newp;
        अगर (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            वापस;
        } अन्यथा अगर (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* स्थिर enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_गाँठ_व्यंजक* स्थिर testp = enumTestValid(nodep->fromp(), enumDtp);
            File_पंक्ति* स्थिर fl_novalue = नया File_पंक्ति{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, true);
            newp = नया AstCond{
                fl, testp,
                नया Ast_व्यंजक_Stmt{fl,
                                नया AstAssign{fl_novalue, nodep->शीर्ष()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                नया AstConst{fl, AstConst::Signed32{}, 1}},
                नया AstConst{fl, AstConst::Signed32{}, 0}};
        } अन्यथा अगर ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = नया Ast_व्यंजक_Stmt{
                fl,
                नया AstAssign{fl, nodep->शीर्ष()->unlinkFrBack(),
                              नया AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                नया AstConst{fl, AstConst::Signed32{}, 1}};
        } अन्यथा अगर (castable == VCastable::INCOMPATIBLE) {
            newp = नया AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } अन्यथा {
            newp = नया AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    व्योम visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        अगर (AstConst* स्थिर constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_गाँठ* स्थिर newp
                = नया Ast_Cast_आकार{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } अन्यथा अगर (Ast_गाँठ_DType* स्थिर refp = VN_CAST(nodep->dtp(), गाँठ_DType)) {
            refp->unlinkFrBack();
            Ast_गाँठ* स्थिर newp = नया AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }
    व्योम visit(AstCast* nodep) override {
        अगर (nodep->didWidth()) वापस;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        अगर (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            अगर (AstSigned* स्थिर fromp = VN_CAST(nodep->fromp(), Signed)) {
                अगर (VN_IS(fromp->lhsp(), गाँठ_Stream)) {
                    Ast_गाँठ* स्थिर lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_वृक्ष(), fromp);
                }
            }
            user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_गाँठ_DType* स्थिर toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_गाँठ_DType* स्थिर fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            स्थिर स्वतः castable = Ast_गाँठ::computeCastable(toDtp, fromDtp, nodep->fromp());
            bool बुरा = false;
            अगर (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                बुरा = true;
            } अन्यथा अगर (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } अन्यथा अगर (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                बुरा = true;
            } अन्यथा अगर (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                बुरा = true;
            } अन्यथा {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_गाँठ_व्यंजक* newp = nullptr;
            अगर (बुरा) {
            } अन्यथा अगर (AstBasicDType* स्थिर basicp = toDtp->basicp()) {
                अगर (!basicp->isString() && fromDtp->isString()) {
                    newp = नया AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } अन्यथा अगर (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_गाँठ_DType* स्थिर origDTypep = nodep->dtypep();
                    अगर (!VN_IS(fromDtp, StreamDType)) {
                        स्थिर पूर्णांक width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } अन्यथा {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 false);
                }
                अगर (newp) {
                } अन्यथा अगर (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    अगर (nodep->fromp()->isSigned()) {
                        newp = नया AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } अन्यथा {
                        newp = नया AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } अन्यथा अगर (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = नया AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } अन्यथा अगर (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = नया AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } अन्यथा अगर (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = नया AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } अन्यथा {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } अन्यथा अगर (VN_IS(toDtp, QueueDType)) {
                अगर (VN_IS(fromDtp, BasicDType)) {
                    newp = नया AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } अन्यथा अगर (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    पूर्णांक srcElementBits = 1;
                    पूर्णांक dstElementBits = 1;
                    अगर (Ast_गाँठ_DType* स्थिर elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    स्थिर AstQueueDType* स्थिर dstQueueDtp = VN_AS(toDtp, QueueDType);
                    अगर (Ast_गाँठ_DType* स्थिर elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = नया AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  false,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } अन्यथा अगर (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } अन्यथा {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            अगर (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        अगर (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, false, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_गाँठ_व्यंजक* स्थिर underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(true);
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(true);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }
    व्योम visit(Ast_Cast_आकार* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            पूर्णांक width = nodep->rhsp()->toSInt();
            अगर (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        अगर (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_गाँठ* स्थिर underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    व्योम visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    व्योम castSized(Ast_गाँठ* nodep, Ast_गाँठ* underp, पूर्णांक width) {
        स्थिर AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        अगर (!underDtp) underDtp = underp->dtypep()->basicp();
        अगर (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            स्थिर पूर्णांक calcWidth = std::max(width, underDtp->width());
            Ast_गाँठ_DType* स्थिर calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         false, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_गाँठ_DType* स्थिर outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, गाँठ_व्यंजक), outDtp, EXTEND_EXP,
                            false);
            VL_DANGLING(underp);
        }
    }
    व्योम visit(AstConstraintBefore* nodep) override {
        user_Iterate_And_अग्र(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(Ast_Constraint_व्यंजक* nodep) override {
        user_Iterate_And_अग्र(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstConstraintUnique* nodep) override {
        user_Iterate_And_अग्र(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    व्योम visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        अगर (nodep->didWidth()) वापस;
        अगर (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            धकेलो_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(true);
            वापस;
        }
        nodep->doingWidth(true);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        अगर (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.धकेलो_back(nodep);
        अगर (nodep->attrsp()) {
            nodep->attrsp()->foreach([यह, nodep](AstAttrOf* attrp) {
                अगर (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    अगर (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), गाँठ_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(धकेलो_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        अगर (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_गाँठ_DType* dtp = nodep->dtypep();
            Ast_गाँठ_DType* np = nullptr;
            जबतक (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                अगर (स्थिर AstDynArrayDType* स्थिर unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    अगर (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } अन्यथा {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* स्थिर newp
                        = नया AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    अगर (!np) {  // for Var itself
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
        अगर (AstWildcardArrayDType* स्थिर wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        अगर (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(true);
        // Parameters if implicit untyped inherit from what they are assigned to
        स्थिर AstBasicDType* स्थिर bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        bool didchk = false;
        स्थिर bool implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        अगर (implicitParam) {
            अगर (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                user_Iterate_And_अग्र(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                अगर (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } अन्यथा अगर (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } अन्यथा {
                    पूर्णांक width = 0;
                    Ast_गाँठ_DType* स्थिर मूल्य_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(मूल्य_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    स्थिर AstBasicDType* स्थिर मूल्य_Bdtypep = मूल्य_DTypep->basicp();
                    bool issigned = false;
                    अगर (bdtypep->isNosign()) {
                        अगर (मूल्य_Bdtypep && मूल्य_Bdtypep->isSigned()) issigned = true;
                    } अन्यथा {
                        issigned = bdtypep->isSigned();
                    }
                    अगर (मूल्य_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        अगर (स्थिर स्वतः* स्थिर constp = VN_CAST(nodep->valuep(), Const)) {
                            अगर (constp->अंक().isString()) {
                                width = constp->अंक().toString().लंबाई() * 8;
                            }
                        }
                        अगर (width < 8) width = 8;
                    } अन्यथा अगर (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } अन्यथा {
                        अगर (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    स्थिर bool is_मूल्य_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    अगर (width == 1) {
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
                    didchk = true;
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
        } अन्यथा अगर (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        अगर (nodep->isNet()) {
            Ast_गाँठ_DType* स्थिर badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            अगर (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        अगर (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            user_Iterate_And_अग्र(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        user_Iterate_And_अग्र(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(true);
        nodep->doingWidth(false);
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (nodep->didWidth()) वापस;
        अगर (!nodep->varp()) {
            अगर (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, false,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                वापस;
            } अन्यथा {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        अगर (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        अगर (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            वापस;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        अगर (VN_IS(nodep->backp(), गाँठ_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        अगर (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(true);
        अगर (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } अन्यथा अगर (nodep->access().isWriteOrRW() && nodep->varp()->is_निवेश()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            स्थिर bool hierRef = !m_curModVars.गिनती(nodep->varp());
            स्थिर bool netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            स्थिर bool contAssign = VN_IS(nodep->abovep(), AssignW);
            स्थिर bool pinConn = VN_IS(nodep->abovep(), Pin);
            अगर (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } अन्यथा अगर (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        अगर (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(true);
    }

    व्योम visit(AstEnumDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_गाँठ_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_गाँठ_DType* स्थिर badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        अगर (badDtp) {
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
        user_Iterate_And_अग्र(nodep->itemsp(), WidthVP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_संख्या अंक(nodep, nodep->width(), 0);
        स्थिर V3_संख्या one{nodep, nodep->width(), 1};
        bool wrapAround = false;
        std::map<स्थिर V3_संख्या, AstEnumItem*> inits;
        क्रम (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            अगर (itemp->valuep()) {
                अगर (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_वृक्ष("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                अगर (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_वृक्ष();
                    जारी;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = false;  // Explicit value resets wrap-around tracking
            }
            अगर (!itemp->valuep()) {
                अगर (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                अगर (अंक.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(नया AstConst{itemp->fileline(), अंक});
            }

            स्थिर AstConst* स्थिर constp = VN_AS(itemp->valuep(), Const);
            अगर (constp->अंक().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            अंक.opAssign(constp->अंक());
            // Look for duplicates
            स्थिर स्वतः pair = inits.emplace(अंक, itemp);
            अगर (!pair.second) {  // IEEE says illegal
                स्थिर Ast_गाँठ* स्थिर otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            स्थिर bool पूर्व_Neg = constp->अंक().isNegative();
            अंक.opAdd(one, constp->अंक());
            अगर (basicp->isSigned()) {
                wrapAround = !पूर्व_Neg && अंक.isNegative();
            } अन्यथा {
                wrapAround = अंक.isEqZero();
            }
        }
    }
    व्योम visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_गाँठ_DType* स्थिर vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        अगर (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            user_Iterate_And_अग्र(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            bool warnOn = true;
            Ast_गाँठ_व्यंजक* valuep = nodep->valuep();
            अगर (स्थिर AstAdd* स्थिर anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                अगर (anodep->fileline()->equal_First_पंक्ति_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            अगर (स्थिर AstConst* स्थिर constp = VN_CAST(valuep, Const)) {
                अगर (static_cast<पूर्णांक>(constp->अंक().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = false;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(नया AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    व्योम visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        अगर (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_गाँठ* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            क्रम (; enump; enump = enump->backp()) {
                अगर (VN_IS(enump, EnumDType)) अवरोध;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    व्योम visit(Ast_गाँठ_Cons* nodep) override {}  // Widthed when created
    व्योम visit(Ast_गाँठ_Cons_Item* nodep) override {}  // Widthed when created
    व्योम visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        user_Iterate_And_अग्र(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        क्रम (Ast_गाँठ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_गाँठ_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        अगर (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } अन्यथा अगर (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } अन्यथा {
            // Take width as maximum across all items
            पूर्णांक width = nodep->exprp()->width();
            पूर्णांक mwidth = nodep->exprp()->widthMin();
            bool isFourstate = nodep->exprp()->dtypep()->isFourstate();
            क्रम (स्थिर Ast_गाँठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        क्रम (Ast_गाँठ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            अगर (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } अन्यथा {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        अगर (m_constraintp) {
            bool canLower = true;
            क्रम (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                अगर (VN_IS(ditemp->rangep(), InsideRange)) जारी;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                स्थिर Ast_गाँठ_DType* स्थिर itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                अगर (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = false;
                    अवरोध;
                }
            }
            अगर (canLower) वापस;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_गाँठ_व्यंजक* newp = nullptr;
        क्रम (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_गाँठ_व्यंजक* inewp
                = insideItem(nodep, nodep->exprp()->clone_वृक्ष_Pure(false), itemp->rangep());
            अगर (!inewp) जारी;
            Ast_गाँठ_व्यंजक* स्थिर cmpp
                = नया AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            नया AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, true);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, true);
            inewp = नया AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? नया AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        अगर (!newp) newp = नया AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(AstInside* nodep) override {
        user_Iterate_And_अग्र(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        क्रम (Ast_गाँठ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_गाँठ_DType* expDTypep = nullptr;

        अगर (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } अन्यथा अगर (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } अन्यथा {
            // Take width as maximum across all items
            पूर्णांक width = nodep->exprp()->width();
            पूर्णांक mwidth = nodep->exprp()->widthMin();
            bool isFourstate = nodep->exprp()->dtypep()->isFourstate();
            क्रम (स्थिर Ast_गाँठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            स्थिर VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        क्रम (Ast_गाँठ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            अगर (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } अन्यथा अगर (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_गाँठ_व्यंजक* exprp;
        Ast_व्यंजक_Stmt* व्यंजक_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        अगर (!m_constraintp && !nodep->exprp()->isPure()) {
            File_पंक्ति* स्थिर fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_गाँठ_DType* स्थिर व्यंजक_Dtp = nodep->exprp()->dtypep();
            स्थिर पूर्णांक w = व्यंजक_Dtp->width();
            Ast_गाँठ_DType* स्थिर अस्थायी_DTypep
                = व्यंजक_Dtp->widthSized() ? व्यंजक_Dtp
                                        : nodep->findBitOrLogicDType(w, w, व्यंजक_Dtp->numeric(),
                                                                     व्यंजक_Dtp->isFourstate());
            AstVar* स्थिर varp
                = नया AstVar{fl, VVarType::XTEMP, m_inside_अस्थायी_Names.लो(nodep), अस्थायी_DTypep};
            exprp = नया AstVarRef{fl, varp, VAccess::READ};
            व्यंजक_Stmtp = नया Ast_व्यंजक_Stmt{fl,
                                        नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            अगर (m_ftaskp) {
                varp->funcLocal(true);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } अन्यथा {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(व्यंजक_Stmtp);
        } अन्यथा {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_गाँठ_व्यंजक* newp = nullptr;
        क्रम (Ast_गाँठ_व्यंजक *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), गाँठ_व्यंजक);  // Will be unlinking
            Ast_गाँठ_व्यंजक* inewp;
            अगर (व्यंजक_Stmtp) {
                inewp = insideItem(nodep, व्यंजक_Stmtp, itemp);
                व्यंजक_Stmtp = nullptr;
            } अन्यथा {
                inewp = insideItem(nodep, exprp->clone_वृक्ष_Pure(false), itemp);
            }
            अगर (!inewp) जारी;
            newp = newp ? नया AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        अगर (व्यंजक_Stmtp) VL_DO_DANGLING(व्यंजक_Stmtp->delete_वृक्ष(), व्यंजक_Stmtp);
        अगर (!newp) newp = नया AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    Ast_गाँठ_व्यंजक* insideItem(Ast_गाँठ* nodep, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* itemp) {
        स्थिर Ast_गाँठ_DType* स्थिर itemDtp = itemp->dtypep()->skipRefp();
        अगर (AstInsideRange* स्थिर irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            वापस irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } अन्यथा अगर (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_गाँठ_व्यंजक* स्थिर inewp = नया AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(true);
            वापस inewp;
        } अन्यथा अगर (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_वृक्ष(), exprp);
            वापस nullptr;
        }
        वापस AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    व्योम visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        अगर (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } अन्यथा {
            अगर (m_vup->prelim()) {
                user_Iterate_And_अग्र(nodep->lhsp(), m_vup);
                user_Iterate_And_अग्र(nodep->rhsp(), m_vup);
            }
            अगर (m_vup->final()) {
                Ast_गाँठ_DType* स्थिर expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                स्थिर पूर्णांक expWidth = expDTypep->width();
                स्थिर bool waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                स्थिर bool waiveRhs = expWidth == 32
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

    व्योम visit(AstIfaceRefDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        अगर (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    व्योम visit(Ast_गाँठ_UOr_Struct_DType* nodep) override {
        अगर (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(false);
            वापस;
        }
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(true);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        अगर (स्थिर AstUnionDType* स्थिर unionp = VN_CAST(nodep, UnionDType)) {
            अगर (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(false);
        // Error checks
        क्रम (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_गाँठ_DType* स्थिर dtp = itemp->subDTypep()->skipRefp();
            अगर (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            अगर ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                धकेलो_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        स्थिर bool isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        स्थिर bool inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        अगर (VN_IS(nodep, UnionDType) || nodep->packed()) {
            पूर्णांक lsb = 0;
            पूर्णांक width = 0;
            bool first = true;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            क्रम (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            क्रम (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                अगर (itemp->isFourstate()) nodep->isFourstate(true);
                itemp->lsb(lsb);
                अगर (VN_IS(nodep, UnionDType)) {
                    स्थिर पूर्णांक itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    अगर (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } अन्यथा {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = false;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } अन्यथा {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(false);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    व्योम visit(AstThisRef* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        अगर (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    व्योम visit(AstClassRefDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    व्योम visit(AstClassOrPackageRef* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        userIterateChildren(nodep, nullptr);
    }
    व्योम visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstClassExtends* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        अगर (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    व्योम visit(AstMemberDType* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        अगर (nodep->valuep()) {
            user_Iterate_And_अग्र(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    व्योम visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        अगर (nodep->didWidth()) वापस;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = true;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_गाँठ_DType* स्थिर fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        स्थिर AstMemberSel* स्थिर fromSel = VN_CAST(nodep->fromp(), MemberSel);
        अगर (AstClocking* स्थिर clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            अगर (AstVar* स्थिर varp = memberSelClocking(nodep, clockingp)) {
                अगर (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_वृक्ष(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                अगर (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                अगर (AstIfaceRefDType* स्थिर adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(true);  // Must not visit again: will confuse scopes
                वापस;
            }
        } अन्यथा अगर (Ast_गाँठ_UOr_Struct_DType* स्थिर adtypep = VN_CAST(fromDtp, गाँठ_UOr_Struct_DType)) {
            अगर (memberSelStruct(nodep, adtypep)) वापस;
        } अन्यथा अगर (AstClassRefDType* स्थिर adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            अगर (memberSelClass(nodep, adtypep)) वापस;
        } अन्यथा अगर (AstIfaceRefDType* स्थिर adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            अगर (Ast_गाँठ* foundp = memberSelIface(nodep, adtypep)) {
                अगर (AstClocking* स्थिर clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                अगर (AstVar* स्थिर varp = VN_CAST(foundp, Var)) {
                    अगर (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* स्थिर ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    स्थिर AstVarRef* स्थिर refp = VN_CAST(nodep->fromp(), VarRef);
                    अगर (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* स्थिर newp = नया AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(true);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                        वापस;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(true);
                    वापस;
                }
                अगर (AstModport* स्थिर modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* स्थिर newDtypep = नया AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_गाँठ_व्यंजक* स्थिर fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
                अगर (AstCell* स्थिर cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    अगर (VN_IS(cellp->modp(), Iface)) {
                        स्थिर string viftopName = cellp->name() + "__Viftop";
                        Ast_गाँठ_Module* स्थिर parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        क्रम (Ast_गाँठ* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            अगर (AstVar* स्थिर vp = VN_CAST(itemp, Var)) {
                                अगर (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    अवरोध;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        अगर (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_गाँठ_DType* subDtypep = viftopVarp->dtypep();
                        अगर (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            अगर (AstIfaceRefDType* स्थिर subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* स्थिर newDtypep = नया AstIfaceRefDType{
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
                        वापस;
                    }
                }
                अगर (Ast_गाँठ_FTask* स्थिर ftaskp = VN_CAST(foundp, गाँठ_FTask)) {
                    AstMethodCall* स्थिर newp = नया AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    वापस;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } अन्यथा अगर (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_गाँठ* स्थिर newp = नया AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            वापस;
        } अन्यथा {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    bool memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        अगर (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* स्थिर newp = नया AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            visit(newp);
            वापस true;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* स्थिर first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        क्रम (AstClass* classp = first_classp; classp;) {
            अगर (Ast_गाँठ* स्थिर foundp = m_memberMap.findMember(classp, nodep->name())) {
                अगर (AstVar* स्थिर varp = VN_CAST(foundp, Var)) {
                    अगर (!varp->didWidth()) userIterate(varp, nullptr);
                    अगर (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* स्थिर varRefp
                            = नया AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                        वापस true;
                    }
                    अगर (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(true);
                    अगर (nodep->fromp()->same_वृक्ष(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    वापस true;
                }
                अगर (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(नया AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस true;
                }
                अगर (AstEnumItemRef* स्थिर adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_वृक्ष(false));
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस true;
                }
                अगर (Ast_गाँठ_FTask* ftaskp = VN_CAST(foundp, गाँठ_FTask)) {
                    AstMethodCall* newp = नया AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस true;
                }
                अगर (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    वापस true;
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
        क्रम (AstClass* classp = first_classp; classp;) {
            क्रम (Ast_गाँठ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                अगर (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.धकेलो_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        स्थिर string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        वापस false;  // Caller handles error
    }
    bool modportAllowsItem(Ast_गाँठ* nodep, स्थिर AstIfaceRefDType* adtypep, स्थिर Ast_गाँठ* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        स्थिर AstModport* स्थिर modportp = adtypep->modportp();
        अगर (!modportp) वापस true;
        अगर (स्थिर AstVar* स्थिर varp = VN_CAST(itemp, Var)) {
            अगर (varp->isParam()) वापस true;
        } अन्यथा अगर (!VN_IS(itemp, गाँठ_FTask) && !VN_IS(itemp, Clocking)) {
            वापस true;
        }
        स्वतः pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        अगर (pair.second) {
            क्रम (स्थिर Ast_गाँठ* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.जोड़ो(mpItemp->name());
            }
        }
        अगर (pair.first->second.गिनती(itemp->name())) वापस true;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        वापस false;
    }
    Ast_गाँठ* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_गाँठ_Module* स्थिर ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        क्रम (Ast_गाँठ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            अगर (itemp->name() == nodep->name()) {
                अगर (!modportAllowsItem(nodep, adtypep, itemp)) {
                    वापस nullptr;  // Caller handles error
                }
                वापस itemp;
            }
            अगर (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.धकेलो_Candidate(itemp->prettyName());
            }
        }
        स्थिर string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        वापस nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        क्रम (Ast_गाँठ* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            अगर (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                अगर (citemp->varp()->name() == nodep->name()) वापस citemp->varp();
                speller.धकेलो_Candidate(citemp->varp()->prettyName());
            }
        }
        स्थिर string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        वापस nullptr;  // Caller handles error
    }
    bool memberSelStruct(AstMemberSel* nodep, Ast_गाँठ_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        अगर (AstMemberDType* स्थिर memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            अगर (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } अन्यथा अगर (adtypep->packed()) {
                AstSel* स्थिर newp = नया AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(true);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } अन्यथा {
                AstStructSel* स्थिर newp = नया AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(true);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            वापस true;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        वापस false;
    }

    व्योम visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    व्योम visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        अगर (nodep->didWidth()) वापस;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_गाँठ_DType* स्थिर fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* स्थिर basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        अगर (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } अन्यथा अगर (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } अन्यथा अगर (AstEnumDType* स्थिर adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } अन्यथा अगर (AstAssocArrayDType* स्थिर adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } अन्यथा अगर (AstWildcardArrayDType* स्थिर adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } अन्यथा अगर (AstDynArrayDType* स्थिर adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } अन्यथा अगर (AstQueueDType* स्थिर adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } अन्यथा अगर (AstClassRefDType* स्थिर adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } अन्यथा अगर (AstIfaceRefDType* स्थिर adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } अन्यथा अगर (AstUnpackArrayDType* स्थिर adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } अन्यथा अगर (AstConstraintRefDType* स्थिर adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } अन्यथा अगर (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } अन्यथा अगर (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_गाँठ_FTask_Ref* nodep, bool required, bool arbReturn,
                              Ast_गाँठ_DType* returnDtp, Ast_गाँठ_DType* सूचकांक_Dtp,
                              Ast_गाँठ_DType* मूल्य_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        अगर (AstWith* स्थिर withp = nodep->withp()) {
            withp->सूचकांक_Arg_Refp()->dtypep(सूचकांक_Dtp);
            withp->मूल्य_Arg_Refp()->dtypep(मूल्य_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            वापस withp;
        }
        अगर (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        वापस nullptr;
    }
    व्योम methodOkArguments(Ast_गाँठ_FTask_Ref* nodep, पूर्णांक minArg, पूर्णांक maxArg) {
        पूर्णांक narg = 0;
        अगर (AstWith* स्थिर withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(धकेलो_Deletep(withp->unlinkFrBack()), withp);
        }
        क्रम (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        स्थिर bool ok = (narg >= minArg) && (narg <= maxArg);
        अगर (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            क्रम (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    नया AstArg{nodep->fileline(), "",
                               नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            क्रम (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                जबतक (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_वृक्ष(), argp);
            }
        }
    }

    Ast_गाँठ_व्यंजक* methodArg(AstMethodCall* nodep, पूर्णांक arg) {
        AstArg* argp = nodep->argsp();
        क्रम (पूर्णांक narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        वापस argp->exprp();
    }

    व्योम methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        अगर (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            अगर (nodep->name() == "num") {
                पूर्णांक वस्तुएँ = 0;
                क्रम (Ast_गाँठ* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++वस्तुएँ;
                newp = नया AstConst(nodep->fileline(), AstConst::Signed32{}, वस्तुएँ);
            } अन्यथा अगर (nodep->name() == "first") {
                स्थिर AstEnumItem* itemp = adtypep->itemsp();
                अगर (!itemp) {
                    newp = नया AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } अन्यथा {
                    newp = VN_AS(itemp->valuep()->clone_वृक्ष(false), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } अन्यथा अगर (nodep->name() == "last") {
                स्थिर AstEnumItem* itemp = adtypep->itemsp();
                जबतक (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                अगर (!itemp) {
                    newp = नया AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } अन्यथा {
                    newp = VN_AS(itemp->valuep()->clone_वृक्ष(false), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            अगर (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } अन्यथा अगर (nodep->name() == "next") {
                attrType = VAttrType::ENUM_अग्र;
                methodOkArguments(nodep, 0, 1);
            } अन्यथा अगर (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_पूर्व;
                methodOkArguments(nodep, 0, 1);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }

            अगर (nodep->name() != "name" && nodep->argsp()) {
                Ast_गाँठ_व्यंजक* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, true);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                अगर (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_गाँठ_व्यंजक* स्थिर newp = नया AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(stepp), stepp);
                    stepp = newp;
                }
                स्थिर uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                अगर (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* स्थिर newp = नया AstMethodCall{
                        nodep->fileline(),
                        नया AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                    वापस;
                } अन्यथा अगर (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    धकेलो_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* स्थिर clonep = nodep->clone_वृक्ष(false);
                    VN_AS(stepp, Const)->अंक().setLong(1);
                    AstConst* स्थिर constp = नया AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* स्थिर argp = नया AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* स्थिर newp
                        = नया AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                    वापस;
                }
            }
            Ast_गाँठ_व्यंजक* स्थिर newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    व्योम methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        अगर (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_आकार};  // So don't need num()
            newp->dtypeSetInt();
        } अन्यथा अगर (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } अन्यथा अगर (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_गाँठ_व्यंजक* स्थिर सूचकांक_exprp = method_Call_Wildcard_सूचकांक_व्यंजक(nodep, adtypep);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, सूचकांक_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } अन्यथा अगर (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            अगर (!nodep->argsp()) {
                newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } अन्यथा {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_गाँठ_व्यंजक* स्थिर सूचकांक_exprp = method_Call_Wildcard_सूचकांक_व्यंजक(nodep, adtypep);
                newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, सूचकांक_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } अन्यथा अगर (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } अन्यथा अगर (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* स्थिर withp
                = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* स्थिर withp
                = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "map") {
            AstWith* स्थिर withp
                = methodWithClause(nodep, true, false, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        अगर (newp) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    व्योम methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        अगर (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        अगर (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_आकार};  // So don't need num()
            newp->dtypeSetInt();
        } अन्यथा अगर (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->कुंजी_DTypep(), BOTH);
            Ast_गाँठ_व्यंजक* स्थिर सूचकांक_exprp = method_Call_Assoc_सूचकांक_व्यंजक(nodep, adtypep);
            methodCallLValueRecurse(nodep, सूचकांक_exprp, VAccess::READWRITE);
            newp
                = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     सूचकांक_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->कुंजी_DTypep(), BOTH);
            Ast_गाँठ_व्यंजक* स्थिर सूचकांक_exprp = method_Call_Assoc_सूचकांक_व्यंजक(nodep, adtypep);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, सूचकांक_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } अन्यथा अगर (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            अगर (!nodep->argsp()) {
                newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } अन्यथा {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->कुंजी_DTypep(),
                                  BOTH);
                Ast_गाँठ_व्यंजक* स्थिर सूचकांक_exprp = method_Call_Assoc_सूचकांक_व्यंजक(nodep, adtypep);
                newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, सूचकांक_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } अन्यथा अगर (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } अन्यथा अगर (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* स्थिर withp = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                                    adtypep->कुंजी_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* स्थिर withp = methodWithClause(
                nodep, false, true, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            अगर (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->कुंजी_DTypep()));
            } अन्यथा {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* स्थिर withp = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                                    adtypep->कुंजी_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* स्थिर withp = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                                    adtypep->कुंजी_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->कुंजी_DTypep()));
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "map") {
            AstWith* स्थिर withp = methodWithClause(nodep, true, false, adtypep->subDTypep(),
                                                    adtypep->कुंजी_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        अगर (newp) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    Ast_गाँठ_व्यंजक* method_Call_Assoc_सूचकांक_व्यंजक(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_गाँठ* स्थिर सूचकांक_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", सूचकांक_exprp, CONTEXT_DET, FINAL, adtypep->कुंजी_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(सूचकांक_exprp);  // May have been edited
        वापस nodep->argsp()->exprp();
    }
    Ast_गाँठ_व्यंजक* method_Call_Wildcard_सूचकांक_व्यंजक(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_गाँठ* स्थिर सूचकांक_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", सूचकांक_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(सूचकांक_exprp);  // May have been edited
        वापस nodep->argsp()->exprp();
    }
    व्योम methodCallLValueRecurse(AstMethodCall* nodep, Ast_गाँठ* childp, स्थिर VAccess& access) {
        अगर (AstCMethodHard* स्थिर ichildp = VN_CAST(childp, CMethodHard)) {
            bool methAt = false;
            bool methAtBack = false;
            bool methAtWrite = false;
            चयन (ichildp->method()) {
            हाल VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = true;
                अवरोध;
            हाल VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = true;
                अवरोध;
            हाल VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            हाल VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            हाल VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = true;
                अवरोध;
            शेष: अवरोध;
            }
            अगर (methAt || methAtBack || methAtWrite) {
                स्थिर Ast_गाँठ_DType* स्थिर fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                अगर (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    अगर (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } अन्यथा अगर (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                वापस;
            }
        }
        अगर (Ast_गाँठ_Var_Ref* स्थिर varrefp = VN_CAST(childp, गाँठ_Var_Ref)) {
            varrefp->access(access);
        } अन्यथा अगर (स्थिर AstMemberSel* स्थिर ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } अन्यथा अगर (स्थिर AstStructSel* स्थिर ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } अन्यथा अगर (स्थिर Ast_गाँठ_Sel* स्थिर ichildp = VN_CAST(childp, गाँठ_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } अन्यथा अगर (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } अन्यथा {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_गाँठ_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        अगर (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            अगर (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, false, true, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* स्थिर withp = methodWithClause(
                nodep, false, true, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            अगर (nodep->name() == "unique_index") {
                newp->dtypep(newp->find_Queue_सूचकांक_DType());
            } अन्यथा {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* स्थिर withp
                = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* स्थिर withp
                = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->find_Queue_सूचकांक_DType());
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* स्थिर withp
                = methodWithClause(nodep, true, false, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        वापस newp;
    }
    व्योम methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        अगर (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा अगर (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा अगर (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_आकार};
            newp->dtypeSetInt();
        } अन्यथा अगर (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* स्थिर withp
                = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर ((newp = methodCallArray(nodep, adtypep))) {
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        अगर (newp) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    व्योम methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        अगर (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा अगर (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा अगर (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_आकार};
            newp->dtypeSetInt();
        } अन्यथा अगर (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            अगर (!nodep->argsp()) {
                newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } अन्यथा {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_गाँठ_व्यंजक* स्थिर सूचकांक_exprp = method_Call_Queue_सूचकांक_व्यंजक(nodep);
                अगर (सूचकांक_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_निकालो_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } अन्यथा {
                    newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, सूचकांक_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } अन्यथा अगर (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_गाँठ_व्यंजक* स्थिर सूचकांक_exprp = method_Call_Queue_सूचकांक_व्यंजक(nodep);
            AstArg* स्थिर argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            अगर (सूचकांक_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_धकेलो_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } अन्यथा {
                newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_जोड़ो, सूचकांक_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } अन्यथा अगर (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा अगर (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* स्थिर argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } अन्यथा अगर (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* स्थिर withp
                = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा अगर ((newp = methodCallArray(nodep, adtypep))) {
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        अगर (newp) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    Ast_गाँठ_व्यंजक* method_Call_Queue_सूचकांक_व्यंजक(AstMethodCall* nodep) {
        Ast_गाँठ* स्थिर सूचकांक_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", सूचकांक_exprp, BOTH);
        VL_DANGLING(सूचकांक_exprp);  // May have been edited
        वापस nodep->argsp()->exprp();
    }
    व्योम methodCallWarnTiming(Ast_गाँठ_FTask_Ref* स्थिर nodep, स्थिर std::string& className) {
        अगर (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } अन्यथा अगर (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    व्योम methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* स्थिर ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        अगर (Ast_गाँठ_FTask* स्थिर ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), गाँठ_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            अगर (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                वापस;
            }
            अगर (adtypep->isVirtual()) {
                क्रम (Ast_गाँठ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    अगर (AstVar* स्थिर mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            अगर (ftaskp->isStatic()) {
                AstArg* स्थिर argsp = nodep->argsp();
                अगर (argsp) argsp->unlink_Fr_Back_With_अग्र();
                Ast_गाँठ_FTask_Ref* newp = nullptr;
                अगर (VN_IS(ftaskp, Task)) {
                    newp = नया AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } अन्यथा {
                    newp = नया AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            } अन्यथा {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                अगर (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            वापस;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    व्योम handleRandomizeArgs(Ast_गाँठ_FTask_Ref* स्थिर nodep, AstClass* स्थिर classp) {
        bool hasNonNullArgs = false;
        AstConst* nullp = nullptr;
        क्रम (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_गाँठ_व्यंजक* exprp = argp->exprp();
            अगर (AstConst* स्थिर constp = VN_CAST(exprp, Const)) {
                अगर (constp->अंक().isNull()) {
                    nullp = constp;
                    जारी;
                }
            }
            hasNonNullArgs = true;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            अगर (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_गाँठ_व्यंजक* fromp = methodCallp->fromp();
                जबतक (AstMemberSel* स्थिर memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* स्थिर varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            अगर (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्ष(), argp);
                जारी;
            }
            जबतक (exprp) {
                अगर (AstMemberSel* स्थिर memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } अन्यथा {
                    अगर (AstVarRef* स्थिर varrefp = VN_CAST(exprp, VarRef)) {
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
                अगर (randVarp && !randVarp->isRand()) randVarp->अक्रम(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            अगर (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } अन्यथा अगर (classp->existsMember([&](स्थिर AstClass*, स्थिर AstVar* memberVarp) {
                           वापस memberVarp == randVarp;
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
        अगर (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    व्योम methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* स्थिर first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        अगर (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, false, false, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            क्रम (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                अगर (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } अन्यथा अगर (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        क्रम (AstClass* classp = first_classp; classp;) {
            अगर (nodep->fileline()->timingOn()) {
                अगर (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* स्थिर packagep = getItemPackage(classp);
                    // Check if it's std
                    अगर (packagep && packagep->name() == "std") {
                        अगर (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } अन्यथा अगर (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } अन्यथा अगर (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            अगर (Ast_गाँठ_FTask* स्थिर ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), गाँठ_FTask)) {
                userIterate(ftaskp, nullptr);
                अगर (ftaskp->isStatic()) {
                    AstArg* स्थिर argsp = nodep->argsp();
                    अगर (argsp) argsp->unlink_Fr_Back_With_अग्र();
                    Ast_गाँठ_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    अगर (!m_vup) {  // Called as task
                        newp = नया AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } अन्यथा {
                        newp = नया AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                } अन्यथा {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    अगर (VN_IS(ftaskp, Task)) {
                        अगर (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_व्यंजक)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    अगर (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                वापस;
            } अन्यथा अगर (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCMethodHard* स्थिर newp
                    = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(true);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            } अन्यथा अगर (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_गाँठ_व्यंजक* स्थिर expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCMethodHard* स्थिर newp
                    = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(true);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            } अन्यथा अगर (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(true);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            क्रम (AstClass* classp = first_classp; classp;) {
                क्रम (Ast_गाँठ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    अगर (VN_IS(itemp, गाँठ_FTask)) speller.धकेलो_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            स्थिर string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    व्योम methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        अगर (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            अगर (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } अन्यथा {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(true);
        } अन्यथा {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }
    व्योम methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        अगर (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } अन्यथा {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(true);
    }
    व्योम methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        क्रमागत : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_योग,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        अगर (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } अन्यथा अगर (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } अन्यथा अगर (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } अन्यथा अगर (nodep->name() == "sum") {
            methodId = ARRAY_योग;
        } अन्यथा अगर (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        अगर (methodId) {
            AstWith* स्थिर withp
                = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            अगर (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* स्थिर newp
                    = नया AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                अगर (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(false);
                newp->didWidth(true);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                वापस;
            } अन्यथा {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_पंक्ति* स्थिर fl = nodep->fileline();
                Ast_गाँठ_व्यंजक* newp = nullptr;
                क्रम (पूर्णांक i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_गाँठ_व्यंजक* स्थिर arrayRef = nodep->fromp()->clone_वृक्ष_Pure(false);
                    Ast_गाँठ_व्यंजक* स्थिर selector = नया AstArraySel{fl, arrayRef, i};
                    अगर (!newp) {
                        newp = selector;
                    } अन्यथा {
                        चयन (methodId) {
                        हाल ARRAY_OR: newp = नया AstOr{fl, newp, selector}; अवरोध;
                        हाल ARRAY_AND: newp = नया AstAnd{fl, newp, selector}; अवरोध;
                        हाल ARRAY_XOR: newp = नया AstXor{fl, newp, selector}; अवरोध;
                        हाल ARRAY_योग: newp = नया AstAdd{fl, newp, selector}; अवरोध;
                        हाल ARRAY_PRODUCT: newp = नया AstMul{fl, newp, selector}; अवरोध;
                        शेष: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            }
        } अन्यथा अगर (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    व्योम methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        अगर (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* स्थिर callp = नया AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    व्योम methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        अगर (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_गाँठ* स्थिर newp = नया AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } अन्यथा अगर (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } अन्यथा अगर (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } अन्यथा अगर (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } अन्यथा अगर (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } अन्यथा अगर (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_गाँठ* स्थिर newp
                = नया AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा अगर (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_गाँठ* स्थिर newp
                = नया AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा अगर (nodep->name() == "compare" || nodep->name() == "icompare") {
            स्थिर bool ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* स्थिर argp = nodep->argsp();
            Ast_गाँठ_व्यंजक* स्थिर lhs = nodep->fromp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर rhs = argp->exprp()->unlinkFrBack();
            Ast_गाँठ* स्थिर newp = नया AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा अगर (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* स्थिर arg0p = nodep->argsp();
            AstArg* स्थिर arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_गाँठ_Var_Ref* स्थिर fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_गाँठ_व्यंजक* स्थिर rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* स्थिर varrefp
                = नया AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_गाँठ* स्थिर newp = नया AstAssign{
                nodep->fileline(), fromp, नया AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            धकेलो_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } अन्यथा अगर (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* स्थिर arg0p = nodep->argsp();
            Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->fromp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा अगर (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* स्थिर arg0p = nodep->argsp();
            AstArg* स्थिर arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->fromp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर thsp = arg1p->exprp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा अगर (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            अगर (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } अन्यथा अगर (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } अन्यथा अगर (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } अन्यथा अगर (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } अन्यथा अगर (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_गाँठ* स्थिर newp
                = नया AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_गाँठ_DType* सूचकांक_DTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        अगर (AstQueueDType* स्थिर queuep = m_queueDTypeIndexed[सूचकांक_DTypep]) {
            वापस queuep;
        } अन्यथा {
            AstQueueDType* स्थिर newp
                = नया AstQueueDType{सूचकांक_DTypep->fileline(), सूचकांक_DTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[सूचकांक_DTypep] = newp;
            वापस newp;
        }
    }

    व्योम visit(AstNew* nodep) override {
        अगर (nodep->didWidth()) वापस;
        AstClass* classp = nullptr;
        bool assign = false;
        अगर (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = true;
            Ast_गाँठ* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            अगर (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                अगर (AstClass* स्थिर clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* स्थिर adtypep
                        = नया AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } अन्यथा {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            अगर (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, गाँठ_DType)
                           ? ("data type "s + VN_AS(warnp, गाँठ_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                वापस;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            अगर (Ast_गाँठ_FTask* स्थिर ftaskp
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
        अगर (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    व्योम visit(AstNewCopy* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        AstClassRefDType* स्थिर refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        अगर (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            वापस;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        अगर (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    व्योम visit(AstNewDynamic* nodep) override {
        अगर (nodep->didWidthAndSet()) वापस;
        AstDynArrayDType* स्थिर adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        अगर (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            वापस;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        अगर (!VN_IS(nodep->backp(), गाँठ_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            वापस;
        }
        nodep->dtypep(adtypep);
        अगर (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        अगर (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* स्थिर consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            अगर (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            }
        }
    }

    व्योम visit(Ast_Tagged_व्यंजक* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    व्योम visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    व्योम visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    व्योम visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    व्योम visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    व्योम visit(AstPattern* nodep) override {
        अगर (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            क्रम (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = false;
                user_Iterate_And_अग्र(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                अगर (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), गाँठ_DType)) {
                    user_Iterate_And_अग्र(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                क्रम (Ast_गाँठ *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            वापस;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = false;
        अगर (nodep->didWidthAndSet()) वापस;
        UINFO(9, "PATTERN " << nodep);
        अगर (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        अगर (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_गाँठ_DType* dtypep = nodep->dtypep();
        अगर (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            अगर (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                वापस;
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            अगर (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                वापस;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            क्रम (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                स्थिर पूर्णांक बार = visitPatMemberRep(patp);
                क्रम (पूर्णांक i = 1; i < बार; i++) {
                    AstPatMember* स्थिर newp = patp->clone_वृक्ष(false);
                    patp->add_अग्र_Here(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            क्रम (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                अगर (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    जबतक (Ast_गाँठ_व्यंजक* स्थिर movep = VN_AS(patp->lhssp()->nextp(), गाँठ_व्यंजक)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_गाँठ* newkeyp = nullptr;
                        अगर (patp->keyp()) newkeyp = patp->keyp()->clone_वृक्ष(true);
                        AstPatMember* स्थिर newp
                            = नया AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->add_अग्र(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            क्रम (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* स्थिर nextp = VN_AS(patp->nextp(), PatMember);
                अगर (patp->isDefault()) {
                    अगर (defaultp) {
                        VL_DO_DANGLING(धकेलो_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            जबतक (स्थिर AstConstDType* स्थिर vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            अगर (स्वतः* स्थिर vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } अन्यथा अगर (स्वतः* स्थिर vdtypep = VN_CAST(dtypep, गाँठ_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } अन्यथा अगर (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* स्थिर consp = नया AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } अन्यथा अगर (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* स्थिर consp = नया AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } अन्यथा अगर (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } अन्यथा अगर (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } अन्यथा {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            अगर (defaultp) VL_DO_DANGLING(धकेलो_Deletep(defaultp), defaultp);
        }
    }
    व्योम patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<स्थिर AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            स्थिर AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            जबतक (patp) {
                करो {
                    अगर (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        अगर (स्थिर AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            अगर (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                अवरोध;
                            } अन्यथा {
                                स्थिर std::pair<PatMap::iterator, bool> ret
                                    = patmap.emplace(memp, patp);
                                अगर (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } अन्यथा अगर (Ast_गाँठ_DType* nodedtypep = VN_CAST(patp->keyp(), गाँठ_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            स्थिर string dtype = nodedtypep->dtypep()->prettyDTypeName(true);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } अन्यथा {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } अन्यथा अगर (memp) {
                        // constant expr
                        स्थिर std::pair<PatMap::iterator, bool> ret = patmap.emplace(memp, patp);
                        अगर (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } अन्यथा {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } जबतक (false);

                //अग्र
                अगर (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_गाँठ_व्यंजक* newp = nullptr;
        अगर (vdtypep->packed()) {
            क्रम (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                स्थिर स्वतः it = patmap.find(memp);
                अगर (it == patmap.end()) {  // default or default_type assignment
                    अगर (AstStructDType* स्थिर memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } अन्यथा {
                        AstPatMember* स्थिर patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = मूल्य_Concat_pattern_Struct(patp, newp, memp, nodep);
                        अगर (patp) VL_DO_DANGLING(धकेलो_Deletep(patp), patp);
                    }
                } अन्यथा {  // member assignment
                    newp = मूल्य_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } अन्यथा {  // Unpacked
            AstConsStruct* स्थिर consp = नया AstConsStruct{nodep->fileline(), vdtypep};
            क्रम (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                स्थिर स्वतः it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                अगर (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    धकेलो_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    धकेलो_Deletep(patp);  // patp used below
                } अन्यथा {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_गाँठ_व्यंजक* स्थिर valuep = pattern_Member_मूल्य_Iterate(patp);
                consp->addItemsp(नया AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        अगर (newp) {
            nodep->replaceWith(newp);
        } अन्यथा {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_गाँठ_व्यंजक* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_गाँठ_व्यंजक* newp,
                                                 AstPattern* nodep, स्थिर DTypeMap& dtypemap) {
        क्रम (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            अगर (AstStructDType* स्थिर memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } अन्यथा {
                AstPatMember* स्थिर patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = मूल्य_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                अगर (patp) VL_DO_DANGLING(धकेलो_Deletep(patp), patp);
            }
        }
        वापस newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, स्थिर DTypeMap& dtypemap) {
        स्थिर string memp_DType = memp->virtRefDTypep()->prettyDTypeName(true);
        स्थिर स्वतः it = dtypemap.find(memp_DType);
        अगर (it != dtypemap.end()) {
            // default_value for data_type
            वापस it->second->clone_वृक्ष(false);
        }
        अगर (defaultp) {
            // default_value for any unmatched member yet
            वापस defaultp->clone_वृक्ष(false);
        }
        अगर (memp->valuep()) {
            वापस नया AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_वृक्ष(false), गाँठ_व्यंजक),
                                    नया AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        वापस nullptr;
    }

    Ast_गाँठ_व्यंजक* मूल्य_Concat_pattern_Struct(AstPatMember* patp, Ast_गाँठ_व्यंजक* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        अगर (patp) {
            patp->dtypep(memp);
            Ast_गाँठ_व्यंजक* स्थिर valuep = pattern_Member_मूल्य_Iterate(patp);
            अगर (!newp) {
                newp = valuep;
            } अन्यथा {
                AstConcat* स्थिर concatp = नया AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, true);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, true);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        वापस newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_गाँठ_DType* elemDTypep,
                                       स्थिर DTypeMap& dtypemap = DTypeMap{}) {
        Ast_गाँठ_व्यंजक* स्थिर valuep = defaultp->lhssp()->clone_वृक्ष(false);
        Ast_गाँठ_DType* स्थिर elemDTypeSkipRefp = elemDTypep->skipRefp();
        स्थिर AstStructDType* स्थिर structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        स्थिर bool unpackedStruct = structp && !structp->packed();

        अगर (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
            वापस defaultp->clone_वृक्ष(false);
        }
        अगर (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
            वापस defaultp->clone_वृक्ष(false);
        }
        अगर (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        bool wholeElement = unpackedStruct;
        अगर (Ast_गाँठ_DType* स्थिर मूल्य_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !मूल्य_DTypep->skipRefp()->isIntegralOrPacked()
                                          : मूल्य_DTypep->similarDType(elemDTypeSkipRefp);
        }
        अगर (wholeElement) {
            VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
            वापस defaultp->clone_वृक्ष(false);
        }

        AstPatMember* स्थिर nestedDefaultp
            = नया AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(true);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        क्रम (स्थिर स्वतः& entry : dtypemap) nestedDefaultp->add_अग्र(entry.second->clone_वृक्ष(false));
        AstPattern* स्थिर recursivePatternp = नया AstPattern{defaultp->fileline(), nestedDefaultp};
        वापस नया AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_गाँठ_व्यंजक* patternDefaultValuep(AstPatMember* defaultp, Ast_गाँठ_DType* elemDTypep) {
        अगर (!defaultp) वापस nullptr;
        AstPatMember* स्थिर elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_गाँठ_व्यंजक* स्थिर valuep = pattern_Member_मूल्य_Iterate(elemDefaultp);
        VL_DO_DANGLING(धकेलो_Deletep(elemDefaultp), elemDefaultp);
        वापस valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    व्योम patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        स्थिर VNumRange range = arrayDtp->declRange();
        स्थिर पूर्णांक elements = range.elements();
        AstConsArray* स्थिर newp = नया AstConsArray{nodep->fileline(), arrayDtp};
        पूर्णांक entn = 0;  // Number of elements given so far, from the left
        bool error = false;  // Reported an error, so the value is not complete
        क्रम (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_गाँठ_व्यंजक* स्थिर valuep = pattern_Member_मूल्य_Iterate(patp);
            स्थिर uint64_t सूचकांक_2 = range.ascending() ? entn : elements - 1 - entn;
            स्थिर AstUnpackArrayDType* स्थिर spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            अगर (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_मूल्यावली(सूचकांक_2, valuep);
                जारी;
            }
            स्थिर VCastable castable = Ast_गाँठ::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            अगर (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = true;
            }
            newp->valuep(सूचकांक_2, valuep);
            ++entn;
        }
        अगर (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = true;
        }
        अगर (error) {
            VL_DO_DANGLING(धकेलो_Deletep(newp), newp);
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } अन्यथा {
            nodep->replaceWith(newp);
        }
    }
    व्योम patternArray(AstPattern* nodep, Ast_गाँठ_Array_DType* arrayDtp, AstPatMember* defaultp) {
        अगर (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            वापस;
        }
        स्थिर VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.बाँए() << " to " << range.दाँए());
        Ast_गाँठ* newp = nullptr;
        bool missed = false;  // Some element missed, so not a complete value
        क्रम (पूर्णांक entn = 0, ent = range.बाँए(); entn < range.elements();
             ++entn, ent += range.बायाँ_To_दायाँ_Inc()) {
            Ast_गाँठ_व्यंजक* valuep = nullptr;
            स्थिर स्वतः it = patmap.find(ent);
            अगर (it == patmap.end()) {
                अगर (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } अन्यथा {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = true;
                }
            } अन्यथा {
                AstPatMember* स्थिर patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_मूल्य_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                स्थिर AstUnpackArrayDType* स्थिर memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                अगर (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
                    valuep = नया AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            अगर (valuep) {
                अगर (VN_IS(arrayDtp, UnpackArrayDType)) {
                    अगर (!newp) newp = नया AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } अन्यथा {  // Packed. Convert to concat for now.
                    अगर (!newp) {
                        newp = valuep;
                    } अन्यथा {
                        AstConcat* स्थिर concatp
                            = नया AstConcat{valuep->fileline(), VN_AS(newp, गाँठ_व्यंजक), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        अगर (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        अगर (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(धकेलो_Deletep(newp), newp);
            newp = नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        अगर (newp) {
            nodep->replaceWith(newp);
        } अन्यथा {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        अगर (VN_IS(patp->keyp(), गाँठ_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            वापस nullptr;
        }
        अगर (patp->कुंजी_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->कुंजी_Exprp());  // May change
        AstConst* स्थिर constp = VN_CAST(patp->कुंजी_Exprp(), Const);
        अगर (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        वापस constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_गाँठ_DType* कुंजी_DTypep) {
        अगर (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            वापस nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        अगर (कुंजी_DTypep->skipRefp()->isIntegralOrPacked() && patp->कुंजी_Exprp()) {
            userIterate(patp->कुंजी_Exprp(), WidthVP{कुंजी_DTypep, PRELIM}.p());
            स्थिर पूर्णांक width = std::max(patp->कुंजी_Exprp()->width(), कुंजी_DTypep->width());
            Ast_गाँठ_DType* स्थिर contextDTypep
                = कुंजी_DTypep->findBitOrLogicDType(width, width, कुंजी_DTypep->numeric(), true);
            iterateCheck(patp, "key", patp->कुंजी_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, false);
        }
        AstConst* स्थिर constp = patternConstKeyp(patp);
        अगर (!constp) वापस nullptr;
        अगर (constp->अंक().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            वापस nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, कुंजी_DTypep, EXTEND_EXP, false);
        Ast_गाँठ_व्यंजक* स्थिर convp = patp->कुंजी_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_गाँठ_व्यंजक* स्थिर keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        वापस VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_गाँठ_व्यंजक* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_गाँठ_DType* arrayDtp) {
        Ast_गाँठ_DType* स्थिर कुंजी_DTypep = VN_AS(arrayDtp, AssocArrayDType)->कुंजी_DTypep();
        अगर (!कुंजी_DTypep->skipRefp()->isAggregateType() || !patp->कुंजी_Exprp()) {
            वापस patternScalarKeyp(patp, कुंजी_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->कुंजी_Exprp(), कुंजी_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->कुंजी_Exprp());
        Ast_गाँठ_व्यंजक* स्थिर foldedp = patp->कुंजी_Exprp();
        अगर (!foldedp->is_Constant_वृक्ष()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            वापस nullptr;
        }
        अगर (foldedp->exists([](स्थिर AstConst* constp) { वापस constp->अंक().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            वापस nullptr;
        }
        वापस foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_गाँठ_DType* arrayDtp) {
        वापस patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_गाँठ_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        वापस patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    ढाँचा <typename T_Item, typename T_Cons>
    व्योम sort_Cons_वस्तुएँ(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        जबतक (T_Item* स्थिर itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.धकेलो_back(itemp);
        }
        स्थिर स्वतः कुंजी_Of = [](स्थिर T_Item* itemp) -> स्थिर V3_संख्या& {
            स्थिर Ast_गाँठ* स्थिर keyp = itemp->keyp();
            वापस VN_AS(keyp, Const)->अंक();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](स्थिर T_Item* ap, स्थिर T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            वापस कुंजी_Of(ap).isLtXZ(कुंजी_Of(bp));
        });
        स्थिर T_Item* firstp = nullptr;  // First item with the current key
        क्रम (T_Item* स्थिर itemp : itemps) {
            अगर (firstp && कुंजी_Of(firstp).isCaseEq(कुंजी_Of(itemp))) {
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
    व्योम patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_गाँठ_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        क्रम (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            स्थिर स्वतः keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_गाँठ_व्यंजक* स्थिर valuep = pattern_Member_मूल्य_Iterate(patp);
            अगर (keyp) {
                consp->addItemsp(नया T_Item{patp->fileline(), keyp, valuep});
            } अन्यथा {
                VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        स्थिर AstAssocArrayDType* स्थिर assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        अगर (!assocDtp || !assocDtp->कुंजी_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_वस्तुएँ<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_गाँठ_व्यंजक* newConsDynArrayOrQueue(File_पंक्ति* fileline, स्थिर Ast_गाँठ_DType* dtypep) {
        अगर (VN_IS(dtypep, DynArrayDType)) वापस नया AstConsDynArray{fileline};
        अगर (VN_IS(dtypep, QueueDType)) वापस नया AstConsQueue{fileline};
        वापस nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    ढाँचा <typename T_Item, typename T_Cons>
    व्योम addConsItem(T_Cons* consp, Ast_गाँठ_व्यंजक* valuep) {
        Ast_गाँठ_DType* स्थिर arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        स्थिर bool isElement
            = Ast_गाँठ::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        स्थिर AstQueueDType* स्थिर subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        अगर (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* स्थिर subp = Ast_गाँठ::cast<T_Cons>(valuep);
            अगर (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                अगर (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlink_Fr_Back_With_अग्र());
                VL_DO_DANGLING(धकेलो_Deletep(subp), subp);
                वापस;
            }
            अगर (AstConsQueue* स्थिर queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_वस्तुएँ(consp, queuep);
                वापस;
            }
            अगर (AstConsDynArray* स्थिर dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_वस्तुएँ(consp, dynp);
                वापस;
            }
        }
        T_Item* स्थिर itemp = नया T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        अगर (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } अन्यथा {
            user_Iterate_And_अग्र(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    व्योम addConsItem(Ast_गाँठ_व्यंजक* consp, Ast_गाँठ_व्यंजक* valuep) {
        अगर (AstConsDynArray* स्थिर dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } अन्यथा {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    ढाँचा <typename T_Sub>
    व्योम add_Cons_वस्तुएँ(Ast_गाँठ_व्यंजक* consp, T_Sub* subp) {
        जबतक (स्वतः* स्थिर itemp = subp->itemsp()) {
            Ast_गाँठ_व्यंजक* स्थिर valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(धकेलो_Deletep(subp), subp);
    }
    व्योम patternDynArrayOrQueue(AstPattern* nodep, Ast_गाँठ_DType* arrayp, AstPatMember* defaultp) {
        Ast_गाँठ_व्यंजक* स्थिर newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        स्थिर AstPatMember* स्थिर firstp = VN_AS(nodep->itemsp(), PatMember);
        स्थिर bool keyed = defaultp || (firstp && firstp->keyp());
        अगर (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            वापस;
        }
        स्थिर अक्षर* स्थिर kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        अगर (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        क्रम (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            अगर (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_मूल्य_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    व्योम patternBasic(AstPattern* nodep, Ast_गाँठ_DType* vdtypep, AstPatMember* defaultp) {
        स्थिर AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        स्थिर VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_गाँठ_व्यंजक* newp = nullptr;
        क्रम (स्थिर पूर्णांक ent : range.seqHiToLo()) {
            Ast_गाँठ_व्यंजक* valuep = nullptr;
            स्थिर स्वतः it = patmap.find(ent);
            अगर (it == patmap.end()) {
                अगर (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } अन्यथा {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } अन्यथा {
                AstPatMember* स्थिर patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_मूल्य_Iterate(patp);
            }
            अगर (valuep) {  // Packed. Convert to concat for now.
                अगर (!newp) {
                    newp = valuep;
                } अन्यथा {
                    AstConcat* स्थिर concatp = नया AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        अगर (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        अगर (newp) {
            nodep->replaceWith(newp);
        } अन्यथा {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_गाँठ_व्यंजक* pattern_Member_मूल्य_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_गाँठ_व्यंजक* valuep = patp->lhssp()->unlinkFrBack();
        अगर (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            अगर (AstConst* स्थिर newp = V3WidthCommit::new_If_Const_Commit_आकार(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        वापस valuep;
    }

    अटल व्योम checkEventAssignment(स्थिर Ast_गाँठ_Assign* स्थिर asgnp) {
        string unsupEvtAsgn;
        अगर (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        अगर (VN_IS(asgnp->rhsp(), CReset)) वापस;
        अगर (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        अगर (!unsupEvtAsgn.empty()) {
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

    अटल bool usesDynamicScheduler(Ast_गाँठ* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        जबतक (true) {
            AstVarRef* स्थिर vrefp = VN_CAST(nodep, VarRef);
            अगर (vrefp) वापस usesDynamicScheduler(vrefp);
            अगर (VN_IS(nodep, MemberSel)) {
                वापस true;
            } अन्यथा अगर (Ast_गाँठ_Sel* selp = VN_CAST(nodep, गाँठ_Sel)) {
                nodep = selp->fromp();
            } अन्यथा {
                वापस false;
            }
        }
    }

    अटल bool usesDynamicScheduler(AstVarRef* vrefp) {
        वापस VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    व्योम visit(AstPatMember* nodep) override {
        Ast_गाँठ_DType* स्थिर vdtypep = m_vup->dtypeNullp();
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
        अगर (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            स्थिर AstConst* स्थिर constp = VN_CAST(nodep->repp(), Const);
            अगर (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                बार = 0;
            } अन्यथा {
                बार = constp->toUInt();
            }
            अगर (बार == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                बार = 1;
            }
            nodep->repp()
                ->unlink_Fr_Back_With_अग्र()
                ->delete_वृक्ष();  // Done with replicate before cloning
        }
        वापस बार;
    }

    व्योम visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_गाँठ* propp = nodep->propp();
            जबतक (AstLogNot* स्थिर notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* स्थिर untilp = VN_CAST(propp, Until);
            अगर (untilp && VN_IS(nodep->backp(), गाँठ_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(नया AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(धकेलो_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            user_Iterate_And_अग्र(nodep->sensesp(), nullptr);
            अगर (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    bool firstNewStatementOkRecurse(Ast_गाँठ* nodep) {
        अगर (AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
            अगर (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) वापस true;
        }
        अगर (AstAssign* स्थिर aitemp = VN_CAST(nodep, Assign)) {
            अगर (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) वापस true;
        }
        अगर (AstInitialStaticStmt* स्थिर aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            क्रम (Ast_गाँठ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                अगर (!firstNewStatementOkRecurse(stmtp)) वापस false;
            }
            वापस true;
        }
        अगर (AstInitialAutomaticStmt* स्थिर aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            क्रम (Ast_गाँठ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                अगर (!firstNewStatementOkRecurse(stmtp)) वापस false;
            }
            वापस true;
        }
        वापस false;
    }

    //--------------------
    // Top levels

    ढाँचा <typename CaseItem>
    व्योम handleCaseType(Ast_गाँठ* casep, Ast_गाँठ_व्यंजक* exprp, CaseItem* itemsp) {
        AstAttrOf* स्थिर exprap = VN_CAST(exprp, AttrOf);
        अगर (!exprap) वापस;
        अगर (exprap->attrType() != VAttrType::TYPEID) वापस;

        स्थिर Ast_गाँठ_DType* स्थिर व्यंजक_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << व्यंजक_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_पंक्ति* स्थिर newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, true);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, true);  // Side effect of transform
        casep->fileline(newfl);
        क्रम (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<CaseItem>(itemp->nextp())) {
            अगर (itemp->isDefault()) जारी;
            bool hit = false;
            क्रम (Ast_गाँठ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                स्थिर AstAttrOf* स्थिर condAttrp = VN_CAST(condp, AttrOf);
                अगर (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } अन्यथा {
                    Ast_गाँठ_DType* स्थिर condDtp = condAttrp->dtypep();
                    अगर (Ast_गाँठ::computeCastable(व्यंजक_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = true;
                        अवरोध;
                    }
                }
            }
            धकेलो_Deletep(itemp->condsp()->unlink_Fr_Back_With_अग्र());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(नया AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(नया AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(धकेलो_Deletep(exprap), exprap);
    }

    ढाँचा <typename CaseItem>
    व्योम handleCase(Ast_गाँठ* casep, Ast_गाँठ_व्यंजक* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_गाँठ_DType* subDTypep = exprp->dtypep();
        क्रम (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<CaseItem>(itemp->nextp())) {
            क्रम (Ast_गाँठ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                अगर (condp->dtypep() == subDTypep) जारी;

                अगर (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } अन्यथा अगर (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } अन्यथा {
                    स्थिर पूर्णांक width = std::max(subDTypep->width(), condp->width());
                    स्थिर पूर्णांक mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    स्थिर bool issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        क्रम (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<CaseItem>(itemp->nextp())) {
            क्रम (Ast_गाँठ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    व्योम visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        user_Iterate_And_अग्र(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        क्रम (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            क्रम (Ast_गाँठ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    व्योम visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        user_Iterate_And_अग्र(nodep->incsp(), nullptr);
    }
    व्योम visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    व्योम visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        user_Iterate_And_अग्र(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        क्रम (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            user_Iterate_And_अग्र(itemp->stmtsp(), nullptr);
            क्रम (Ast_गाँठ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    व्योम visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_गाँठ_DType* स्थिर itemDTypep = nodep->findUInt32DType();
        क्रम (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            user_Iterate_And_अग्र(itemp->stmtsp(), nullptr);
            क्रम (Ast_गाँठ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    व्योम visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->stmtsp(), nullptr);
    }
    व्योम visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    व्योम visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    व्योम visit(Ast_गाँठ_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        user_Iterate_And_अग्र(nodep->thensp(), nullptr);
        user_Iterate_And_अग्र(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    व्योम visit(Ast_व्यंजक_Stmt* nodep) override {
        user_Iterate_And_अग्र(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        user_Iterate_And_अग्र(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    व्योम visit(Ast_गाँठ_Foreach* nodep) override {
        अगर (nodep->didWidth()) वापस;
        nodep->didWidth(true);
        स्थिर AstForeachHeader* स्थिर headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        user_Iterate_And_अग्र(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_गाँठ_व्यंजक* स्थिर fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_गाँठ_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        क्रम (Ast_गाँठ *argsp = headerp->elementsp(), *अग्र_argsp; argsp; argsp = अग्र_argsp) {
            अग्र_argsp = argsp->nextp();
            स्थिर bool empty = VN_IS(argsp, Empty);
            AstVar* स्थिर varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            अगर (varp) varp->usedLoopIdx(true);
            अगर (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                वापस;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            अगर (VN_IS(fromDtp, गाँठ_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } अन्यथा अगर (AstBasicDType* स्थिर adtypep = VN_CAST(fromDtp, BasicDType)) {
                अगर (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                    वापस;
                }
            } अन्यथा अगर (स्थिर AstAssocArrayDType* स्थिर adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->कुंजी_DTypep());
            } अन्यथा {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                वापस;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_गाँठ* स्थिर bodyp = nodep->bodyp();
        user_Iterate_And_अग्र(bodyp, nullptr);
        अगर (AstForeach* स्थिर loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            वापस;
        }
    }

    व्योम visit(Ast_गाँठ_Assign* nodep) override {
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
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            अगर (स्थिर AstIfaceRefDType* स्थिर irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                अगर (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                    वापस;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            streamAssignLower(nodep);
            Ast_गाँठ_DType* स्थिर lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            अगर (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                स्थिर Ast_गाँठ_DType* स्थिर lhsDtp = lhsDTypep->skipRefp();
                स्थिर Ast_गाँठ_DType* स्थिर rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                स्थिर bool lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                स्थिर bool rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                अगर (lhsIsUnpackArray && rhsIsUnpackArray) {
                    अगर (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_गाँठ_Var_Ref, Ast_गाँठ_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), गाँठ_Var_Ref), VN_CAST(nodep->rhsp(), गाँठ_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                अगर (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), गाँठ_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        अगर (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        अगर (Ast_गाँठ* स्थिर controlp = nodep->timingControlp()) {
            अगर (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlink_Fr_Back_With_अग्र()->delete_वृक्ष(), controlp);
            } अन्यथा अगर (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } अन्यथा {
                अगर (nodep->fileline()->timingOn()) {
                    अगर (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } अन्यथा {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlink_Fr_Back_With_अग्र()->delete_वृक्ष(), controlp);
            }
        }
        अगर (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            स्थिर Ast_गाँठ_DType* स्थिर lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            अगर (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
            AstMethodCall* स्थिर newp
                = नया AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            वापस;
        }
        अगर (स्थिर AstNewDynamic* स्थिर dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            अगर (!dynp->rhsp()) {
                newp = नया AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } अन्यथा {
                newp = नया AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(true);
            newp->protect(false);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        अगर (AstCMethodHard* स्थिर slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            अगर (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } अन्यथा अगर (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } अन्यथा अगर (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } अन्यथा {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            अगर (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_गाँठ_व्यंजक* स्थिर fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_गाँठ_व्यंजक* स्थिर lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_गाँठ_व्यंजक* स्थिर msbp = slicep->pinsp()->unlinkFrBack();
                Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* स्थिर newp
                    = नया AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(true);
                newp->protect(false);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            }
        }

        अगर (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    व्योम visit(AstRSRule* nodep) override {
        अगर (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        user_Iterate_And_अग्र(nodep->prodlistsp(), nullptr);
        user_Iterate_And_अग्र(nodep->weightStmtsp(), nullptr);
    }

    व्योम visit(AstRelease* nodep) override {
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    व्योम visit(AstDeassign* nodep) override {
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    अटल bool isFormatNonNumericArg(स्थिर Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        वापस dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, गाँठ_UOr_Struct_DType)
                   && !VN_AS(dtypep, गाँठ_UOr_Struct_DType)->packed());
    }

    व्योम checkFormatNumericArg(Ast_गाँठ* argp, अक्षर ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        अगर (std::छोटे(ch) == 'd') वापस;
        // In contrast, %x of a class reference, causes errors on several simulators.
        अगर (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    व्योम visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    व्योम visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    व्योम visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        अगर (m_vup && !m_vup->prelim()) वापस;  // Can be called as statement or function
        अगर (nodep->didWidthAndSet()) वापस;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        अगर (!nodep->व्यंजक_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_गाँठ_व्यंजक* oldExprsp = nodep->exprsp();
        अगर (nodep->व्यंजक_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), गाँठ_व्यंजक);
        }
        अगर (oldExprsp) oldExprsp->unlink_Fr_Back_With_अग्र();
        जबतक (Ast_गाँठ_व्यंजक* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), गाँठ_व्यंजक);
            अगर (oldExprsp) oldExprsp->unlink_Fr_Back_With_अग्र();
            अगर (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                जारी;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_गाँठ_DType* स्थिर dtypep = argp->dtypep()->skipRefp();
            अगर (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } अन्यथा अगर (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } अन्यथा अगर (isFormatNonNumericArg(dtypep)) {
                अगर (AstClassRefDType* स्थिर classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    अगर (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(true);
                    }
                } अन्यथा {
                    // Class handles inside containers print type names, not their members.
                    Ast_गाँठ_DType* गाँठ_Dtypep = dtypep;
                    जबतक (गाँठ_Dtypep) {
                        गाँठ_Dtypep = गाँठ_Dtypep->skipRefp();
                        अगर (Ast_गाँठ_UOr_Struct_DType* स्थिर uOrStructDTypep
                            = VN_CAST(गाँठ_Dtypep, गाँठ_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(true);
                        }
                        गाँठ_Dtypep = गाँठ_Dtypep->subDTypep();
                    }
                }
                Ast_गाँठ_व्यंजक* स्थिर newp = नया AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } अन्यथा अगर (nodep->व्यंजक_Format_2()) {
                अगर (AstEnumDType* स्थिर enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    जारी;
                }
            }
            अगर (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            अगर (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } अन्यथा {
                nodep->addExprsp(नया AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    व्योम visit(AstCReset* nodep) override {
        assert_At_व्यंजक(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    व्योम visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    व्योम visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    व्योम visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        अगर (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        अगर (!m_paramsOnly) {
            nodep->foreach([यह](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    नया AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            अगर (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            चयन (nodep->displayType()) {
            हाल VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); अवरोध;
            हाल VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); अवरोध;
            हाल VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); अवरोध;
            हाल VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); अवरोध;
            शेष: UASSERT_OBJ(false, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        }
    }
    व्योम visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        अगर (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    व्योम visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    व्योम visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    व्योम visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    व्योम visit(AstFError* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            user_Iterate_And_अग्र(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    व्योम visit(AstFEof* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    व्योम visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        अगर (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    व्योम visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    व्योम visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    व्योम visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    व्योम visit(AstFGetC* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    व्योम visit(AstFGetS* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_अग्र(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    व्योम visit(AstFUngetC* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    व्योम visit(AstFRead* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            अगर (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            अगर (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    व्योम visit(AstFScanF* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_अग्र(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    व्योम visit(AstSScanF* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    व्योम visit(Ast_ढेर_Trace_F* nodep) override { nodep->dtypeSetString(); }
    व्योम visit(AstSysIgnore* nodep) override {
        user_Iterate_And_अग्र(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstSystemF* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(Ast_गाँठ_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        user_Iterate_And_अग्र(nodep->memp(), WidthVP{SELF, BOTH}.p());
        स्थिर Ast_गाँठ_DType* subp = nullptr;
        अगर (स्थिर AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            अगर (!adtypep->कुंजी_DTypep()->skipRefp()->basicp()
                || !adtypep->कुंजी_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } अन्यथा अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } अन्यथा अगर (स्थिर AstDynArrayDType* स्थिर adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } अन्यथा {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        अगर (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        user_Iterate_And_अग्र(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstTestPlusArgs* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    व्योम visit(Ast_मूल्य_Plus_Args* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            user_Iterate_And_अग्र(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    व्योम visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        अगर (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        अगर (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        अगर (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        अगर (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    व्योम visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_अग्र(nodep->passsp(), nullptr);
        user_Iterate_And_अग्र(nodep->failsp(), nullptr);
    }
    व्योम visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    व्योम visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_अग्र(nodep->passsp(), nullptr);
        user_Iterate_And_अग्र(nodep->failsp(), nullptr);
    }
    व्योम visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_अग्र(nodep->passsp(), nullptr);
    }
    व्योम visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    bool pinInstArrayElement(AstPin* nodep, स्थिर AstRange* rangep, पूर्णांक idx) {
        Ast_गाँठ_DType* स्थिर modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_गाँठ_DType* स्थिर conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        अगर (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) वापस true;
        // Types don't match, but all dimensions done, still need the usual checks
        अगर (!rangep) वापस false;

        स्थिर AstRange* स्थिर subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        पूर्णांक subElems = 1;
        क्रम (स्थिर AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        स्थिर पूर्णांक elements = rangep->elementsConst();
        स्थिर पूर्णांक from_बायाँ = idx / subElems;

        स्थिर AstUnpackArrayDType* स्थिर arrp = VN_CAST(conDTypep, UnpackArrayDType);
        स्थिर uint32_t conDims = conDTypep->dimensions(false).second;
        स्थिर uint32_t modDims = modDTypep->dimensions(false).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        अगर (arrp && conDims != modDims) {
            अगर (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                user_Iterate_And_अग्र(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                वापस true;
            }
            // ArraySel index is counted from lo
            स्थिर पूर्णांक bit = arrp->declRange().ascending() ? from_बायाँ : elements - 1 - from_बायाँ;
            // Connection is self-determined
            user_Iterate_And_अग्र(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_गाँठ_व्यंजक* स्थिर fromp = VN_AS(nodep->exprp(), गाँठ_व्यंजक)->unlinkFrBack();
            अगर (Ast_गाँठ_व्यंजक* स्थिर elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(धकेलो_Deletep(fromp), fromp);
            } अन्यथा {
                AstArraySel* स्थिर selp = नया AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(true);
                nodep->exprp(selp);
            }
            वापस pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        अगर (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) वापस false;

        // Packed connection
        स्थिर पूर्णांक modwidth = modDTypep->width();
        स्थिर पूर्णांक conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        अगर (conwidth == modwidth) वापस false;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        अगर (conwidth == modwidth * elements * subElems) {
            अगर (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->बायाँ_Const() << ":"
                                                     << rangep->दायाँ_Const() << "]");
            }
            स्थिर पूर्णांक selwidth = modwidth * subElems;
            // Connection is self-determined
            user_Iterate_And_अग्र(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_गाँठ_व्यंजक* स्थिर fromp = VN_AS(nodep->exprp(), गाँठ_व्यंजक)->unlinkFrBack();
            AstSel* स्थिर selp = नया AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_बायाँ), selwidth};
            selp->didWidth(true);
            nodep->exprp(selp);
            वापस pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        user_Iterate_And_अग्र(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        वापस true;
    }

    व्योम visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        अगर (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* स्थिर concatp = VN_CAST(nodep->exprp(), Replicate);
            स्थिर AstConst* स्थिर countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            स्थिर Ast_गाँठ_DType* स्थिर dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            अगर (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* स्थिर patternp = नया AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(धकेलो_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } अन्यथा अगर (!m_paramsOnly) {
            अगर (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            अगर (!nodep->exprp()) {  // No-connect
                अगर (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                वापस;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            स्थिर VDirection pinDirection = nodep->modVarp()->direction();
            अगर (VN_IS(nodep->exprp(), गाँठ_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            user_Iterate_And_अग्र(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_गाँठ_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_गाँठ_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            अगर (स्थिर AstIfaceRefDType* स्थिर modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                स्थिर AstIfaceRefDType* स्थिर conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                अगर (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_गाँठ_DType* subDTypep = modDTypep;
            स्थिर पूर्णांक modwidth = modDTypep->width();
            पूर्णांक conwidth = conDTypep->width();
            अगर (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                user_Iterate_And_अग्र(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } अन्यथा {
                अगर (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    अगर (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        अगर (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        वापस;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                अगर (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } अन्यथा अगर (nodep->modVarp()->isTristate()) {
                    अगर (modwidth != conwidth) {
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
                } अन्यथा अगर (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                अगर ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                स्थिर Ast_गाँठ_Array_DType* स्थिर व्यंजक_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                स्थिर Ast_गाँठ_Array_DType* स्थिर modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                अगर (व्यंजक_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpacked_Array_आकार(conDTypep) != unpacked_Array_आकार(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpacked_Array_आकार(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpacked_Array_आकार(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } अन्यथा अगर ((व्यंजक_Arrayp && !modArrayp) || (!व्यंजक_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (व्यंजक_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } अन्यथा {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_गाँठ_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), गाँठ_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            अगर (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    व्योम visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        अगर (!m_paramsOnly) {
            अगर (VN_IS(nodep->modp(), NotFoundModule)) {
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
            अगर (nodep->rangep()) user_Iterate_And_अग्र(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            अगर (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                धकेलो_Deletep(nodep->rangep()->unlink_Fr_Back_With_अग्र());
            }
        }
        user_Iterate_And_अग्र(nodep->paramsp(), nullptr);
    }
    व्योम visit(AstGatePin* nodep) override {
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            स्थिर Ast_गाँठ_DType* स्थिर pinDTypep = nodep->exprp()->dtypep();
            स्थिर पूर्णांक अंक_Insts = nodep->rangep()->elementsConst();
            स्थिर पूर्णांक modwidth = अंक_Insts;
            स्थिर पूर्णांक pinwidth = pinDTypep->width();
            अगर (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_गाँठ_DType* स्थिर subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                user_Iterate_And_अग्र(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_गाँठ* स्थिर newp
                    = नया AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(अंक_Insts)};
                nodep->replaceWith(newp);
            } अन्यथा {
                अगर (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                user_Iterate_And_अग्र(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            }
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        अगर (nodep->didWidth()) वापस;
        अगर (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(true);
            nodep->didWidth(true);
            वापस;
        }
        अगर (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } अन्यथा अगर (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        अगर (nodep->classMethod() && nodep->name() == "new") {
            अगर (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            अगर (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_गाँठ* firstp = nullptr;
            क्रम (Ast_गाँठ* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                अगर (Ast_Stmt_व्यंजक* स्थिर sep = VN_CAST(itemp, Stmt_व्यंजक)) {
                    अगर (AstNew* स्थिर newp = VN_CAST(sep->exprp(), New)) {
                        अगर (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            अवरोध;
                        }
                    }
                    जारी;
                }
                अगर (firstNewStatementOkRecurse(itemp)) जारी;
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
        अगर (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        अगर (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } अन्यथा अगर (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } अन्यथा अगर (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        अगर (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(true);
        nodep->doingWidth(false);
        अगर (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        अगर (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->exists_And_अग्र([&](स्थिर AstVarRef* varrefp) {
                   वापस varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                अगर (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } अन्यथा अगर (AstAttrOf* स्थिर attrp = VN_CAST(stmtp, AttrOf)) {
                अगर (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                अगर (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        अगर (protop) {
            क्रम (माप_प्रकार i = 0; i < std::max(ports.आकार(), protos.आकार()); ++i) {
                अगर (i >= ports.आकार() || i >= protos.आकार()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    अवरोध;
                } अन्यथा {
                    AstVar* स्थिर portp = ports[i];
                    AstAttrOf* स्थिर rprotop = protos[i];
                    Ast_गाँठ_DType* स्थिर declDtp = portp->dtypep();
                    Ast_गाँठ_DType* स्थिर protoDtp = rprotop->fromp()->dtypep();
                    अगर (portp->name() != rprotop->name()) {
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
                    } अन्यथा अगर (!similarDTypeRecurse(protoDtp, declDtp)) {
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
    व्योम visit(AstConstraint* nodep) override {
        अगर (nodep->didWidth()) वापस;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(true);
    }
    व्योम visit(AstProperty* nodep) override {
        अगर (nodep->didWidth()) वापस;
        अगर (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(true);
            nodep->didWidth(true);
            वापस;
        }
        nodep->doingWidth(true);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        क्रम (Ast_गाँठ* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            अगर (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } अन्यथा अगर (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } अन्यथा अगर (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } अन्यथा {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(true);
        nodep->doingWidth(false);
    }
    व्योम visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    व्योम visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        अगर (nodep->didWidth()) वापस;
        nodep->dtypeSetBit();
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } अन्यथा अगर (VN_IS(stmtp, गाँठ_व्यंजक)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } अन्यथा {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(true);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_गाँठ* pkgItemp) {
        जबतक (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        वापस VN_CAST(pkgItemp->backp(), Package);
    }
    व्योम visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_गाँठ_FTask_Ref*>(nodep));
        अगर (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            अगर (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_व्यंजक))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } अन्यथा {
            nodep->dtypeFrom(nodep->taskp());
        }
        अगर (nodep->fileline()->timingOn()) {
            Ast_गाँठ_Module* स्थिर classp = nodep->classOrPackagep();
            अगर (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* स्थिर packagep = getItemPackage(classp);
                अगर (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    अटल bool are_Same_आकार(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        स्थिर std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        स्थिर std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        अगर (dims0.आकार() != dims1.आकार()) वापस false;
        क्रम (माप_प्रकार i = 0; i < dims0.आकार(); ++i) {
            अगर (dims0[i]->elementsConst() != dims1[i]->elementsConst()) वापस false;
        }
        वापस true;
    }
    // Makes sure that port and pin have same size and same datatype
    व्योम checkUnpackedArrayArgs(AstVar* portp, Ast_गाँठ* pinp) {
        अगर (AstUnpackArrayDType* स्थिर portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            अगर (AstUnpackArrayDType* स्थिर pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                अगर (!are_Same_आकार(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                स्थिर AstBasicDType* स्थिर portBasicp = portDtypep->basicp();
                स्थिर AstBasicDType* स्थिर pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                स्थिर bool sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                अगर (!sameElemDType) {
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
    व्योम processFTaskRefArgs(Ast_गाँठ_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        अगर (nodep->didWidth()) वापस;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        करो {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            स्थिर V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            अगर (m_taskConnectState.didWrap()) m_memberMap.सफा();  // As added a member
            क्रम (स्थिर स्वतः& tconnect : tconnects) {
                स्थिर AstVar* स्थिर portp = tconnect.first;
                AstArg* स्थिर argp = tconnect.second;
                Ast_गाँठ_व्यंजक* pinp = argp->exprp();
                अगर (!pinp) जारी;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                अगर (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlink_Fr_Back_With_अग्र(&handle);  // Format + additional args, if any
                    Ast_गाँठ_व्यंजक* argsp = nullptr;
                    जबतक (AstArg* स्थिर nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->add_अग्र(nextargp->exprp()->unlink_Fr_Back_With_अग्र());
                        nextargp->unlinkFrBack()->delete_वृक्ष();  // Remove the call's Arg wrapper
                    }
                    string format;
                    अगर (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->अंक().toString();
                    } अन्यथा {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(धकेलो_Deletep(argp), argp);
                    AstSFormatF* स्थिर newp
                        = नया AstSFormatF{nodep->fileline(), format, false, argsp};
                    अगर (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(नया AstScopeName{newp->fileline(), true});
                    }
                    handle.relink(नया AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    जाओ reloop;  // so exit early; next loop will correct it
                }  //
                अन्यथा अगर (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_गाँठ_व्यंजक* स्थिर newp = नया AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                अगर (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } जबतक (false);
        // Stage 2
        {
            स्थिर V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            क्रम (स्थिर स्वतः& tconnect : tconnects) {
                AstVar* स्थिर portp = tconnect.first;
                स्थिर AstArg* स्थिर argp = tconnect.second;
                Ast_गाँठ_व्यंजक* स्थिर pinp = argp->exprp();
                अगर (!pinp) जारी;  // Argument error we'll find later
                // Change data types based on above accept completion
                अगर (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                अगर (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            स्थिर V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            क्रम (स्थिर स्वतः& tconnect : tconnects) {
                स्थिर AstVar* स्थिर portp = tconnect.first;
                स्थिर AstArg* स्थिर argp = tconnect.second;
                Ast_गाँठ_व्यंजक* स्थिर pinp = argp->exprp();
                अगर (!pinp) जारी;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        अगर (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            स्थिर V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            क्रम (स्थिर स्वतः& tconnect : tconnects) {
                स्थिर AstVar* स्थिर portp = tconnect.first;
                स्थिर AstArg* स्थिर argp = tconnect.second;
                Ast_गाँठ_व्यंजक* pinp = argp->exprp();
                अगर (!pinp) जारी;  // Argument error we'll find later
                Ast_गाँठ_DType* स्थिर portDTypep = portp->dtypep()->skipRefToEnump();
                अगर (VN_IS(pinp, EmptyQueue)) {
                    Ast_गाँठ_व्यंजक* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    अगर (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = नया AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(pinp), pinp);
                    pinp = newp;
                }
                स्थिर Ast_गाँठ_DType* स्थिर pinDTypep = pinp->dtypep()->skipRefToEnump();
                स्थिर AstIfaceRefDType* स्थिर portIfacep
                    = VN_CAST(portDTypep->elemDTypep(true), IfaceRefDType);
                स्थिर VCastable ifaceCastable
                    = portIfacep ? Ast_गाँठ::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ true)
                                 : VCastable{VCastable::UNSUPPORTED};
                स्थिर bool matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                स्थिर bool pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->अंक().isNull();
                अगर ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    अगर (portIfacep) {
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
                } अन्यथा अगर (portIfacep && portIfacep->isVirtual() && portp->is_निवेश() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } अन्यथा अगर (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_गाँठ_व्यंजक* स्थिर newp = नया AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                अगर (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    जारी;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                अगर (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } अन्यथा अगर (!portp->basicp() || portp->basicp()->isOpaque()) {
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

    व्योम handleStdRandomizeArgs(Ast_गाँठ_FTask_Ref* स्थिर nodep) {
        AstConst* nullp = nullptr;
        क्रम (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_गाँठ_व्यंजक* स्थिर exprp = argp->exprp();
            अगर (AstConst* स्थिर constp = VN_CAST(exprp, Const)) {
                अगर (constp->अंक().isNull()) {
                    nullp = constp;
                    जारी;
                }
            }
            अगर (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                जारी;
            }
            अगर (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                जारी;
            }
            अगर (स्थिर AstCMethodHard* स्थिर methodp = VN_CAST(exprp, CMethodHard)) {
                अगर (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    जारी;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        अगर (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        अगर (nodep->didWidth()) वापस;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        अगर (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(true);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            वापस;  // Handled in V3Randomize
        } अन्यथा अगर (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* स्थिर classp = VN_CAST(nodep->classOrPackagep(), Class);
            अगर (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(true);
                Ast_गाँठ_DType* स्थिर adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, false, false, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                क्रम (स्थिर AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_अग्र(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                अगर (withp) nodep->withp(withp);
                nodep->didWidth(true);
                वापस;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            अगर (nodep->name() == "randomize") {
                AstClassRefDType* स्थिर adtypep
                    = नया AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, false, false, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                क्रम (स्थिर AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_अग्र(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } अन्यथा अगर (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.सफा();
            } अन्यथा अगर (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCExpr* स्थिर newp
                    = नया AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            } अन्यथा अगर (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_गाँठ_व्यंजक* स्थिर expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCExpr* स्थिर newp
                    = नया AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            } अन्यथा {
                UASSERT_OBJ(false, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        अगर (nodep->didWidth()) {
            अगर (withp) nodep->withp(withp);
            वापस;
        }
        अगर ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            bool allow = false;
            अगर (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                अगर (स्थिर AstFuncRef* स्थिर funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } अन्यथा अगर (स्थिर AstTaskRef* स्थिर taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                अगर (!allow) {
                    स्थिर AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    अगर (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    स्थिर AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    अगर (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            अगर (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                स्थिर AstClass* स्थिर cgClassp = m_containingClassFinder.find(nodep);
                अगर (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    स्थिर AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    अगर (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            अगर (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        अगर (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(नया AstScopeName{nodep->fileline(), false});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        अगर (withp) nodep->withp(withp);
        nodep->didWidth(true);
        // See steps that follow in visit(AstFuncRef*)
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    व्योम visit(AstSenItem* nodep) override {
        अगर (nodep->isComboStar()) वापस;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        अगर (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            स्वतः* स्थिर concatOrReplp = VN_CAST(nodep->sensp(), गाँठ_Biop);
            स्वतः* स्थिर rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            अगर (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                स्वतः* स्थिर lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->add_अग्र_Here(नया AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(नया AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            अगर (nodep->edgeType().anEdge()) {
                Ast_गाँठ_DType* स्थिर sensDtp = nodep->sensp()->dtypep()->skipRefp();
                अगर (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } अन्यथा अगर (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    व्योम visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* स्थिर refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    व्योम visit(AstWait* nodep) override {
        अगर (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }
        अगर (nodep->fileline()->timingOn()) {
            अगर (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                अगर (Ast_गाँठ_Var_Ref* स्थिर varrefp = VN_CAST(nodep->condp(), गाँठ_Var_Ref)) {
                    अगर (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                वापस;
            } अन्यथा अगर (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } अन्यथा {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        अगर (Ast_गाँठ* स्थिर stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_Back_With_अग्र();
            nodep->replaceWith(stmtsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_गाँठ_DType* स्थिर vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->सूचकांक_Arg_Refp(), nullptr);
        userIterateChildren(nodep->मूल्य_Arg_Refp(), nullptr);
        अगर (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } अन्यथा {
            अगर (!nodep->exprp()->hasDType()) {
                user_Iterate_And_अग्र(nodep->exprp(), nullptr);
            } अन्यथा {
                अगर (vdtypep) {
                    user_Iterate_And_अग्र(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } अन्यथा {  // 'sort with' allows arbitrary type
                    user_Iterate_And_अग्र(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            अगर (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } अन्यथा {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    व्योम visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        अगर (nodep->सूचकांक_2()) {
            nodep->dtypeFrom(m_withp->सूचकांक_Arg_Refp());
        } अन्यथा {
            nodep->dtypeFrom(m_withp->मूल्य_Arg_Refp());
        }
    }
    व्योम visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    व्योम visit(Ast_गाँठ_व्यंजक* nodep) override {
        अगर (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    व्योम visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << प_पूर्ण);
        अगर (nodep->didWidthAndSet()) वापस;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        अगर (Ast_गाँठ_DType* स्थिर vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } अन्यथा अगर (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->width() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->width() << " bits -> " << nodep->width()
                                    << " bits" << प_पूर्ण);

        // Final processing
        user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, FINAL}.p());
        nodep->didWidth(true);
    }
    व्योम visitClass(AstClass* nodep) {
        अगर (nodep->didWidthAndSet()) वापस;

        // If the class is std::process
        अगर (nodep->name() == "process") {
            AstPackage* स्थिर packagep = getItemPackage(nodep);
            अगर (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                अगर (AstVar* स्थिर varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_गाँठ_DType* स्थिर dtypep = varp->getChildDTypep();
                    अगर (!varp->dtypep()) {
                        VL_DO_DANGLING(धकेलो_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* स्थिर newdtypep = नया AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                अगर (Ast_गाँठ_FTask* स्थिर ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), गाँठ_FTask)) {
                    ftaskp->setNeedProcess();
                    v3Global.setUsesTiming();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        user_Iterate_And_अग्र(nodep->extendsp(), nullptr);
        userIterateChildren(nodep, nullptr);  // First size all members
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enum_अस्थायी_Names);
        VL_RESTORER_COPY(m_inside_अस्थायी_Names);
        अगर (AstClass* स्थिर classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } अन्यथा {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.सफा();
            nodep->foreach([यह](स्थिर AstVar* varp) { m_curModVars.जोड़ो(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    व्योम visit_Or_Lu64(Ast_गाँठ_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_गाँठ_DType* स्थिर subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    व्योम visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            अगर (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    नया AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            }
        }
    }
    व्योम visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    व्योम visit_Os32_Lr(Ast_गाँठ_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    व्योम visit_Ou64_Lr(Ast_गाँठ_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    व्योम visit_log_not(AstLogNot* nodep) {
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
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            अगर (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* स्थिर newp
                    = नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            }
        }
    }
    व्योम visit_log_and_or(Ast_गाँठ_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_गाँठ_व्यंजक::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    व्योम visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit_red_and_or(Ast_गाँठ_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    व्योम visit_red_unknown(Ast_गाँठ_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    bool isEquivalentDType(स्थिर Ast_गाँठ_DType* lhs, स्थिर Ast_गाँठ_DType* rhs) {
        // a) If two types match, they are equivalent.
        अगर (!lhs || !rhs) वापस false;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        अगर (lhs == rhs) वापस true;
        // If both are basic types, check if they are the same type
        अगर (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            स्थिर स्वतः* lb = VN_CAST(lhs, BasicDType);
            स्थिर स्वतः* rb = VN_CAST(rhs, BasicDType);
            अगर (lb->isString() != rb->isString()) वापस false;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        स्थिर bool lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        स्थिर bool rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        अगर (lhsIsUnpackArray || rhsIsUnpackArray) {
            अगर (lhsIsUnpackArray && rhsIsUnpackArray) {
                स्थिर AstUnpackArrayDType* स्थिर lhsp = VN_CAST(lhs, UnpackArrayDType);
                स्थिर AstUnpackArrayDType* स्थिर rhsp = VN_CAST(rhs, UnpackArrayDType);
                स्थिर पूर्णांक lsz = lhsp->elementsConst();
                स्थिर पूर्णांक rsz = rhsp->elementsConst();
                अगर (lsz >= 0 && rsz >= 0 && lsz != rsz) वापस false;
                वापस isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            वापस false;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        स्थिर bool lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        स्थिर bool rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        स्थिर bool lhsIsQueue = VN_IS(lhs, QueueDType);
        स्थिर bool rhsIsQueue = VN_IS(rhs, QueueDType);
        स्थिर bool lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        स्थिर bool rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        अगर (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            अगर (स्थिर AstDynArrayDType* स्थिर lhsp = VN_CAST(lhs, DynArrayDType)) {
                अगर (स्थिर AstDynArrayDType* स्थिर rhsp = VN_CAST(rhs, DynArrayDType)) {
                    वापस isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            अगर (स्थिर AstQueueDType* स्थिर lhsp = VN_CAST(lhs, QueueDType)) {
                अगर (स्थिर AstQueueDType* स्थिर rhsp = VN_CAST(rhs, QueueDType)) {
                    वापस isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            अगर (स्थिर AstAssocArrayDType* स्थिर lhsp = VN_CAST(lhs, AssocArrayDType)) {
                अगर (स्थिर AstAssocArrayDType* स्थिर rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    वापस isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->कुंजी_DTypep(), rhsp->कुंजी_DTypep());
                }
            }

            वापस false;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        अगर (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            अगर (lhs->width() != rhs->width()) वापस false;
            अगर (lhs->isFourstate() != rhs->isFourstate()) वापस false;
            अगर (lhs->isSigned() != rhs->isSigned()) वापस false;
            वापस true;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        अगर (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) वापस false;
        वापस !(VN_IS(lhs, गाँठ_UOr_Struct_DType) || VN_IS(rhs, गाँठ_UOr_Struct_DType));
    }

    अटल bool isAggregateType(स्थिर Ast_गाँठ* nodep) {
        अगर (!nodep) वापस false;
        स्थिर Ast_गाँठ_DType* dtypep = nodep->dtypep();
        अगर (!dtypep) वापस false;
        dtypep = dtypep->skipRefp();
        अगर (!dtypep) वापस false;
        वापस dtypep->isAggregateType();
    }

    व्योम visit_cmp_eq_gt(Ast_गाँठ_Biop* nodep, स्थिर ComparisonType cmpType) {
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
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            स्थिर bool lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            स्थिर bool rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            अगर (lhsIsPat && !rhsIsPat) {
                user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } अन्यथा अगर (rhsIsPat && !lhsIsPat) {
                user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } अन्यथा {
                user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            स्थिर bool isAggrLhs = isAggregateType(nodep->lhsp());
            स्थिर bool isAggrRhs = isAggregateType(nodep->rhsp());

            अगर ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                स्थिर Ast_गाँठ_DType* स्थिर lhsDType = nodep->lhsp()->dtypep();
                स्थिर Ast_गाँठ_DType* स्थिर rhsDType = nodep->rhsp()->dtypep();

                अगर (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_गाँठ* स्थिर newp
                        = नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } अन्यथा अगर (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                अगर (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_गाँठ* स्थिर newp
                        = नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
                अगर (Ast_गाँठ_Biop* स्थिर newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } अन्यथा अगर (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                अगर (Ast_गाँठ_Biop* स्थिर newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } अन्यथा {
                स्थिर bool signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                अगर (Ast_गाँठ_Biop* स्थिर newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                स्थिर पूर्णांक width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                स्थिर पूर्णांक ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                bool warnOn = true;
                अगर (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    अगर ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = false;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    अगर ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = false;
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
    व्योम visit_cmp_real(Ast_गाँठ_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit_cmp_string(Ast_गाँठ_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    व्योम visit_cmp_type(Ast_गाँठ_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            स्थिर AstAttrOf* स्थिर lhsap = VN_AS(nodep->lhsp(), AttrOf);
            स्थिर AstAttrOf* स्थिर rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            स्थिर Ast_गाँठ_DType* स्थिर lhsDtp = lhsap->dtypep();
            स्थिर Ast_गाँठ_DType* स्थिर rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            स्थिर bool invert = VN_IS(nodep, NeqT);
            स्थिर bool identical
                = Ast_गाँठ::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            स्थिर bool eq = invert ^ identical;
            Ast_गाँठ* स्थिर newp = नया AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }

    व्योम visit_negate_not(Ast_गाँठ_Uniop* nodep, bool real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            अगर (!real_ok) checkCvtUS(nodep->lhsp(), false);
        }
        अगर (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            अगर (Ast_गाँठ_Uniop* स्थिर newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                वापस;
            }
        } अन्यथा {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        अगर (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        अगर (m_vup->final()) {
            Ast_गाँठ_DType* स्थिर expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_गाँठ_DType* स्थिर subDTypep = expDTypep;
            // Some warning suppressions
            bool lhsWarn = true;
            अगर (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                अगर (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = false;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    व्योम visit_signed_unsigned(Ast_गाँठ_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), true);
            स्थिर पूर्णांक width = nodep->lhsp()->width();
            Ast_गाँठ_DType* स्थिर expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_गाँठ_DType* स्थिर subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    व्योम visit_shift(Ast_गाँठ_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        स्थिर Ast_गाँठ_Biop* स्थिर newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (व्योम)newp;  // Ununused
    }
    व्योम iterate_shift_prelim(Ast_गाँठ_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), false);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            अगर (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_गाँठ_Biop* iterate_shift_final(Ast_गाँठ_Biop* nodep) {
        // Nodep maybe edited
        assert_At_व्यंजक(nodep);
        अगर (m_vup->final()) {
            Ast_गाँठ_DType* स्थिर expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाँठ_DType* स्थिर subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            अगर (VN_IS(nodep, ShiftRS)) {
                अगर (Ast_गाँठ_Biop* स्थिर newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            bool warnOn = true;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            अगर (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), गाँठ_Assign)) warnOn = false;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* स्थिर shiftp = VN_CAST(nodep->rhsp(), Const);
            अगर (shiftp && !shiftp->अंक().isFourState() && shiftp->width() <= 32) {
                स्थिर int64_t shiftVal = shiftp->अंक().toSQuad();
                अगर (VN_IS(nodep, ShiftL)) {
                    अगर (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = false;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            अगर (nodep->rhsp()->width() > 32) {
                अगर (shiftp && shiftp->अंक().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_संख्या अंक{shiftp, 32, shiftp->अंक()};
                    Ast_गाँठ* स्थिर shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(नया AstConst{shiftrhsp->fileline(), अंक});
                    VL_DO_DANGLING(shiftrhsp->delete_वृक्ष(), shiftrhsp);
                }
            }
        }
        वापस nodep;  // May edit
    }

    व्योम visit_boolexpr_and_or(Ast_गाँठ_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), false);
            checkCvtUS(nodep->rhsp(), false);
            स्थिर पूर्णांक width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            स्थिर पूर्णांक mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            स्थिर bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_गाँठ_व्यंजक::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        अगर (m_vup->final()) {
            Ast_गाँठ_DType* स्थिर expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाँठ_DType* स्थिर subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    व्योम visit_add_sub_replace(Ast_गाँठ_Biop* nodep, bool real_ok, स्थिर bool modOrDiv) {
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
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_अग्र(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            अगर (!real_ok) {
                checkCvtUS(nodep->lhsp(), false);
                checkCvtUS(nodep->rhsp(), false);
            }
            अगर (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                अगर (Ast_गाँठ_Biop* स्थिर newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                वापस;
            } अन्यथा अगर (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            } अन्यथा {
                स्थिर पूर्णांक width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                स्थिर पूर्णांक mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                स्थिर bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_गाँठ_व्यंजक::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        अगर (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_गाँठ_DType* स्थिर expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाँठ_DType* स्थिर subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            अगर (Ast_गाँठ_Biop* स्थिर newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            bool lhsWarn = true;
            bool rhsWarn = true;
            अगर (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                अगर (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = false;
                अगर (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = false;
                अगर (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = false;  // do_increment + ...
                अगर (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = false;  // ... + do_increment
            } अन्यथा अगर (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                अगर (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = false;
                अगर (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = false;
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
    व्योम visit_real_add_sub(Ast_गाँठ_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    व्योम visit_real_neg_ceil(Ast_गाँठ_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_व्यंजक(nodep);
        अगर (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    व्योम visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->व्यंजक_Format_2(), nodep, "Assumes constant format");
        bool inPct = false;
        Ast_गाँठ_व्यंजक* argp = nodep->exprsp();
        string newFormat;
        क्रम (अक्षर ch : nodep->text()) {
            अगर (!inPct && ch == '%') {
                inPct = true;
                newFormat += ch;
            } अन्यथा अगर (inPct && (std::है_अंक(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } अन्यथा अगर (!inPct) {  // Normal text
                newFormat += ch;
            } अन्यथा {
                inPct = false;
                Ast_गाँठ_व्यंजक* स्थिर nextp = argp ? VN_AS(argp->nextp(), गाँठ_व्यंजक) : nullptr;
                AstSFormatArg* स्थिर fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_गाँठ_व्यंजक* subargp = fargp ? fargp->exprp() : argp;
                स्थिर Ast_गाँठ_DType* स्थिर dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::छोटे(ch);
                चयन (ch) {
                हाल '?':  // Unspecified by user, guess
                    अगर (dtypep->isDouble()) {
                        ch = 'g';
                    } अन्यथा अगर (dtypep->isString()) {
                        ch = 's';
                    } अन्यथा अगर (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } अन्यथा {
                        ch = 'p';
                    }
                    argp = nextp;
                    अवरोध;
                हाल '%': अवरोध;  // %% - just output a %
                हाल 'm': अवरोध;  // %m - auto insert "name"
                हाल 'l': अवरोध;  // %m - auto insert "library"
                हाल 'd':
                    अगर (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        अगर (subargp->isDouble()) spliceCvtS(subargp, true, 64);
                        argp = nextp;
                    }
                    अवरोध;
                हाल 'b':  // FALLTHRU
                हाल 'o':  // FALLTHRU
                हाल 'x':
                    अगर (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    अवरोध;
                हाल 't':  // Decimal/float time
                    अगर (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    अवरोध;
                हाल 'e':  // FALLTHRU
                हाल 'f':  // FALLTHRU
                हाल 'g':
                    अगर (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        अगर (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    अवरोध;
                हाल 'p':  // FALLTHRU
                हाल 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    अगर (subargp && !fargp) {
                        अगर (AstEnumDType* स्थिर enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    अवरोध;
                शेष:  // Most operators, just move to next argument
                    argp = nextp;
                    अवरोध;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    अटल AstEnumDType* formatEnumDType(Ast_गाँठ_व्यंजक* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        अगर (!enumDtp) {
            अगर (स्थिर AstVarRef* स्थिर varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        वापस enumDtp;
    }

    Ast_व्यंजक_Stmt* newEnumCapture(Ast_गाँठ_व्यंजक* valuep, AstEnumDType* dtypep) {
        File_पंक्ति* स्थिर flp = valuep->fileline();
        AstVar* स्थिर varp = नया AstVar{flp, VVarType::XTEMP, m_enum_अस्थायी_Names.लो(valuep), dtypep};
        varp->noSubst(true);
        अगर (m_ftaskp) {
            varp->funcLocal(true);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } अन्यथा {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_व्यंजक_Stmt* स्थिर exprp = नया Ast_व्यंजक_Stmt{
            flp, नया AstAssign{flp, नया AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            नया AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        वापस exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_गाँठ_व्यंजक* valuep, AstEnumDType* dtypep) {
        अगर (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_गाँठ_व्यंजक* keyp;
        अगर (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_वृक्ष(false);
        } अन्यथा {
            Ast_व्यंजक_Stmt* स्थिर capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_वृक्ष_Pure(false);
        }
        स्थिर VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* स्थिर argp = नया AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        वापस argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    bool widthBad(Ast_गाँठ* nodep, Ast_गाँठ_DType* expDTypep) {
        स्थिर पूर्णांक expWidth = expDTypep->width();
        पूर्णांक expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        अगर (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) वापस false;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        अगर (expWidthMin == 0) expWidthMin = expWidth;
        अगर (nodep->dtypep()->width() == expWidth) वापस false;
        अगर (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) वापस true;
        अगर (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) वापस true;
        वापस false;
    }

    व्योम fixWidthExtend(Ast_गाँठ_व्यंजक* nodep, Ast_गाँठ_DType* expDTypep, ExtendRule extendRule) {
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
        अगर (extendRule == EXTEND_OFF) वापस;
        AstConst* स्थिर constp = VN_CAST(nodep, Const);
        स्थिर पूर्णांक expWidth = expDTypep->width();
        अगर (constp && !constp->अंक().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_संख्या अंक{nodep, expWidth, constp->अंक()};
            अंक.isSigned(false);
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstConst{nodep->fileline(), अंक};
            constp->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } अन्यथा अगर (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(true);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } अन्यथा {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            bool doSigned = false;
            चयन (extendRule) {
            हाल EXTEND_ZERO: doSigned = false; अवरोध;
            हाल EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); अवरोध;
            हाल EXTEND_LHS: doSigned = nodep->isSigned(); अवरोध;
            शेष: nodep->v3fatalSrc("bad case");
            }
            Ast_गाँठ_व्यंजक* स्थिर newp
                = (doSigned ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_गाँठ_व्यंजक*>(नया AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(true);
            linker.relink(newp);
            nodep = newp;
        }
        अगर (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_गाँठ_व्यंजक* स्थिर newp = spliceCvtD(nodep);
            nodep = newp;
        }
        अगर (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } अन्यथा {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    व्योम fixWidthReduce(Ast_गाँठ_व्यंजक* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        स्थिर पूर्णांक expWidth = 1;
        स्थिर पूर्णांक expSigned = false;
        अगर (nodep->width() == expWidth) वापस;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* स्थिर constp = VN_CAST(nodep, Const);
        अगर (constp) {
            V3_संख्या अंक(nodep, expWidth);
            अंक.opRedOr(constp->अंक());
            अंक.isSigned(expSigned);
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstConst{nodep->fileline(), अंक};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } अन्यथा {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_गाँठ_व्यंजक* newp;
            Ast_गाँठ_DType* स्थिर dtypeSkipp = nodep->dtypep()->skipRefp();
            अगर (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = नया AstNeq{nodep->fileline(),
                                  नया AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } अन्यथा {
                // But, AstRedOr can be faster on wide
                newp = नया AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    bool fixAutoExtend(Ast_गाँठ_व्यंजक*& nodepr, पूर्णांक expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        अगर (AstConst* स्थिर constp = VN_CAST(nodepr, Const)) {
            अगर (constp->अंक().autoExtend() && !constp->अंक().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_संख्या अंक(constp, expWidth);
                अंक.opRepl(constp->अंक(), expWidth);  // {width{'1}}
                Ast_गाँठ_व्यंजक* स्थिर newp = नया AstConst{constp->fileline(), अंक};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                वापस true;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            अन्यथा अगर (!constp->अंक().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->अंक().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_संख्या अंक(constp, expWidth);
                अंक.opExtendXZ(constp->अंक(), constp->width());
                Ast_गाँठ_व्यंजक* स्थिर newp = नया AstConst{constp->fileline(), अंक};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                वापस true;
            }
        }
        वापस false;  // No change
    }
    bool isBaseClassRecurse(स्थिर AstClass* स्थिर cls1p, स्थिर AstClass* स्थिर cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        अगर (cls1p == cls2p) वापस true;
        क्रम (स्थिर AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            अगर (isBaseClassRecurse(cls1p, cextp->classp())) वापस true;
        }
        वापस false;
    }
    // Lines to end a message on nodep with the data types of its operands
    अटल string warnDTypes(स्थिर Ast_गाँठ* nodep, स्थिर Ast_गाँठ_DType* lhsDtp,
                             स्थिर Ast_गाँठ_DType* rhsDtp, स्थिर string& lhsName = "Left-hand",
                             स्थिर string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        अगर (lhsState == rhsState) lhsState = rhsState = "";
        वापस "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    अटल bool unpackedArraySizesMatch(स्थिर Ast_गाँठ_DType* lhsDtp, स्थिर Ast_गाँठ_DType* rhsDtp) {
        जबतक (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            स्थिर AstUnpackArrayDType* स्थिर lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            स्थिर AstUnpackArrayDType* स्थिर rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            अगर (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                वापस false;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        वापस true;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    ढाँचा <typename T, typename N>
    व्योम checkUnpackedArrayAssignmentCompatible(स्थिर Ast_गाँठ* nodep, स्थिर T* स्थिर lhsRefp,
                                                स्थिर N* स्थिर rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::मूल्य || std::is_same<T, Ast_गाँठ_Var_Ref>::मूल्य)
                && (std::is_same<N, AstVar>::मूल्य || std::is_same<N, Ast_गाँठ_Var_Ref>::मूल्य),
            "Unsupported types provided.");
        अगर (!lhsRefp || !rhsRefp) वापस;
        std::string lhsName;
        std::string rhsName;
        अगर (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } अन्यथा {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        स्थिर Ast_गाँठ_DType* स्थिर lhsDtp = lhsRefp->dtypep()->skipRefp();
        स्थिर Ast_गाँठ_DType* स्थिर rhsDtp = rhsRefp->dtypep()->skipRefp();
        स्थिर bool isLhsAggregate = lhsDtp->isAggregateType();
        स्थिर bool isRhsAggregate = rhsDtp->isAggregateType();
        अगर (!isLhsAggregate && !isRhsAggregate) वापस;
        अगर (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            वापस;
        } अन्यथा अगर (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            वापस;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(false),
                                      rhsDim = rhsDtp->dimensions(false);
        // Check if unpacked array dimensions are matching
        अगर (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            वापस;
        }

        स्थिर Ast_गाँठ_DType* lhsDtpIterp = lhsDtp;
        स्थिर Ast_गाँठ_DType* rhsDtpIterp = rhsDtp;
        क्रम (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            अगर (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                वापस;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        अगर (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    व्योम checkClassAssign(स्थिर Ast_गाँठ* nodep, स्थिर अक्षर* side, Ast_गाँठ* rhsp,
                          Ast_गाँठ_DType* स्थिर lhsDTypep, bool isOutputArg = false) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        स्थिर Ast_गाँठ_DType* स्थिर lhsRawDTypep = lhsDTypep->skipRefp();
        स्थिर Ast_गाँठ_DType* स्थिर rhsRawDTypep = rhsp->dtypep()->skipRefp();
        अगर (स्थिर AstClassRefDType* स्थिर lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            अगर (स्थिर AstClassRefDType* स्थिर rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                स्थिर AstClass* स्थिर dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                स्थिर AstClass* स्थिर srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                अगर (isBaseClassRecurse(dstp, srcp)) वापस;
            } अन्यथा अगर (rhsp->isNull()) {
                वापस;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } अन्यथा अगर (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        अगर (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                नया AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, गाँठ_व्यंजक), lhsDTypep});
        }
    }
    अटल bool similarDTypeRecurse(स्थिर Ast_गाँठ_DType* स्थिर node1p,
                                    स्थिर Ast_गाँठ_DType* स्थिर node2p) {
        वापस node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    व्योम iterateCheckFileDesc(Ast_गाँठ* parentp, Ast_गाँठ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_गाँठ_DType* स्थिर expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (व्योम)underp;  // cppcheck
    }
    व्योम iterateCheckReal(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    व्योम iterateCheckSigned8(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    व्योम iterateCheckSigned32(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    व्योम iterateCheckUInt32(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    व्योम iterateCheckDelay(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        अगर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        अगर (stage & FINAL) {
            Ast_गाँठ_DType* expDTypep;
            अगर (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } अन्यथा {
                File_पंक्ति* स्थिर newFl = नया File_पंक्ति{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, true);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, false);
        }
        (व्योम)underp;  // cppcheck
    }
    व्योम iterateCheckString(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    व्योम iterateCheckTyped(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp,
                           Ast_गाँठ_DType* expDTypep, Stage stage) {
        अगर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        अगर (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (व्योम)underp;  // cppcheck
    }
    व्योम iterateCheckTypedSelfPrelim(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp,
                                     Ast_गाँठ_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        अगर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        अगर (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (व्योम)underp;  // cppcheck
    }
    व्योम iterateCheckIntegralSelf(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, true);
    }
    व्योम iterateCheckSelf(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Determ determ,
                          Stage stage, bool integralOnly = false) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        अगर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, गाँठ_व्यंजक) ? checkCvtUS(VN_AS(underp, गाँठ_व्यंजक), integralOnly) : underp;
        Ast_गाँठ_DType* स्थिर expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (व्योम)underp;  // cppcheck
    }
    व्योम iterateCheckSizedSelf(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Determ determ,
                               Stage stage, bool deferUntyped = false) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        अगर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, गाँठ_व्यंजक) ? checkCvtUS(VN_AS(underp, गाँठ_व्यंजक), false) : underp;
        Ast_गाँठ_DType* स्थिर expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_गाँठ_DType* स्थिर checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        अगर (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (व्योम)underp;  // cppcheck
    }
    व्योम iterateCheckAssign(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* rhsp, Stage stage,
                            Ast_गाँठ_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        अगर (AstConst* constp = VN_CAST(rhsp, Const)) {
            अगर (स्थिर AstUnpackArrayDType* स्थिर arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                अगर (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    अगर (basicp->width() == 8 && constp->अंक().isFromString()) {
                        AstConsArray* newp = नया AstConsArray{
                            constp->fileline(), lhsDTypep,
                            नया AstConst{constp->fileline(), AstConst::Widthed_मूल्य{}, 8, 0}};
                        क्रम (स्थिर पूर्णांक aindex : arrayp->declRange().seqLoToHi()) {
                            पूर्णांक cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_संख्या selected{constp, 8};
                            selected.opSel(constp->अंक(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            अगर (!selected.isFourState()) {
                                अगर (स्थिर uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 नया AstConst{constp->fileline(),
                                                              AstConst::Widthed_मूल्य{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(धकेलो_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        स्थिर bool lhsStream = (VN_IS(parentp, गाँठ_Assign)
                                && VN_IS(VN_AS(parentp, गाँठ_Assign)->lhsp(), गाँठ_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, true, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (व्योम)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_गाँठ_व्यंजक* convertUnpackedOrder(Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_DType* dtypep) {
        स्थिर AstUnpackArrayDType* स्थिर arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        अगर (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) वापस exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_गाँठ_व्यंजक* स्थिर newp = नया AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        वापस newp;
    }

    अटल bool isLogicalOperator(स्थिर Ast_गाँठ* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        वापस VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    व्योम iterateCheckBool(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Stage stage) {
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
        Ast_गाँठ_DType* स्थिर underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        अगर (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_गाँठ* स्थिर newp
                = नया AstNeqD{parentp->fileline(), VN_AS(underp, गाँठ_व्यंजक),
                              नया AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } अन्यथा अगर (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_गाँठ* स्थिर newp
                = नया AstNeqN{parentp->fileline(), VN_AS(underp, गाँठ_व्यंजक),
                              नया AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } अन्यथा अगर (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, गाँठ_व्यंजक)), underp);  // Changed
        } अन्यथा अगर (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(नया AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(धकेलो_Deletep(underp), underp);
        } अन्यथा {
            स्थिर bool बुरा = widthBad(underp, parentp->findBitDType());
            अगर (बुरा) {
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
            अगर (Ast_व्यंजक_Stmt* स्थिर व्यंजक_Stmtp = VN_CAST(underp, व्यंजक_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(व्यंजक_Stmtp->resultp());
                व्यंजक_Stmtp->dtypeFrom(व्यंजक_Stmtp->resultp());
            } अन्यथा अगर (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, गाँठ_व्यंजक)), underp);  // Changed
            }
        }
    }

    अटल व्योम checkEnum(Ast_गाँठ* nodep, Ast_गाँठ* parentp, स्थिर Ast_गाँठ_DType* targetDTypep,
                          स्थिर Ast_गाँठ_DType* sourceDTypep, स्थिर Ast_गाँठ_व्यंजक* sourceExprp) {
        अगर (स्थिर AstEnumDType* स्थिर targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            स्थिर VCastable castable
                = Ast_गाँठ::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            अगर (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    Ast_गाँठ* iterateCheck(Ast_गाँठ* parentp, स्थिर अक्षर* side, Ast_गाँठ* underp, Determ determ,
                          Stage stage, Ast_गाँठ_DType* expDTypep, ExtendRule extendRule,
                          bool warnOn = true, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        स्थिर StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        अगर (rhsAccess.isReadOnly()) {
            अगर (Ast_गाँठ_व्यंजक* स्थिर exprp = VN_CAST(underp, गाँठ_व्यंजक)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        अगर (VN_IS(underp, गाँठ_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } अन्यथा अगर (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } अन्यथा अगर (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } अन्यथा अगर (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_गाँठ* स्थिर oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, गाँठ_व्यंजक));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } अन्यथा अगर (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_गाँठ* स्थिर oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, गाँठ_व्यंजक), true, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } अन्यथा अगर (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_गाँठ* स्थिर oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, गाँठ_व्यंजक));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } अन्यथा {
            स्थिर Ast_गाँठ_DType* स्थिर underDtp = underp->dtypep()->skipRefp();
            अगर (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_गाँठ* स्थिर newp
                    = नया AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(underp), underp);
                underp = newp;
            }
            स्थिर Ast_गाँठ_DType* स्थिर expDtp = expDTypep->skipRefp();
            अगर (determ == ASSIGN && VN_IS(expDtp, गाँठ_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            अगर (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            स्थिर AstBasicDType* स्थिर expBasicp = expDTypep->basicp();
            स्थिर AstBasicDType* स्थिर underBasicp = underp->dtypep()->basicp();
            अगर (expBasicp && underBasicp) {
                अगर (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    अगर (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, गाँठ_व्यंजक));
                    }
                    अगर (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_गाँठ_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                अगर (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } अन्यथा अगर (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    अगर (underp->isSigned() != subDTypep->isSigned()
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
                widthCheckSized(parentp, side, VN_AS(underp, गाँठ_व्यंजक), expDTypep, extendRule,
                                warnOn);
            } अन्यथा अगर (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } अन्यथा अगर (स्थिर AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                स्थिर AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                अगर (VN_IS(underp, Const) && VN_AS(underp, Const)->अंक().isNull()) {
                    // '= null' is ok
                } अन्यथा अगर (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } अन्यथा अगर (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } अन्यथा अगर (underIfaceRefp->modportp()
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
        वापस underp;
    }

    व्योम
    widthCheckSized(Ast_गाँठ* parentp, स्थिर अक्षर* side,
                    Ast_गाँठ_व्यंजक* underp,  // Node to be checked or have typecast added in front of
                    Ast_गाँठ_DType* expDTypep, ExtendRule extendRule, bool warnOn = true) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        स्थिर AstBasicDType* स्थिर expBasicp = expDTypep->basicp();
        स्थिर AstBasicDType* स्थिर underBasicp = underp->dtypep()->basicp();
        अगर (expDTypep == underp->dtypep()) {
            वापस;  // Same type must match
        } अन्यथा अगर (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#अगर VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#पूर्ण_अगर
            वापस;
        } अन्यथा {
            स्थिर पूर्णांक expWidth = expDTypep->width();
            पूर्णांक expWidthMin = expDTypep->widthMin();
            अगर (expWidthMin == 0) expWidthMin = expWidth;
            स्थिर bool बुरा = widthBad(underp, expDTypep);
            अगर ((बुरा || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                वापस;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            अगर (VN_IS(parentp->backp(), Cast_आकार)) warnOn = false;
            अगर (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = false;
            अगर (VN_IS(underp, Const) && VN_AS(underp, Const)->अंक().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = false;
            }
            अगर ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::म_भेद(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = false;
            }
            अगर (बुरा && warnOn) {
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
            अगर (बुरा || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                स्थिर Ast_गाँठ_Assign* assignp = VN_CAST(parentp, गाँठ_Assign);
                स्थिर AstPin* pinp = VN_CAST(parentp, Pin);
                अगर (assignp && VN_IS(assignp->lhsp(), गाँठ_Stream)) {
                } अन्यथा अगर (pinp && pinp->modVarp()->direction() != VDirection::निवेश) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } अन्यथा अगर (assignp && VN_IS(underp, गाँठ_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    स्थिर पूर्णांक shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* स्थिर widenedp = नया AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(true);
                    // Shift left so zeros fill on the right
                    Ast_गाँठ_व्यंजक* स्थिर shiftedp = नया AstShiftL{
                        underp->fileline(), widenedp,
                        नया AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
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

    Ast_गाँठ_व्यंजक* checkCvtUS(Ast_गाँठ_व्यंजक* nodep, bool fatal) {
        अगर (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            अगर (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } अन्यथा {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, false, 32);
        }
        वापस nodep;
    }

    Ast_गाँठ_व्यंजक* spliceCvtD(Ast_गाँठ_व्यंजक* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        अगर (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_गाँठ_व्यंजक* newp;
            अगर (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = नया AstISToRD{nodep->fileline(), nodep};
            } अन्यथा {
                newp = नया AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            वापस newp;
        } अन्यथा {
            वापस nodep;
        }
    }
    Ast_गाँठ_व्यंजक* spliceCvtS(Ast_गाँठ_व्यंजक* nodep, bool warnOn, पूर्णांक width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        अगर (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                अगर (constp->isDouble() && V3_संख्या::epsilonIntegral(constp->अंक().toDouble())) {
                    warnOn = false;
                }
            }
            अगर (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            वापस newp;
        } अन्यथा {
            वापस nodep;
        }
    }
    Ast_गाँठ_व्यंजक* spliceCvtString(Ast_गाँठ_व्यंजक* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        अगर (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            वापस newp;
        } अन्यथा {
            वापस nodep;
        }
    }
    Ast_गाँठ_Biop* replaceWithUOrSVersion(Ast_गाँठ_Biop* nodep, bool signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        अगर (signedFlavorNeeded == nodep->signedFlavor()) वापस nullptr;
        अगर (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        चयन (nodep->type()) {
        हाल VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); वापस nullptr;
        हाल VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); वापस nullptr;
        हाल VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); वापस nullptr;
        हाल VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); वापस nullptr;
        हाल VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); वापस nullptr;
        हाल VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); वापस nullptr;
        हाल VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); वापस nullptr;
        हाल VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); वापस nullptr;
        हाल VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); वापस nullptr;
        शेष: अवरोध;
        }
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ_Biop* newp = nullptr;
        चयन (nodep->type()) {
        हाल VNType::Gt: newp = नया AstGtS{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::GtS: newp = नया AstGt{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Gte: newp = नया AstGteS{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::GteS: newp = नया AstGte{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Lt: newp = नया AstLtS{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::LtS: newp = नया AstLt{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Lte: newp = नया AstLteS{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::LteS: newp = नया AstLte{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Div: newp = नया AstDivS{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::DivS: newp = नया AstDiv{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::ModDiv: newp = नया AstModDivS{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::ModDivS: newp = नया AstModDiv{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Mul: newp = नया AstMulS{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::MulS: newp = नया AstMul{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::ShiftR: newp = नया AstShiftRS{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::ShiftRS: newp = नया AstShiftR{fl, lhsp, rhsp}; अवरोध;
        शेष:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            अवरोध;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        वापस newp;
    }
    Ast_गाँठ_Biop* replaceWithDVersion(Ast_गाँठ_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        अगर (nodep->doubleFlavor()) वापस nullptr;
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        चयन (nodep->type()) {
        हाल VNType::Add: newp = नया AstAddD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Sub: newp = नया AstSubD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Pow: newp = नया AstPowD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Eq:
        हाल VNType::EqCase: newp = नया AstEqD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Neq:
        हाल VNType::NeqCase: newp = नया AstNeqD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Gt:
        हाल VNType::GtS: newp = नया AstGtD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Gte:
        हाल VNType::GteS: newp = नया AstGteD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Lt:
        हाल VNType::LtS: newp = नया AstLtD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Lte:
        हाल VNType::LteS: newp = नया AstLteD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Div:
        हाल VNType::DivS: newp = नया AstDivD{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Mul:
        हाल VNType::MulS: newp = नया AstMulD{fl, lhsp, rhsp}; अवरोध;
        शेष:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            अवरोध;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        वापस newp;
    }
    Ast_गाँठ_Biop* replaceWithNVersion(Ast_गाँठ_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        अगर (nodep->stringFlavor()) वापस nullptr;
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        चयन (nodep->type()) {
        हाल VNType::Eq:
        हाल VNType::EqCase:
        हाल VNType::EqWild: newp = नया AstEqN{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Neq:
        हाल VNType::NeqCase:
        हाल VNType::NeqWild: newp = नया AstNeqN{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Gt:
        हाल VNType::GtS: newp = नया AstGtN{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Gte:
        हाल VNType::GteS: newp = नया AstGteN{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Lt:
        हाल VNType::LtS: newp = नया AstLtN{fl, lhsp, rhsp}; अवरोध;
        हाल VNType::Lte:
        हाल VNType::LteS: newp = नया AstLteN{fl, lhsp, rhsp}; अवरोध;
        शेष:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            अवरोध;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        वापस newp;
    }
    Ast_गाँठ_Uniop* replaceWithDVersion(Ast_गाँठ_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        अगर (nodep->doubleFlavor()) वापस nullptr;
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाँठ_Uniop* newp = nullptr;
        चयन (nodep->type()) {
        हाल VNType::Negate: newp = नया AstNegateD{fl, lhsp}; अवरोध;
        शेष:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            अवरोध;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        वापस newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    व्योम replaceWithSFormat(AstMethodCall* nodep, स्थिर string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* स्थिर argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_गाँठ_Var_Ref* स्थिर fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_गाँठ* स्थिर newp = नया AstAssign{
            nodep->fileline(), fromp,
            नया AstSFormatF{nodep->fileline(), format, false, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        धकेलो_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_गाँठ_DType* iterateEditMoveDTypep(Ast_गाँठ* parentp, Ast_गाँठ_DType* dtnodep) {
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
        स्थिर bool child1 = (parentp->getChildDTypep() == dtnodep);
        स्थिर bool child2 = (parentp->getChild2DTypep() == dtnodep);
        अगर (child1 || child2) {
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
        अगर (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        वापस dtnodep;
    }

    AstConst* dimension_मूल्य(File_पंक्ति* fileline, Ast_गाँठ_DType* nodep, VAttrType attrType,
                             पूर्णांक dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_गाँठ_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        क्रम (पूर्णांक i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            अगर (स्थिर Ast_गाँठ_Array_DType* स्थिर adtypep = VN_CAST(dtypep, गाँठ_Array_DType)) {
                declRange = adtypep->declRange();
                अगर (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                जारी;
            } अन्यथा अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर adtypep
                       = VN_CAST(dtypep, गाँठ_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                अवरोध;  // Sub elements don't look like arrays and can't iterate into
            } अन्यथा अगर (स्थिर AstBasicDType* स्थिर adtypep = VN_CAST(dtypep, BasicDType)) {
                अगर (adtypep->isRanged()) declRange = adtypep->declRange();
                अवरोध;
            }
            अवरोध;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        पूर्णांक val = 0;
        चयन (attrType) {
        हाल VAttrType::DIM_BITS: {
            पूर्णांक bits = 1;
            जबतक (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                अगर (स्थिर Ast_गाँठ_Array_DType* स्थिर adtypep = VN_CAST(dtypep, गाँठ_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    जारी;
                } अन्यथा अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर adtypep
                           = VN_CAST(dtypep, गाँठ_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    अवरोध;
                } अन्यथा अगर (स्थिर AstBasicDType* स्थिर adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    अवरोध;
                }
                अवरोध;
            }
            अगर (dim == 0) {
                val = 0;
            } अन्यथा अगर (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } अन्यथा {
                val = bits;
            }
            अवरोध;  // LCOV_EXCL_LINE
        }
        हाल VAttrType::DIM_ऊपरी: val = !declRange.ranged() ? 0 : declRange.hi(); अवरोध;
        हाल VAttrType::DIM_बायाँ: val = !declRange.ranged() ? 0 : declRange.बाँए(); अवरोध;
        हाल VAttrType::DIM_निचला: val = !declRange.ranged() ? 0 : declRange.lo(); अवरोध;
        हाल VAttrType::DIM_दायाँ: val = !declRange.ranged() ? 0 : declRange.दाँए(); अवरोध;
        हाल VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            अवरोध;
        हाल VAttrType::DIM_आकार: val = !declRange.ranged() ? 0 : declRange.elements(); अवरोध;
        शेष: nodep->v3fatalSrc("Missing DIM ATTR type case"); अवरोध;
        }
        अगर (!valp) valp = नया AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        वापस valp;
    }
    AstVarRef* dimensionVarRefp(Ast_गाँठ_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        अगर (cachedpr) वापस cachedpr->clone_वृक्ष(false);

        Ast_गाँठ_Array_DType* स्थिर vardtypep = नया AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), नया AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* स्थिर consp = नया AstConsArray{nodep->fileline(), vardtypep};
        क्रम (अचिन्हित i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_मूल्य(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_वृक्ष(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(true);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(true);
        धकेलो_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        वापस cachedpr->clone_वृक्ष(false);
    }
    अटल uint64_t enum_Max_मूल्य(स्थिर Ast_गाँठ* errNodep, स्थिर AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        क्रम (स्थिर AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            स्थिर AstConst* स्थिर vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            अगर (!vconstp->अंक().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        अगर (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            वापस 64;
        }
        वापस maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* स्थिर nodep, VAttrType attrType, bool assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        अगर (cachedpr) वापस cachedpr->clone_वृक्ष(false);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ_DType* basep;
        अगर (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } अन्यथा अगर (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } अन्यथा {
            basep = nodep->dtypep();
        }
        Ast_गाँठ_DType* vardtypep;
        अगर (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = नया AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } अन्यथा {
            vardtypep = नया AstUnpackArrayDType{flp, basep, नया AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        अगर (attrType == VAttrType::ENUM_NAME) {
            defaultp = नया AstConst{flp, AstConst::String{}, ""};
        } अन्यथा अगर (attrType == VAttrType::ENUM_अग्र || attrType == VAttrType::ENUM_पूर्व) {
            defaultp = नया AstConst{flp, V3_संख्या{nodep, nodep->width(), 0}};
        } अन्यथा अगर (attrType == VAttrType::ENUM_VALID) {
            defaultp = नया AstConst{flp, AstConst::BitFalse{}};
        } अन्यथा {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_गाँठ_व्यंजक*> मूल्यावली;
        {
            uint64_t ordinal = 0;
            AstEnumItem* स्थिर firstp = nodep->itemsp();
            स्थिर AstEnumItem* prevp = firstp;  // Prev must start with last item
            जबतक (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            क्रम (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* स्थिर nextp = VN_AS(itemp->nextp(), EnumItem);
                स्थिर AstConst* स्थिर vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                अगर (!vconstp->अंक().isAnyXZ()) {  // Can 2-state runtime decode
                    स्थिर uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    अगर (attrType == VAttrType::ENUM_NAME) {
                        मूल्यावली[i] = नया AstConst{flp, AstConst::String{},
                                                 V3_संख्या::displayedEnumName(itemp)};
                    } अन्यथा अगर (attrType == VAttrType::ENUM_अग्र) {
                        मूल्यावली[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_वृक्ष(false);  // A const
                    } अन्यथा अगर (attrType == VAttrType::ENUM_पूर्व) {
                        मूल्यावली[i] = prevp->valuep()->clone_वृक्ष(false);  // A const
                    } अन्यथा अगर (attrType == VAttrType::ENUM_VALID) {
                        मूल्यावली[i] = नया AstConst{flp, AstConst::BitTrue{}};
                    } अन्यथा {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_गाँठ_व्यंजक* valuep = nullptr;
        अगर (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* स्थिर consp = नया AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            क्रम (स्थिर स्वतः& itr : मूल्यावली) {
                V3_संख्या कुंजी{nodep, nodep->width()};
                कुंजी.setQuad(itr.first);
                consp->addItemsp(नया AstConsAssocItem{flp, नया AstConst{flp, कुंजी}, itr.second});
            }
            valuep = consp;
        } अन्यथा {
            AstConsArray* स्थिर consp = नया AstConsArray{flp, vardtypep, defaultp};
            क्रम (uint64_t i = 0; i < (msbdim + 1); ++i) {
                अगर (मूल्यावली[i]) consp->valuep(i, मूल्यावली[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(true);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(true);
        धकेलो_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        वापस cachedpr->clone_वृक्ष(false);
    }

    Ast_गाँठ_व्यंजक* enumSelect(Ast_गाँठ_व्यंजक* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        अगर (attrType == VAttrType::ENUM_NAME) {
            अगर (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep, Const)) {
                Ast_गाँठ_व्यंजक* स्थिर newp = नया AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->अंक().displayedEnumName(adtypep)};
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस newp;
            }
        }
        अगर (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_व्यंजक_Stmt* स्थिर capturep = newEnumCapture(nodep, adtypep);
            Ast_गाँठ_व्यंजक* स्थिर valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            वापस capturep;
        }
        अगर (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_गाँठ_व्यंजक* indexp = नया AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            क्रम (स्थिर AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                स्थिर AstConst* स्थिर constp = VN_AS(itemp->valuep(), Const);
                अगर (constp->अंक().isAnyXZ()) जारी;
                indexp = नया AstCond{nodep->fileline(),
                                     नया AstEq{nodep->fileline(), nodep->clone_वृक्ष(false),
                                               itemp->valuep()->clone_वृक्ष(false)},
                                     नया AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* स्थिर tabRefp = enumVarRefp(adtypep, attrType, false, ordinal);
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस newp;
        }
        स्थिर uint64_t msbdim = enum_Max_मूल्य(nodep, adtypep);
        स्थिर bool assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_गाँठ_व्यंजक* newp;
        अगर (assoc) {
            AstVarRef* स्थिर tabRefp = enumVarRefp(adtypep, attrType, true, 0);
            newp = नया AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } अन्यथा {
            स्थिर पूर्णांक selwidth = V3_संख्या::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* स्थिर tabRefp
                = enumVarRefp(adtypep, attrType, false, (1ULL << selwidth) - 1);
            Ast_गाँठ_व्यंजक* स्थिर boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? नया AstGt{nodep->fileline(), nodep->clone_वृक्ष(false),
                                  नया AstConst{nodep->fileline(),
                                               V3_संख्या{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = नया AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                नया AstSel{nodep->fileline(), nodep, 0, selwidth}};
            अगर (boundp) {
                newp = नया AstCond{nodep->fileline(), boundp,
                                   नया AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        अगर (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } अन्यथा अगर (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } अन्यथा {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        वापस newp;
    }
    Ast_गाँठ_व्यंजक* enumTestValid(Ast_गाँठ_व्यंजक* valp, AstEnumDType* enumDtp) {
        स्थिर uint64_t maxval = enum_Max_मूल्य(valp, enumDtp);
        स्थिर bool assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_गाँठ_व्यंजक* testp = nullptr;
        File_पंक्ति* स्थिर fl_novalue = नया File_पंक्ति{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, true);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, true);
        अगर (assoc) {
            AstVarRef* स्थिर tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, true, 0);
            testp = नया AstAssocSel{fl_novalue, tabRefp, valp->clone_वृक्ष_Pure(false)};
        } अन्यथा {
            स्थिर पूर्णांक selwidth = V3_संख्या::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* स्थिर tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, false, (1ULL << selwidth) - 1);
            File_पंक्ति* स्थिर fl_nowidth = नया File_पंक्ति{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, true);
            testp = नया AstCond{
                fl_novalue,
                नया AstGt{fl_nowidth, valp->clone_वृक्ष_Pure(false),
                          नया AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                नया AstConst{fl_novalue, AstConst::BitFalse{}},
                नया AstArraySel{fl_novalue, tabRefp,
                                नया AstSel{fl_novalue, valp->clone_वृक्ष_Pure(false), 0, selwidth}}};
        }
        वापस testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, स्थिर VNumRange& range) {
        PatVecMap patmap;
        पूर्णांक element = range.बाँए();
        क्रम (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            अगर (patp->keyp()) {
                अगर (स्थिर AstConst* स्थिर constp = patternConstKeyp(patp)) {
                    अगर (constp->अंक().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } अन्यथा {
                        element = constp->toSInt();
                    }
                }
            }
            स्थिर bool newEntry = patmap.emplace(element, patp).second;
            अगर (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.बायाँ_To_दायाँ_Inc();
        }
        वापस patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    व्योम patConcatConvertRecurse(AstPattern* patternp, Ast_गाँठ_व्यंजक* nodep) {
        अगर (AstConcat* स्थिर concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } अन्यथा {
            AstPatMember* स्थिर newp
                = नया AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(true);
            patternp->addItemsp(newp);
        }
    }

    व्योम makeOpenArrayShell(Ast_गाँठ_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_गाँठ_FTask* स्थिर oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_गाँठ_FTask* स्थिर newTaskp = oldTaskp->clone_वृक्ष(false);
        newTaskp->dpiOpenChild(true);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->add_अग्र_Here(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        स्थिर V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        क्रम (स्थिर स्वतः& tconnect : tconnects) {
            AstVar* स्थिर portp = tconnect.first;
            स्थिर AstArg* स्थिर argp = tconnect.second;
            स्थिर Ast_गाँठ* स्थिर pinp = argp->exprp();
            अगर (!pinp) जारी;  // Argument error we'll find later
            अगर (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_गाँठ_DType* स्थिर oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_गाँठ* nodep) {
                    अगर (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    bool markHasOpenArray(Ast_गाँठ_FTask* nodep) {
        bool hasOpen = false;
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                अगर (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(true);
                    hasOpen = true;
                }
            }
        }
        वापस hasOpen;
    }
    bool hasOpenArrayDTypeRecurse(Ast_गाँठ_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        अगर (VN_IS(nodep, UnsizedArrayDType)) वापस true;
        अगर (nodep->subDTypep()) वापस hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        वापस false;
    }
    Ast_गाँठ_DType* dtypeNotIntAtomOrVecRecurse(Ast_गाँठ_DType* nodep, bool ranged = false) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        अगर (AstBasicDType* स्थिर dtp = VN_CAST(nodep, BasicDType)) {
            अगर (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                वापस dtp;  // Packed when already packed
            अगर (dtp->keyword().isIntNumeric()) वापस nullptr;
            वापस dtp;
        } अन्यथा अगर (AstPackArrayDType* स्थिर dtp = VN_CAST(nodep, PackArrayDType)) {
            अगर (ranged) वापस dtp;  // Packed when already packed
            वापस dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), true);
        }
        वापस nodep;
    }
    Ast_गाँठ_DType* dtypeNot4StateIntegralRecurse(Ast_गाँठ_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        अगर (AstBasicDType* स्थिर dtp = VN_CAST(nodep, BasicDType)) {
            अगर (!dtp->keyword().isFourstate()) वापस dtp;
            वापस nullptr;
        } अन्यथा अगर (Ast_गाँठ_Array_DType* स्थिर dtp = VN_CAST(nodep, गाँठ_Array_DType)) {
            वापस dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } अन्यथा अगर (Ast_गाँठ_UOr_Struct_DType* स्थिर dtp = VN_CAST(nodep, गाँठ_UOr_Struct_DType)) {
            क्रम (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_गाँठ_DType* स्थिर badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                अगर (badDtp) वापस badDtp;
            }
            वापस nullptr;
        }
        वापस nodep;
    }
    व्योम checkForceReleaseLhs(Ast_गाँठ* nodep, Ast_गाँठ* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        अगर (Ast_गाँठ* स्थिर selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    व्योम assert_At_व्यंजक(Ast_गाँठ* nodep) {
        अगर (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    व्योम assertAtStatement(Ast_गाँठ* nodep) {
        अगर (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    व्योम checkConstantOrReplace(Ast_गाँठ* nodep, bool noFourState, स्थिर string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* स्थिर constp = VN_CAST(nodep, Const);
        अगर (!constp || (noFourState && constp->अंक().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(धकेलो_Deletep(nodep), nodep, constp);
            वापस;
        }
    }
    Ast_गाँठ* गाँठ_For_Unsized_Warning(Ast_गाँठ* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        अगर (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            वापस nodep->op1p();
        } अन्यथा अगर (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            वापस nodep->op2p();
        }
        वापस nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_गाँठ* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        अगर (VL_LIKELY(!nodep)) वापस nullptr;
        अगर (स्वतः* स्थिर refp = VN_CAST(nodep, RefDType)) {
            अगर (refp->typedefp() == typedefp) वापस refp;
        }
        अगर (स्वतः* स्थिर refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) वापस refp;
        अगर (स्वतः* स्थिर refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) वापस refp;
        अगर (स्वतः* स्थिर refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) वापस refp;
        अगर (स्वतः* स्थिर refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) वापस refp;
        वापस nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    अटल bool isIfaceArrayDType(स्थिर Ast_गाँठ_DType* dtypep) {
        अगर (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) वापस false;
        स्थिर AstIfaceRefDType* स्थिर irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        वापस irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    अटल std::string unpacked_Array_आकार(स्थिर Ast_गाँठ_DType* dtypep) {
        std::string आकार;
        dtypep = dtypep->skipRefp();
        जबतक (स्थिर AstUnpackArrayDType* स्थिर arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            अगर (!आकार.empty()) आकार += "x";
            आकार += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        वापस आकार;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    अटल Ast_गाँठ_व्यंजक* newIfaceArrayElement(Ast_गाँठ_व्यंजक* fromp, पूर्णांक bit) {
        AstConsArray* स्थिर consp = VN_CAST(fromp, ConsArray);
        अगर (!consp || !isIfaceArrayDType(consp->dtypep())) वापस nullptr;
        स्थिर AstUnpackArrayDType* स्थिर arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        अगर (bit < 0 || bit >= arrp->elementsConst()) वापस nullptr;
        वापस consp->valuep(bit)->clone_वृक्ष(false);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_गाँठ_व्यंजक* new_Iface_Array_मूल्य(स्थिर Ast_गाँठ_Var_Ref* refp, Ast_गाँठ_DType* dtypep,
                                    स्थिर std::string& name, स्थिर std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_पंक्ति* स्थिर flp = refp->fileline();
        AstUnpackArrayDType* स्थिर arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        अगर (!arrp) {
            AstVar* स्थिर varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_गाँठ_Var_Ref* newp;
            अगर (स्थिर AstVarXRef* स्थिर xrefp = VN_CAST(refp, VarXRef)) {
                newp = नया AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } अन्यथा {
                newp = नया AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            वापस newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* स्थिर consp = नया AstConsArray{flp, arrp};
        स्थिर VNumRange range = arrp->declRange();
        क्रम (स्थिर पूर्णांक i : range.seq_बायाँ_To_दायाँ()) {
            स्थिर std::string s = name + "__BRA__" + Ast_गाँठ::encode_संख्या(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_मूल्य(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        वापस consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    व्योम replaceSplitIfaceArrayRef(Ast_गाँठ_Var_Ref* nodep) {
        AstVar* स्थिर varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        अगर (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_मूल्य(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    व्योम expandIfaceArrayPinDimensions(AstPin* nodep, स्थिर Ast_गाँठ_DType* dtypep,
                                       Ast_गाँठ_व्यंजक* conp, स्थिर std::string& suffix,
                                       AstVar*& elemVarpr) {
        स्थिर AstUnpackArrayDType* स्थिर arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        अगर (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* स्थिर newp = nodep->clone_वृक्ष(false);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            अगर (conp) newp->exprp(conp->clone_वृक्ष(false));
            nodep->add_अग्र_Here(newp);
            वापस;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        स्थिर VNumRange range = arrp->declRange();
        क्रम (पूर्णांक n = range.elements() - 1; n >= 0; --n) {
            स्थिर पूर्णांक i = range.बाँए() + n * range.बायाँ_To_दायाँ_Inc();
            स्थिर std::string s = suffix + "__BRA__" + Ast_गाँठ::encode_संख्या(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_गाँठ_व्यंजक* स्थिर elemConnp = [&]() -> Ast_गाँठ_व्यंजक* {
                अगर (!conp) वापस nullptr;
                स्थिर AstUnpackArrayDType* स्थिर conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                स्थिर पूर्णांक elements = conArrp->elementsConst();
                // Counted from lo
                Ast_गाँठ_व्यंजक* स्थिर resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                वापस resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    व्योम expandIfaceArrayPin(AstPin* nodep) {
        AstVar* स्थिर portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        क्रम (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_गाँठ_व्यंजक* स्थिर exprp = VN_AS(nodep->exprp(), गाँठ_व्यंजक);
        अगर (exprp) धकेलो_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        स्थिर bool connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpacked_Array_आकार(exprp->dtypep()) == unpacked_Array_आकार(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_गाँठ* userIterateSubtreeReturnEdits(Ast_गाँठ* nodep, WidthVP* vup) {
        अगर (!nodep) वापस nullptr;
        Ast_गाँठ* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        वापस ret;
    }
    व्योम userIterate(Ast_गाँठ* nodep, WidthVP* vup) {
        अगर (!nodep) वापस;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    व्योम user_Iterate_And_अग्र(Ast_गाँठ* nodep, WidthVP* vup) {
        अगर (!nodep) वापस;
        अगर (nodep->didWidth()) वापस;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate_And_अग्र_Null(nodep);
    }
    व्योम userIterateChildren(Ast_गाँठ* nodep, WidthVP* vup) {
        अगर (!nodep) वापस;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    व्योम userIterateChildrenBackwardsConst(Ast_गाँठ* nodep, WidthVP* vup) {
        अगर (!nodep) वापस;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

खुला:
    // CONSTRUCTORS
    WidthVisitor(bool paramsOnly,  // [in] TRUE if we are considering parameters only.
                 bool doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enum_अस्थायी_Names{"__Venum"}
        , m_inside_अस्थायी_Names{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_गाँठ* mainAcceptEdit(Ast_गाँठ* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        क्रम (AstVar* स्थिर varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(धकेलो_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.सफा();
        वापस nodep;
    }
    अटल bool lowerAsFixedAggregate(स्थिर Ast_गाँठ_DType* स्थिर dtypep) {
        वापस dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    व्योम streamAssignLower(Ast_गाँठ_Assign* nodep) {
        Ast_गाँठ_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        अगर (Ast_गाँठ_Stream* स्थिर streamp = VN_CAST(nodep->rhsp(), गाँठ_Stream)) {
            Ast_गाँठ_DType* स्थिर lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            स्थिर पूर्णांक lwidth = lhsDTypeSkippedRefp->widthStream();
            स्थिर पूर्णांक rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            अगर (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            अगर (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                स्थिर पूर्णांक queue_Element_आकार = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queue_Element_आकार <= lwidth, nodep, "LHS < RHS");
            }
            अगर (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    नया AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        अगर (Ast_गाँठ_Stream* स्थिर streamp = VN_CAST(nodep->lhsp(), गाँठ_Stream)) {
            स्थिर Ast_गाँठ_DType* स्थिर rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_गाँठ_DType* स्थिर lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            स्थिर पूर्णांक lwidth = lhsStreamDTypep->widthStream();
            स्थिर पूर्णांक rwidth = rhsDTypep->widthStream();
            अगर (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            अगर (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_गाँठ_व्यंजक* स्थिर streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_गाँठ_व्यंजक* स्थिर dstp = streamp->lhsp()->unlinkFrBack();
                Ast_गाँठ_व्यंजक* srcp = nodep->rhsp()->unlinkFrBack();
                अगर (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_गाँठ_व्यंजक::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } अन्यथा {
                    अगर (srcp->width() > lwidth) {
                        srcp = नया AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(धकेलो_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(नया AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } अन्यथा अगर (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(नया AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = शेष;
};

//######################################################################
// Width class functions

व्योम V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        स्थिर WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        स्थिर VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{false, false};
        (व्योम)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (व्योम)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("width", 0, dump_वृक्ष_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_गाँठ* V3Width::widthParamsEdit(Ast_गाँठ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{true, false};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    वापस nodep;
}

व्योम V3Width::streamAssignLowerEdit(Ast_गाँठ_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{false, false};
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
Ast_गाँठ* V3Width::widthGenerateParamsEdit(
    Ast_गाँठ* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{true, true};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    वापस nodep;
}

Ast_गाँठ* V3Width::selectNonConstantRecurse(Ast_गाँठ* nodep, bool inSel) {
    // If node has a non-constant select, return that select
    Ast_गाँठ* resultp = nullptr;
    अगर (Ast_गाँठ_Sel* स्थिर anodep = VN_CAST(nodep, गाँठ_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        अगर (resultp) वापस resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), true);
    } अन्यथा अगर (AstSel* स्थिर anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        अगर (resultp) वापस resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), true);
    } अन्यथा अगर (Ast_गाँठ_Var_Ref* स्थिर anodep = VN_CAST(nodep, गाँठ_Var_Ref)) {
        अगर (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) वापस anodep;
    } अन्यथा {
        अगर (Ast_गाँठ* स्थिर refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        अगर (resultp) वापस resultp;
        अगर (Ast_गाँठ* स्थिर refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        अगर (resultp) वापस resultp;
        अगर (Ast_गाँठ* स्थिर refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        अगर (resultp) वापस resultp;
        अगर (Ast_गाँठ* स्थिर refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    वापस resultp;
}
