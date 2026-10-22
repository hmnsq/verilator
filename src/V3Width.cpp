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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Width.h"

#ଅନ୍ତର୍ଭାଵ "V3Assert.h"
#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Begin.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkLValue.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"
#ଅନ୍ତର୍ଭାଵ "V3Number.h"
#ଅନ୍ତର୍ଭାଵ "V3Randomize.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"
#ଅନ୍ତର୍ଭାଵ "V3Task.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"
#ଅନ୍ତର୍ଭାଵ "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#ଅନ୍ତର୍ଭାଵ "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି*, AstClass*> m_cache;

    AstClass* findCached(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ nullptr;
        ଅଚଳ auto it = m_cache.find(nodep);
        ଯଦି (it != m_cache.end()) ଫେରନ୍ତୁ it->second;

        AstClass* classp = nullptr;
        ଯଦି (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } ଅନ୍ୟଥା ଯଦି (AstClass* ଅଚଳ parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } ଅନ୍ୟଥା ଯଦି (AstClassPackage* ଅଚଳ packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } ଅନ୍ୟଥା {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        ଫେରନ୍ତୁ classp;
    }

ସର୍ବସାଧାରଣ:
    AstClass* find(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(nodep, Class)) ଫେରନ୍ତୁ classp;
        ଯଦି (AstClassPackage* ଅଚଳ packagep = VN_CAST(nodep, ClassPackage)) {
            ଫେରନ୍ତୁ packagep->classp();
        }
        ଫେରନ୍ତୁ findCached(nodep);
    }
};

//######################################################################

ଗଣନା Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, ଅଚଳ Stage& rhs) {
    ଫେରନ୍ତୁ str << ("-PFB"[static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(rhs)]);
}

ଗଣନା Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, ଅଚଳ Determ& rhs) {
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ s_det[] = {"SELF", "CNTX", "ASSN"};
    ଫେରନ୍ତୁ str << s_det[rhs];
}

ଗଣନା StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, ସନ୍ଦେଶ) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               ସନ୍ଦେଶ)

//######################################################################
// Width state, as a visitor of each AstNode

ଶ୍ରେଣୀ WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ m_dtypep;  // Parent's data type to resolve to
    ଅଚଳ Stage m_stage;  // If true, report errors
    ଅଚଳ StreamUse m_streamUse;  // Current expression may be a stream concat

ସର୍ବସାଧାରଣ:
    WidthVP(Ast_ଗଣ୍ଠି_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        ଯଦି (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { ଫେରନ୍ତୁ ଏହି; }
    ତାର୍କିକ selfDtm() ଅଚଳ { ଫେରନ୍ତୁ m_dtypep == nullptr; }
    StreamUse streamUse() ଅଚଳ { ଫେରନ୍ତୁ m_streamUse; }
    Ast_ଗଣ୍ଠି_DType* dtypep() ଅଚଳ {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        ଫେରନ୍ତୁ m_dtypep;
    }
    Ast_ଗଣ୍ଠି_DType* dtypeNullp() ଅଚଳ { ଫେରନ୍ତୁ m_dtypep; }
    Ast_ଗଣ୍ଠି_DType* dtypeNullSkipRefp() ଅଚଳ {
        Ast_ଗଣ୍ଠି_DType* dtp = dtypeNullp();
        ଯଦି (dtp) dtp = dtp->skipRefp();
        ଫେରନ୍ତୁ dtp;
    }
    Ast_ଗଣ୍ଠି_DType* dtypeOverridep(Ast_ଗଣ୍ଠି_DType* ଅଚଳ defaultp) ଅଚଳ {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        ଯଦି (!m_dtypep) ଫେରନ୍ତୁ defaultp;
        ଯଦି (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            ଫେରନ୍ତୁ defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        ଫେରନ୍ତୁ m_dtypep;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା width() ଅଚଳ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ଫେରନ୍ତୁ m_dtypep->width();
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin() ଅଚଳ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ଫେରନ୍ତୁ m_dtypep->widthMin();
    }
    ତାର୍କିକ prelim() ଅଚଳ { ଫେରନ୍ତୁ m_stage & PRELIM; }
    ତାର୍କିକ final() ଅଚଳ { ଫେରନ୍ତୁ m_stage & FINAL; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ {
        ଯଦି (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } ଅନ୍ୟଥା {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, ଅଚଳ WidthVP* vup) {
    ଯଦି (vup) vup->dump(str);
    ଫେରନ୍ତୁ str;
}

//######################################################################

ଶ୍ରେଣୀ WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    ଶୂନ୍ୟ clearWidthRecurse(Ast_ଗଣ୍ଠି* nodep) {
        ପାଇଁ (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(ମିଥ୍ୟା);
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op1p()) clearWidthRecurse(refp);
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op2p()) clearWidthRecurse(refp);
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op3p()) clearWidthRecurse(refp);
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    ଆଭାସୀ ~WidthClearVisitor() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_ଗଣ୍ଠି_iterate_instead_of_Ast_ଗଣ୍ଠି_accept

//######################################################################

ଶ୍ରେଣୀ WidthVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // TYPES
    using PatVecMap = std::map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, AstPatMember*>;
    using DTypeMap = std::map<ଅଚଳ std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ଅଚଳ AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    ତାର୍କିକ m_underFork = ମିଥ୍ୟା;  // Visiting under a fork
    ତାର୍କିକ m_underSExpr = ମିଥ୍ୟା;  // Visiting under a sequence expression
    ତାର୍କିକ m_underPackedArray = ମିଥ୍ୟା;  // Visiting under a AstPackArrayDType
    ତାର୍କିକ m_underMemberSel = ମିଥ୍ୟା;  // Viting under a MemberSel
    Ast_ଗଣ୍ଠି* m_seqUnsupp = nullptr;  // Property has unsupported node
    ତାର୍କିକ m_hasSExpr = ମିଥ୍ୟା;  // Property has a sequence expression
    ଅଚଳ AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    ଅଚଳ AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ଗଣ୍ଠି_Module* m_modep = nullptr;  // Current module
    ଅଚଳ AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_ଗଣ୍ଠି_Procedure* m_procedurep = nullptr;  // Current final/always
    ଅଚଳ AstWith* m_withp = nullptr;  // Current 'with' statement
    ଅଚଳ AstFunc* m_funcp = nullptr;  // Current function
    ଅଚଳ AstAttrOf* m_attrp = nullptr;  // Current attribute
    ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    ଅଚଳ ତାର୍କିକ m_paramsOnly;  // Computing parameter value; limit operation
    ଅଚଳ ତାର୍କିକ m_doGenerate;  // Do errors later inside generate statement
    ତାର୍କିକ m_streamConcat = ମିଥ୍ୟା;  // True if visiting arguments of stream concatenation
    ତାର୍କିକ m_inParamOverride = ମିଥ୍ୟା;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ଅଚଳ Ast_ଗଣ୍ଠି_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<ଅଚଳ Ast_ଗଣ୍ଠି_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<ଅଚଳ AstVar*> m_curModVars;  // Variables declared in current module

    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    ଗଣନା ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    ଗଣନା ଶ୍ରେଣୀ ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    ସ୍ଥିର StreamUse streamUseForDeterm(Determ determ) {
        ଫେରନ୍ତୁ determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() ଅଚଳ { ଫେରନ୍ତୁ m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    ସ୍ଥିର ଶୂନ୍ୟ packIfUnpacked(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nodep) {
        ଯଦି (AstUnpackArrayDType* ଅଚଳ unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements_ଅଙ୍କ = unpackDTypep->arrayUnpackedElements();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା unpackMinBits = elements_ଅଙ୍କ * unpackDTypep->subDTypep()->widthMin();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା unpackBits = elements_ଅଙ୍କ * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(ନୂତନ AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* fromDTypep(Ast_ଗଣ୍ଠି* fromp) {
        ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = VN_CAST(fromp, ଗଣ୍ଠି_DType))
            ଫେରନ୍ତୁ dtypep->skipRefOrNullp();
        ଫେରନ୍ତୁ fromp ? fromp->dtypep() : nullptr;
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
    ଶୂନ୍ୟ visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    ଶୂନ୍ୟ visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    ଶୂନ୍ୟ visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    ଶୂନ୍ୟ visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    ଶୂନ୍ୟ visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    ଶୂନ୍ୟ visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    ଶୂନ୍ୟ visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    ଶୂନ୍ୟ visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ଶୂନ୍ୟ visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ଶୂନ୍ୟ visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ଶୂନ୍ୟ visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ଶୂନ୍ୟ visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ଶୂନ୍ୟ visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ଶୂନ୍ୟ visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ଶୂନ୍ୟ visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ଶୂନ୍ୟ visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    ଶୂନ୍ୟ visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    ଶୂନ୍ୟ visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    ଶୂନ୍ୟ visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    ଶୂନ୍ୟ visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    ଶୂନ୍ୟ visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    ଶୂନ୍ୟ visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    ଶୂନ୍ୟ visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    ଶୂନ୍ୟ visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    ଶୂନ୍ୟ visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    ଶୂନ୍ୟ visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    ଶୂନ୍ୟ visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    ଶୂନ୍ୟ visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    ଶୂନ୍ୟ visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    ଶୂନ୍ୟ visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    ଶୂନ୍ୟ visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    ଶୂନ୍ୟ visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    ଶୂନ୍ୟ visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    ଶୂନ୍ୟ visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    ଶୂନ୍ୟ visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    ଶୂନ୍ୟ visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, ସତ୍ୟ, ମିଥ୍ୟା); }
    ଶୂନ୍ୟ visit(AstSub* nodep) override { visit_add_sub_replace(nodep, ସତ୍ୟ, ମିଥ୍ୟା); }
    ଶୂନ୍ୟ visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, ସତ୍ୟ, ସତ୍ୟ); }
    ଶୂନ୍ୟ visit(AstMul* nodep) override { visit_add_sub_replace(nodep, ସତ୍ୟ, ମିଥ୍ୟା); }
    // These can't promote to real
    ଶୂନ୍ୟ visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, ମିଥ୍ୟା, ସତ୍ୟ); }
    ଶୂନ୍ୟ visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, ମିଥ୍ୟା, ସତ୍ୟ); }
    ଶୂନ୍ୟ visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା); }
    ଶୂନ୍ୟ visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, ମିଥ୍ୟା, ସତ୍ୟ); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    ଶୂନ୍ୟ visit(AstNegate* nodep) override { visit_negate_not(nodep, ସତ୍ୟ); }
    // Unary never real
    ଶୂନ୍ୟ visit(AstNot* nodep) override { visit_negate_not(nodep, ମିଥ୍ୟା); }

    // Real: inputs and output real
    ଶୂନ୍ୟ visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    ଶୂନ୍ୟ visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    ଶୂନ୍ୟ visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    ଶୂନ୍ୟ visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    ଶୂନ୍ୟ visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    ଶୂନ୍ୟ visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    ଶୂନ୍ୟ visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    ଶୂନ୍ୟ visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    ଶୂନ୍ୟ visit(AstShiftL* nodep) override { visit_shift(nodep); }
    ଶୂନ୍ୟ visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    ଶୂନ୍ୟ visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    ଶୂନ୍ୟ visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    ଶୂନ୍ୟ visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    ଶୂନ୍ୟ visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    ଶୂନ୍ୟ visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    ଶୂନ୍ୟ visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        ଯଦି (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    ଶୂନ୍ୟ visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ଯଦି (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    ଶୂନ୍ୟ visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ଯଦି (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    ଶୂନ୍ୟ visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ଯଦି (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    ଶୂନ୍ୟ visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ଯଦି (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    ଶୂନ୍ୟ visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    ଶୂନ୍ୟ visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            ଯଦି (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } ଅନ୍ୟଥା {
                nodep->dtypeSetInteger();
            }
        }
    }
    ଶୂନ୍ୟ visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    ଶୂନ୍ୟ visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    ଶୂନ୍ୟ visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    ଶୂନ୍ୟ visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    ଶୂନ୍ୟ visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    ଶୂନ୍ୟ visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            ନୂତନ AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    ସ୍ଥିର ତାର୍କିକ isOverridePattern(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (ଅଚଳ AstPattern* ଅଚଳ patternp = VN_CAST(nodep, Pattern)) {
            ଫେରନ୍ତୁ !patternp->childDTypep();
        }
        ଅଚଳ AstCond* ଅଚଳ condp = VN_CAST(nodep, Cond);
        ଫେରନ୍ତୁ condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା errors = V3Error::error_ଗଣନା();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = ମିଥ୍ୟା;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->condp(), Const);
            ଅଚଳ ତାର୍କିକ known = constp && !constp->ଅଙ୍କ().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            ଯଦି (!known && V3Error::error_ଗଣନା() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            ଫେରନ୍ତୁ;
        }
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
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
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ thenDTypep = nodep->thenp()->dtypep();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ elseDTypep = nodep->elsep()->dtypep();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ elseDTypeSkippedp = elseDTypep->skipRefp();
            ଯଦି (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } ଅନ୍ୟଥା ଯଦି (nodep->thenp()->is_Class_Handle_ମୂଲ୍ୟ()
                       || nodep->elsep()->is_Class_Handle_ମୂଲ୍ୟ()) {
                Ast_ଗଣ୍ଠି_DType* commonClassTypep = nullptr;
                ଯଦି (nodep->thenp()->is_Class_Handle_ମୂଲ୍ୟ() && nodep->elsep()->is_Class_Handle_ମୂଲ୍ୟ()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_ଗଣ୍ଠି::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                ଯଦି (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } ଅନ୍ୟଥା {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } ଅନ୍ୟଥା ଯଦି (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                ଯଦି (Ast_ଗଣ୍ଠି::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } ଅନ୍ୟଥା {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    ଯଦି (!Ast_ଗଣ୍ଠି::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } ଅନ୍ୟଥା ଯଦି (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } ଅନ୍ୟଥା ଯଦି (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } ଅନ୍ୟଥା {
                ଅଚଳ ତାର୍କିକ isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                ତାର୍କିକ handled = ମିଥ୍ୟା;
                ଅଚଳ ତାର୍କିକ isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ thenEnumDTypep = thenDTypep->skipRefToEnump();
                ଯଦି (VN_IS(thenDTypeSkippedp, ଗଣ୍ଠି_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_ବୃକ୍ଷ(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_ବୃକ୍ଷ(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    ଯଦି (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = ସତ୍ୟ;
                    } ଅନ୍ୟଥା {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(ନୂତନ File_ଧାଡ଼ି{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, ସତ୍ୟ);
                    }
                }
                ଯଦି (!handled) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = std::max(thenDTypep->width(), elseDTypep->width());
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    ଅଚଳ VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ଅଚଳ ତାର୍କିକ isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    ଯଦି ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } ଅନ୍ୟଥା ଯଦି ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } ଅନ୍ୟଥା {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        ଯଦି (m_vup->final()) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    ଶୂନ୍ୟ visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* ଅଚଳ newp = ନୂତନ AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ଫେରନ୍ତୁ;
        }

        // Concat handling
        ଯଦି (m_vup->prelim()) {
            ଯଦି (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            ଯଦି (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } ଅନ୍ୟଥା {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                ଯଦି (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            ଯଦି (ଅଚଳ AstReplicate* ଅଚଳ repp = VN_CAST(nodep->lhsp(), Replicate)) {
                ଯଦି (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (ଅଚଳ AstReplicate* ଅଚଳ repp = VN_CAST(nodep->rhsp(), Replicate)) {
                ଯଦି (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
        }
        ଯଦି (m_vup->final()) {
            ଯଦି (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp = nodep->rhsp()->unlinkFrBack();
                ଯଦି (!lhsp->isString()) lhsp = ନୂତନ AstCvtPackString{lhsp->fileline(), lhsp};
                ଯଦି (!rhsp->isString()) rhsp = ନୂତନ AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ଗଣ୍ଠି_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ଶୂନ୍ୟ visit(AstConcatN* nodep) override {
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        ଯଦି (m_vup->final()) {
            nodep->didWidth(ସତ୍ୟ);
            ଯଦି (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ଗଣ୍ଠି_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ଶୂନ୍ୟ visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstDelay* nodep) override {
        ଯଦି (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            ଯଦି (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            ଯଦି (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                ଫେରନ୍ତୁ;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->fileline()->timingOn()) {
            ଯଦି (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } ଅନ୍ୟଥା {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        ଯଦି (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        ଯଦି (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ଅଚଳ newp = ନୂତନ AstBegin{nodep->fileline(), nodep->name(), nullptr, ମିଥ୍ୟା};
            nodep->replaceWith(newp);
            ଯଦି (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            ଯଦି (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            ଯଦି (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = ସତ୍ୟ;
            iterateChildren(nodep);
        } ଅନ୍ୟଥା ଯଦି (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        } ଅନ୍ୟଥା {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    ଶୂନ୍ୟ visitWaitOrDisableFork(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (nodep->fileline()->timingOn()) {
            ଯଦି (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    ଶୂନ୍ୟ visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    ଶୂନ୍ୟ visit(AstToLowerN* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    ଶୂନ୍ୟ visit(AstToUpperN* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    ଶୂନ୍ୟ visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->countp(), Const);
            ଯଦି (constp) {
                ଯଦି (constp->ଅଙ୍କ().isFourState()
                    || (constp->dtypep()->isSigned() && constp->ଅଙ୍କ().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } ଅନ୍ୟଥା {
                    times = constp->toSInt();
                }
            }

            Ast_ଗଣ୍ଠି_DType* ଅଚଳ vdtypep = m_vup->dtypeNullSkipRefp();
            ଯଦି (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                ଯଦି (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                ଯଦି (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                } ଅନ୍ୟଥା {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* ଅଚଳ newp = ନୂତନ AstPattern{
                        nodep->fileline(),
                        ନୂତନ AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            ଯଦି (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } ଅନ୍ୟଥା {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            ଯଦି ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_ଗଣ୍ଠି* ଅଚଳ newp
                    = ନୂତନ AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା {
                ଯଦି (!constp) nodep->v3error("Replication value isn't a constant.");
                ଯଦି (times == 0
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
        ଯଦି (m_vup->final()) {
            ଯଦି (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ଗଣ୍ଠି_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ଶୂନ୍ୟ visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        ଯଦି (m_vup->final()) {
            ଯଦି (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ଗଣ୍ଠି_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Dist_Biop* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Dist_Triop* nodep) override {
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    ତାର୍କିକ streamImplicitUseAllowed(ଅଚଳ Ast_ଗଣ୍ଠି_Stream* nodep) ଅଚଳ {
        ଯଦି (m_streamConcat) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (m_vup && m_vup->streamUse() != STREAM_USE_NONE) ଫେରନ୍ତୁ ସତ୍ୟ;

        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ backp = nodep->backp();
        ଯଦି (!backp) ଫେରନ୍ତୁ ସତ୍ୟ;  // Error elsewhere

        ଫେରନ୍ତୁ VN_IS(backp, ଗଣ୍ଠି_Assign);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            m_streamConcat = ସତ୍ୟ;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = ମିଥ୍ୟା;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->rhsp(), Const)) {
                ଯଦି (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } ଅନ୍ୟଥା {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            ଯଦି (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, ଗଣ୍ଠି_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, ଗଣ୍ଠି_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } ଅନ୍ୟଥା ଯଦି (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } ଅନ୍ୟଥା {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ଯଦି (m_vup->final()) {
            ଯଦି (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            ଯଦି (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ଗଣ୍ଠି_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    ଶୂନ୍ୟ visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), ସତ୍ୟ,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), ସତ୍ୟ,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        ଯଦି (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->elementsConst();
            ଯଦି (nodep->fromBracket() && nodep->ବାମ_Const() > nodep->ଡାହାଣ_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } ଅନ୍ୟଥା ଯଦି (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ଅଚଳ ତାର୍କିକ inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ଅଚଳ ତାର୍କିକ inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ଅଚଳ ତାର୍କିକ inTypeTable = !m_modep;
            ଅଚଳ Ast_ଗଣ୍ଠି* basep = nodep->backp();
            ଯେତେବେଳେ (VN_IS(basep, Range)) basep = basep->backp();
            ଯଦି (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->ବାମ_Const() << ":" << nodep->ଡାହାଣ_Const()
                                            << "]");
            }
        }
    }

    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), ମିଥ୍ୟା);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->widthConst();
            ଯଦି (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                ଫେରନ୍ତୁ;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ଅଚଳ ତାର୍କିକ isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ଅଚଳ ତାର୍କିକ inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            ଯଦି (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                ଠେଲ_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(ନୂତନ AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            ଯଦି (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                ଯଦି (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ମିଥ୍ୟା /*noerror*/);
                } ଅନ୍ୟଥା {
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
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା frommsb = nodep->fromp()->width() - 1;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା fromlsb = 0;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            ଯଦି (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } ଅନ୍ୟଥା {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା selwidth = V3_ସଂଖ୍ୟା::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            ଯଦି (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                ଯଦି (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            ଯଦି (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                ଯଦି (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } ଅନ୍ୟଥା ଯଦି (!inParameterizedTemplate) {
                    ଯଦି (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } ଅନ୍ୟଥା {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                ଯଦି (!isWriteSelect) {
                    // Extend it
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା extendTo = nodep->msbConst() + 1;
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ମିଥ୍ୟା /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            ଯଦି (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                ମିଥ୍ୟା /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            ଯଦି (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->bitp(), Const)) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = constp->toSInt();
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    ଯଦି (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
            //
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା frommsb;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା fromlsb;
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                ଯଦି (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                ଯଦି (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } ଅନ୍ୟଥା {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା selwidth = V3_ସଂଖ୍ୟା::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            ଯଦି (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                ଯଦି (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            ଯଦି (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ଯଦି (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    ଯଦି (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                ମିଥ୍ୟା /*NOWARN*/);
            }
        }
    }

    ଶୂନ୍ୟ visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ଅଚଳ AstAssocArrayDType* ଅଚଳ adtypep = VN_CAST(fromDtp, AssocArrayDType);
            ଯଦି (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->ଚାବି_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ଶୂନ୍ୟ visit(AstAlias* nodep) override {
        ଯଦି (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        ଅଚଳ auto check_If_ପଦସମଷ୍ଟି_Ok = [ଏହି](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp) {
            ଯଦି (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }

            ଅଚଳ AstVarRef* ଅଚଳ varRefp = VN_CAST(exprp, VarRef);
            ଯଦି (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            AstVar* ଅଚଳ varp = varRefp->varp();
            ଯଦି (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଯଦି (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            } ଅନ୍ୟଥା {
                m_aliasedVars.ଭର୍ତ୍ତି(varp);
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        };

        check_If_ପଦସମଷ୍ଟି_Ok(nodep->itemsp());
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ firstItemDtypep = nodep->itemsp()->dtypep();
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_ପଦସମଷ୍ଟି_Ok(VN_AS(itemp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
            ଯଦି (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    ଶୂନ୍ୟ visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ଅଚଳ AstWildcardArrayDType* ଅଚଳ adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            ଯଦି (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ଅଚଳ AstBasicDType* ଅଚଳ basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            ଯଦି (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ଶୂନ୍ୟ visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            ଯଦି (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ newDtp
                = ନୂତନ AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          ନୂତନ AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            ଯଦି (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ଅଚଳ VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                ଯଦି ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(ନୂତନ AstConsArray{
                        nodep->fileline(), newDtp,
                        ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                } ଅନ୍ୟଥା ଯଦି ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            ଯଦି (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ଅଚଳ fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ଅଚଳ newp = ନୂତନ AstConsArray{nodep->fileline(), newDtp};
                ଅଚଳ VNumRange range = nodep->declRange();
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        }
    }

    ଶୂନ୍ୟ visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ଗଣ୍ଠି* ଅଚଳ selp = V3Width::widthSelNoIterEdit(nodep);
        ଯଦି (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            ଫେରନ୍ତୁ;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    ଶୂନ୍ୟ visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ଗଣ୍ଠି* ଅଚଳ selp = V3Width::widthSelNoIterEdit(nodep);
        ଯଦି (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ଫେରନ୍ତୁ;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    ଶୂନ୍ୟ visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ଗଣ୍ଠି* ଅଚଳ selp = V3Width::widthSelNoIterEdit(nodep);
        ଯଦି (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ଫେରନ୍ତୁ;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    ଶୂନ୍ୟ visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ଗଣ୍ଠି* ଅଚଳ selp = V3Width::widthSelNoIterEdit(nodep);
        ଯଦି (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ଫେରନ୍ତୁ;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    ଶୂନ୍ୟ visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        ଯଦି (m_vup && m_vup->prelim()) {
            ଯଦି (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } ଅନ୍ୟଥା ଯଦି (nodep->ଅଙ୍କ().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } ଅନ୍ୟଥା ଯଦି (nodep->ଅଙ୍କ().isString()) {
                nodep->dtypeSetString();
            } ଅନ୍ୟଥା ଯଦି (nodep->ଅଙ୍କ().isDouble()) {
                nodep->dtypeChgWidth(nodep->ଅଙ୍କ().width(), nodep->ଅଙ୍କ().sized()
                                                               ? nodep->ଅଙ୍କ().width()
                                                               : nodep->ଅଙ୍କ().widthToFit());
            } ଅନ୍ୟଥା {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->ଅଙ୍କ().width(),
                    nodep->ଅଙ୍କ().sized() ? nodep->ଅଙ୍କ().width() : nodep->ଅଙ୍କ().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->ଅଙ୍କ().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    ଶୂନ୍ୟ visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        ଯଦି (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    ଶୂନ୍ୟ visit(AstFell* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ସତ୍ୟ);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstFalling* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ସତ୍ୟ);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstFuture* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ସତ୍ୟ);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    ଶୂନ୍ୟ visit(AstPast* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ସତ୍ୟ);
            nodep->dtypeFrom(nodep->exprp());
            ଯଦି (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ ସତ୍ୟ);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->ticksp(), Const);
                ଯଦି (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                } ଅନ୍ୟଥା ଯଦି (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                } ଅନ୍ୟଥା ଯଦି (V3AssertCommon::check_Sva_ମୂଲ୍ୟ_Limit(nodep, constp, "$past tick value")) {
                    ଯଦି (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    ଶୂନ୍ୟ visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ଅଚଳ AstConst* ଅଚଳ minConstp = VN_CAST(nodep->countp(), Const);
            ଯଦି (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } ଅନ୍ୟଥା ଯଦି (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } ଅନ୍ୟଥା ଯଦି (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            ଯଦି (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ଅଚଳ AstConst* ଅଚଳ maxConstp = VN_CAST(nodep->maxCountp(), Const);
                ଯଦି (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } ଅନ୍ୟଥା ଯଦି (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } ଅନ୍ୟଥା ଯଦି (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* propp = nodep->propp();
            ଯଦି (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            ଯଦି (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ଅଚଳ ତାର୍କିକ loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ଅଚଳ ତାର୍କିକ hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            ଯଦି (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_ଗଣ୍ଠି* ଅଚଳ boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ଫେରନ୍ତୁ;
            }
            ଯଦି (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ଫେରନ୍ତୁ;
            }
            ଅଚଳ AstConst* ଅଚଳ loConstp = VN_CAST(nodep->loBoundp(), Const);
            ଅଚଳ AstConst* ଅଚଳ hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            ଯଦି (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ଯଦି (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ଯଦି (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } ଅନ୍ୟଥା ଯଦି (loConstp) {
                V3AssertCommon::check_Sva_ମୂଲ୍ୟ_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            ଯଦି (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_ମୂଲ୍ୟ_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            ଯଦି (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ତାର୍କିକ hasPropertyOp = propp->isMultiCycleSva();
            ଯଦି (!hasPropertyOp) {
                propp->foreach([&](ଅଚଳ Ast_ଗଣ୍ଠି* np) {
                    ଯଦି (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = ସତ୍ୟ;
                    }
                });
            }
            ଯଦି (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstRising* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ସତ୍ୟ);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstRose* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ସତ୍ୟ);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    ଶୂନ୍ୟ visit(AstSampled* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ସତ୍ୟ);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    ଶୂନ୍ୟ visit(AstSetuphold* nodep) override {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        ଯଦି (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        ଯଦି (nodep->deldatap()) {
            ଯଦି (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } ଅନ୍ୟଥା {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        ଯଦି (!newp) {
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        nodep->replaceWith(ନୂତନ AstAlways{newp});
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_ଧାଡ଼ି* ଅଚଳ flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ evp,
                                         Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ delp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = delp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = evp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        UASSERT_OBJ(VN_IS(lhsp, ଗଣ୍ଠି_Var_Ref) || VN_IS(lhsp, ଗଣ୍ଠି_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* varRefp = VN_CAST(lhsp, ଗଣ୍ଠି_Var_Ref)) {
            ଯଦି (varRefp->varp()->direction() == VDirection::ନିବେଶ) {
                VL_DO_DANGLING(ଠେଲ_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ଠେଲ_Deletep(rhsp), rhsp);
                ଫେରନ୍ତୁ nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        ଯଦି (Ast_ଗଣ୍ଠି_Pre_Sel* selp = VN_CAST(lhsp, ଗଣ୍ଠି_Pre_Sel)) {
            ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* varRefp = VN_CAST(selp->fromp(), ଗଣ୍ଠି_Var_Ref)) {
                ଯଦି (varRefp->varp()->direction() == VDirection::ନିବେଶ) {
                    VL_DO_DANGLING(ଠେଲ_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(rhsp), rhsp);
                    ଫେରନ୍ତୁ nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        ଫେରନ୍ତୁ ନୂତନ AstAssignW{flp, lhsp, rhsp};
    }

    ଶୂନ୍ୟ visit(AstStable* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ସତ୍ୟ);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstSteady* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ସତ୍ୟ);
            nodep->dtypeSetBit();
        }
    }

    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    ଶୂନ୍ୟ visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    ଶୂନ୍ୟ visit(AstUntil* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            ଯଦି (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            ଯଦି (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    ଶୂନ୍ୟ visit(AstRand* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            ଯଦି (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } ଅନ୍ୟଥା {
                nodep->dtypeSetInt();  // Says the spec
            }
            ଯଦି (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    ଶୂନ୍ୟ visit(AstSEventually* nodep) override {
        ଯଦି (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstSGotoRep* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            ଯଦି (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } ଅନ୍ୟଥା {
                width_Check_Rep_ଗଣନା(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstSNonConsRep* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            ଯଦି (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } ଅନ୍ୟଥା {
                width_Check_Rep_ଗଣନା(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    ଶୂନ୍ୟ width_Check_Rep_ଗଣନା(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->countp(), Const);
        ଯଦି (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } ଅନ୍ୟଥା ଯଦି (constp->dtypep()->isSigned() && constp->ଅଙ୍କ().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } ଅନ୍ୟଥା ଯଦି (constp->ଅଙ୍କ().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } ଅନ୍ୟଥା ଯଦି (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    ଶୂନ୍ୟ widthCheckGotoRepRange(T_Rep* nodep, ଅଚଳ ଅକ୍ଷର* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ଅଚଳ AstConst* ଅଚଳ minConstp = VN_CAST(nodep->countp(), Const);
        ଅଚଳ AstConst* ଅଚଳ maxConstp = VN_CAST(nodep->maxCountp(), Const);
        ଯଦି (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            ଫେରନ୍ତୁ;
        }
        ଯଦି (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            ଫେରନ୍ତୁ;
        }
        ଯଦି (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            ଫେରନ୍ତୁ;
        }
        ଯଦି (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    ଶୂନ୍ୟ visit(AstSThroughout* nodep) override {
        m_hasSExpr = ସତ୍ୟ;
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = ମିଥ୍ୟା;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = ସତ୍ୟ;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ସତ୍ୟ;
        m_hasSExpr = ସତ୍ୟ;
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            ଯଦି (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ସତ୍ୟ;
        m_hasSExpr = ସତ୍ୟ;
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            ପାଇଁ (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstURandomRange* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    ଶୂନ୍ୟ visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        ଯଦି (VN_IS(nodep->backp(), IsUnbounded)) ଫେରନ୍ତୁ;  // Ok, leave
        ଯଦି (VN_IS(nodep->backp(), BracketArrayDType)) ଫେରନ୍ତୁ;  // Ok, leave
        ଯଦି (VN_IS(nodep->backp(), InsideRange)) ଫେରନ୍ତୁ;  // Ok, leave
        ଯଦି (ଅଚଳ auto* ଅଚଳ varp = VN_CAST(nodep->backp(), Var)) {
            ଯଦି (varp->isParam()) ଫେରନ୍ତୁ;  // Ok, leave
        }
        Ast_ଗଣ୍ଠି* backp = nodep->backp();
        ଯଦି (VN_IS(backp, Sub)) backp = backp->backp();
        ଯଦି (ଅଚଳ auto* ଅଚଳ selp = VN_CAST(backp, SelExtract)) {
            ଯଦି (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ଫେରନ୍ତୁ;
        }
        ଯଦି (ଅଚଳ auto* ଅଚଳ selp = VN_CAST(backp, SelBit)) {
            ଯଦି (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ଫେରନ୍ତୁ;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    ଶୂନ୍ୟ visit(AstInferredDisable* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) nodep->dtypeSetBit();
    }
    ଶୂନ୍ୟ visit(AstIsUnbounded* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        ଯଦି (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        ଯଦି (m_vup->final()) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            ଯଦି (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    ଶୂନ୍ୟ visit(AstCLog2* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    ଶୂନ୍ୟ widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = nodep->valuep();
        ଅଚଳ std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const)) {
            ଯଦି (!constp->ଅଙ୍କ().isNegative()) ଫେରନ୍ତୁ;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } ଅନ୍ୟଥା ଯଦି (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ;
        }
        // Continue with the default weight
        valuep->replaceWith(ନୂତନ AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    ଶୂନ୍ୟ widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    ଶୂନ୍ୟ visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ଅଚଳ cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        ଯଦି (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ଫେରନ୍ତୁ;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        ଯଦି (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            ଫେରନ୍ତୁ;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        ଯଦି (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            ଯଦି (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } ଅନ୍ୟଥା {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = nodep->valuep();
            ଯଦି (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(ନୂତନ AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
            }
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->valuep(), Const);
            ଯଦି (!constp || !constp->ଅଙ୍କ().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstCoverOption* nodep) override {
        ଯଦି (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            ଫେରନ୍ତୁ;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        ଯଦି (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        ଯଦି (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        ଯଦି (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    ଶୂନ୍ୟ widthCovergroupRanges(Ast_ଗଣ୍ଠି* rangesp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ଅଚଳ auto fill = [&](Ast_ଗଣ୍ଠି* nodep) {
            ଯଦି (!fillWidth) ଫେରନ୍ତୁ;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = VN_AS(nodep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ଯଦି (AstInsideRange* ଅଚଳ rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } ଅନ୍ୟଥା ଯଦି (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } ଅନ୍ୟଥା {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    ଶୂନ୍ୟ visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    ଶୂନ୍ୟ visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        ଯଦି (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        ଯଦି (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    ଶୂନ୍ୟ visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        ଯଦି (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        ଯଦି (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    ଶୂନ୍ୟ visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ countp = nodep->countp()) {
            ଯଦି (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } ଅନ୍ୟଥା {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    ଶୂନ୍ୟ widthCoverFilter(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filterp, ଅଚଳ ଅକ୍ଷର* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        File_ଧାଡ଼ି* ଅଚଳ fl = filterp->fileline();
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ resultp = filterp->dtypep()->skipRefp();
        ଯଦି (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                ନୂତନ AstNeqD{fl, filterp, ନୂତନ AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } ଅନ୍ୟଥା ଯଦି (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(ନୂତନ AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(filterp), filterp);
        } ଅନ୍ୟଥା {
            fixWidthReduce(filterp);
        }
    }
    ଶୂନ୍ୟ visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    ଶୂନ୍ୟ visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    ଶୂନ୍ୟ visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ଯଦି (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                ଫେରନ୍ତୁ;
            }
            checkCvtUS(nodep->lhsp(), ମିଥ୍ୟା);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ଯଦି (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        ଯଦି (m_vup->final()) {
            Ast_ଗଣ୍ଠି_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_ଗଣ୍ଠି* newp = nullptr;  // No change
            ଯଦି (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = ନୂତନ AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } ଅନ୍ୟଥା ଯଦି (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = ନୂତନ AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } ଅନ୍ୟଥା ଯଦି (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = ନୂତନ AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            ଯଦି (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        }
    }
    ଶୂନ୍ୟ visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣନା_Bits* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin = V3_ସଂଖ୍ୟା::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣନା_Ones* nodep) override {
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin = V3_ସଂଖ୍ୟା::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    ଶୂନ୍ୟ visit(AstCvtPackString* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dt = nodep->lhsp()->dtypep()) {
            ଯଦି (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    ଶୂନ୍ୟ visit(AstCvtPackedToArray* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    ଶୂନ୍ୟ visit(AstCvtArrayToArray* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    ଶୂନ୍ୟ visit(AstCvtArrayToPacked* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    ଶୂନ୍ୟ visit(AstCvtUnpackToQueue* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    ଶୂନ୍ୟ visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        ଯଦି (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    ଶୂନ୍ୟ visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ଅଚଳ ଦ୍ୱିଗୁଣ timePrescale = constp->ଅଙ୍କ().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ଅଚଳ ଦ୍ୱିଗୁଣ time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstEventControl* nodep) override {
        ଯଦି (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->fileline()->timingOn()) {
            ଯଦି (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } ଅନ୍ୟଥା {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        ଯଦି (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    ସ୍ଥିର string typenameExtends(ଅଚଳ AstClass* classp) {
        // An interface class may extend several, so those are not named
        ଯଦି (!classp || classp->isInterfaceClass()) ଫେରନ୍ତୁ "";
        ପାଇଁ (ଅଚଳ AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ଯଦି (extendsp->isImplements()) ଜାରି;
            ଅଚଳ AstClass* ଅଚଳ basep = extendsp->classOrNullp();
            ଫେରନ୍ତୁ basep ? " extends class " + basep->dtypeName(ସତ୍ୟ) : "";
        }
        ଫେରନ୍ତୁ "";
    }
    ଶୂନ୍ୟ visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        ଯଦି (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        ବାଛ (nodep->attrType()) {
        ସ୍ଥିତି VAttrType::DIM_DIMENSIONS:
        ସ୍ଥିତି VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ଅଚଳ std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(ସତ୍ୟ);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(ନୂତନ AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି VAttrType::DIM_BITS_OR_ସଂଖ୍ୟା: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            ଯଦି (VN_IS(nodep->fromp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା {
                Ast_ଗଣ୍ଠି* newp = ନୂତନ AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            ଫେରନ୍ତୁ;
        }
        ସ୍ଥିତି VAttrType::DIM_BITS:
        ସ୍ଥିତି VAttrType::DIM_ଉପର:
        ସ୍ଥିତି VAttrType::DIM_INCREMENT:
        ସ୍ଥିତି VAttrType::DIM_ବାମ:
        ସ୍ଥିତି VAttrType::DIM_ତଳ:
        ସ୍ଥିତି VAttrType::DIM_ଡାହାଣ:
        ସ୍ଥିତି VAttrType::DIM_SIZE: {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            ଯଦି (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownModp = v3Global.rootp()->containingModule(dtypep);
                ଯଦି (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                ବାଛ (nodep->attrType()) {
                ସ୍ଥିତି VAttrType::DIM_SIZE: {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp
                        = ନୂତନ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(ସତ୍ୟ);
                    newp->protect(ମିଥ୍ୟା);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                    ଭାଙ୍ଗ;
                }
                ସ୍ଥିତି VAttrType::DIM_ବାମ:
                ସ୍ଥିତି VAttrType::DIM_ତଳ: {
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଭାଙ୍ଗ;
                }
                ସ୍ଥିତି VAttrType::DIM_ଡାହାଣ:
                ସ୍ଥିତି VAttrType::DIM_ଉପର: {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sizep
                        = ନୂତନ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(ସତ୍ୟ);
                    sizep->protect(ମିଥ୍ୟା);
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp
                        = ନୂତନ AstSub{nodep->fileline(), sizep,
                                     ନୂତନ AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                    ଭାଙ୍ଗ;
                }
                ସ୍ଥିତି VAttrType::DIM_INCREMENT: {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
                        = ନୂତନ AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଭାଙ୍ଗ;
                }
                ସ୍ଥିତି VAttrType::DIM_BITS: {
                    ଯଦି (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } ଅନ୍ୟଥା {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    ଭାଙ୍ଗ;
                }
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } ଅନ୍ୟଥା {
                ଅଚଳ std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(ସତ୍ୟ);
                ଅଚଳ uint32_t msbdim = dimpair.first + dimpair.second;
                ଯଦି (!nodep->dimp() || msbdim < 1) {
                    ଯଦି (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                        Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstShiftL{
                            nodep->fileline(), ନୂତନ AstLenN{nodep->fileline(), fromp},
                            ନୂତନ AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    } ଅନ୍ୟଥା {
                        Ast_ଗଣ୍ଠି_DType* ଅଚଳ baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା dim = 1;
                        AstConst* ଅଚଳ newp = dimension_ମୂଲ୍ୟ(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    }
                } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->dimp(), Const)) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* ଅଚଳ newp
                        = dimension_ମୂଲ୍ୟ(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                } ଅନ୍ୟଥା {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ଅଚଳ tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ dimp = nodep->dimp()->unlinkFrBack();
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                }
            }
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ protoDtp = nodep->fromp()->dtypep();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ declDtp = [&]() {
                ଯଦି (m_ftaskp->fvarp()) ଫେରନ୍ତୁ m_ftaskp->fvarp()->dtypep();
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ voidp = ନୂତନ AstVoidDType{m_ftaskp->fileline()};
                ଠେଲ_Deletep(voidp);  // Note voidp used past here
                ଫେରନ୍ତୁ voidp;
            }();
            ଯଦି (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(ସତ୍ୟ);
            ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି VAttrType::TYPEID:
            ଯଦି (Ast_ଗଣ୍ଠି_DType* dtypep = VN_CAST(nodep->fromp(), ଗଣ୍ଠି_DType)) {
                nodep->dtypep(dtypep);
            } ଅନ୍ୟଥା {
                nodep->dtypep(VN_AS(nodep->fromp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)->dtypep());
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            ଭାଙ୍ଗ;
        }
        }
    }
    ଶୂନ୍ୟ visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    ଶୂନ୍ୟ visit(AstCellArrayRef* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(ସତ୍ୟ);
    }

    ଶୂନ୍ୟ visit(AstCellRef* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;

        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cellExprp = VN_CAST(nodep->cellp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = VN_CAST(nodep->exprp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(ସତ୍ୟ);
    }

    // DTYPES
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Array_DType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed

        ଯଦି (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ଅଚଳ basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            ଯଦି (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* ଅଚଳ newp = ନୂତନ AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        ଯଦି (VN_IS(nodep, PackArrayDType)) m_underPackedArray = ସତ୍ୟ;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        ଯଦି (AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            ଯଦି (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(ସତ୍ୟ);
        } ଅନ୍ୟଥା {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDTypep = nodep->subDTypep()->skipRefp();
            ଯଦି (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            ଯଦି (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* baseDTypep = nodep->subDTypep();
                ଯେତେବେଳେ (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                ଅଚଳ AstBasicDType* ଅଚଳ basicp = nodep->basicp();
                ଯଦି (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(ସତ୍ୟ));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ଶୂନ୍ୟ visit(AstAssocArrayDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->ଚାବି_DTypep(iterateEditMoveDTypep(nodep, nodep->ଚାବି_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ଶୂନ୍ୟ visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_ଗଣ୍ଠି* ଅଚଳ elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି* newp;
        ଯଦି (VN_IS(elementsp, Unbounded)) {
            newp = ନୂତନ AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_ବୃକ୍ଷ(), elementsp);
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ keyp = VN_CAST(elementsp, ଗଣ୍ଠି_DType)) {
            newp = ନୂତନ AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } ଅନ୍ୟଥା {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_ଧାଡ଼ି* ଅଚଳ elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, ସତ୍ୟ);
            // Must be expression that is constant, but we'll determine that later
            newp = ନୂତନ AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                ନୂତନ AstRange{nodep->fileline(), ନୂତନ AstConst(elementsp->fileline(), 0),
                             ନୂତନ AstSub{elementsNewFl, VN_AS(elementsp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି),
                                        ନୂତନ AstConst(elementsNewFl, 1)},
                             ସତ୍ୟ}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    ଶୂନ୍ୟ visit(AstConstraintRefDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    ଶୂନ୍ୟ visit(AstDynArrayDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ଶୂନ୍ୟ visit(AstQueueDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        ଯଦି (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_ବୃକ୍ଷ();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ଶୂନ୍ୟ visit(AstVoidDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    ଶୂନ୍ୟ visit(AstUnsizedArrayDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        ଯଦି (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ଶୂନ୍ୟ visit(AstWildcardArrayDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ଶୂନ୍ୟ visit(AstBasicDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        ଯଦି (nodep->generic()) ଫେରନ୍ତୁ;  // Already perfect
        ଯଦି (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } ଅନ୍ୟଥା ଯଦି (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } ଅନ୍ୟଥା ଯଦି (nodep->implicit()) {
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
    ଶୂନ୍ୟ visit(AstConstDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(ସତ୍ୟ);
        ଯଦି (nodep->typeofp()) {  // type(typeofp_expression)
            ଯଦି (Ast_ଗଣ୍ଠି_DType* typeofDtp = VN_CAST(nodep->typeofp(), ଗଣ୍ଠି_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } ଅନ୍ୟଥା {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_ଗଣ୍ଠି* ଅଚଳ typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_ବୃକ୍ଷ(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        ଯଦି (nodep->subDTypep()) {
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
        ଯଦି (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ଅଚଳ ତାର୍କିକ inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            ଯଦି (inTemplateModule) {
                nodep->doingWidth(ମିଥ୍ୟା);
                ଫେରନ୍ତୁ;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(ମିଥ୍ୟା);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    ଶୂନ୍ୟ widthDTypeNameParams(ଅଚଳ AstTypedef* nodep) {
        ଅଚଳ VDTypeNameScopes* ଅଚଳ scopesp = VDTypeNameScopes::currentp();
        ଯଦି (!scopesp) ଫେରନ୍ତୁ;  // Computing parameters, as by V3Param, so naming as found
        Ast_ଗଣ୍ଠି* scopep = nullptr;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            ଯଦି (ଅଚଳ std::vector<Ast_ଗଣ୍ଠି*>* ଅଚଳ paramsp = scopesp->paramsp(scopep)) {
                ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ paramp : *paramsp) {
                    ଯଦି (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstTypedef* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        ଯଦି (auto* ଅଚଳ refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ଅଚଳ ତାର୍କିକ declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ଯଦି (declares) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->dtypep();
            ଯଦି (Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep = VN_CAST(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } ଅନ୍ୟଥା ଯଦି (AstEnumDType* ଅଚଳ edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    ଶୂନ୍ୟ visit(AstParamTypeDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        ଯଦି (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    ଶୂନ୍ୟ visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = VN_CAST(nodep->lhsp(), ଗଣ୍ଠି_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } ଅନ୍ୟଥା {
            ଯଦି (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(ନୂତନ AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ toDtp = nodep->ଶୀର୍ଷ()->dtypep()->skipRefToEnump();
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଅଚଳ auto castable = Ast_ଗଣ୍ଠି::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_ଗଣ୍ଠି* newp;
        ଯଦି (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ଅଚଳ enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ testp = enumTestValid(nodep->fromp(), enumDtp);
            File_ଧାଡ଼ି* ଅଚଳ fl_novalue = ନୂତନ File_ଧାଡ଼ି{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, ସତ୍ୟ);
            newp = ନୂତନ AstCond{
                fl, testp,
                ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{fl,
                                ନୂତନ AstAssign{fl_novalue, nodep->ଶୀର୍ଷ()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                ନୂତନ AstConst{fl, AstConst::Signed32{}, 1}},
                ନୂତନ AstConst{fl, AstConst::Signed32{}, 0}};
        } ଅନ୍ୟଥା ଯଦି ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{
                fl,
                ନୂତନ AstAssign{fl, nodep->ଶୀର୍ଷ()->unlinkFrBack(),
                              ନୂତନ AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                ନୂତନ AstConst{fl, AstConst::Signed32{}, 1}};
        } ଅନ୍ୟଥା ଯଦି (castable == VCastable::INCOMPATIBLE) {
            newp = ନୂତନ AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } ଅନ୍ୟଥା {
            newp = ନୂତନ AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    ଶୂନ୍ୟ visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        ଯଦି (AstConst* ଅଚଳ constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_ଗଣ୍ଠି* ଅଚଳ newp
                = ନୂତନ AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ refp = VN_CAST(nodep->dtp(), ଗଣ୍ଠି_DType)) {
            refp->unlinkFrBack();
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ଅନ୍ୟଥା {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstCast* nodep) override {
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ଯଦି (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            ଯଦି (AstSigned* ଅଚଳ fromp = VN_CAST(nodep->fromp(), Signed)) {
                ଯଦି (VN_IS(fromp->lhsp(), ଗଣ୍ଠି_Stream)) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_ବୃକ୍ଷ(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            ଅଚଳ auto castable = Ast_ଗଣ୍ଠି::computeCastable(toDtp, fromDtp, nodep->fromp());
            ତାର୍କିକ bad = ମିଥ୍ୟା;
            ଯଦି (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } ଅନ୍ୟଥା ଯଦି (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = ସତ୍ୟ;
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
            ଯଦି (bad) {
            } ଅନ୍ୟଥା ଯଦି (AstBasicDType* ଅଚଳ basicp = toDtp->basicp()) {
                ଯଦି (!basicp->isString() && fromDtp->isString()) {
                    newp = ନୂତନ AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } ଅନ୍ୟଥା ଯଦି (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ origDTypep = nodep->dtypep();
                    ଯଦି (!VN_IS(fromDtp, StreamDType)) {
                        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } ଅନ୍ୟଥା {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 ମିଥ୍ୟା);
                }
                ଯଦି (newp) {
                } ଅନ୍ୟଥା ଯଦି (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    ଯଦି (nodep->fromp()->isSigned()) {
                        newp = ନୂତନ AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } ଅନ୍ୟଥା {
                        newp = ନୂତନ AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } ଅନ୍ୟଥା ଯଦି (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = ନୂତନ AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } ଅନ୍ୟଥା ଯଦି (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = ନୂତନ AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } ଅନ୍ୟଥା ଯଦି (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = ନୂତନ AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } ଅନ୍ୟଥା {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } ଅନ୍ୟଥା ଯଦି (VN_IS(toDtp, QueueDType)) {
                ଯଦି (VN_IS(fromDtp, BasicDType)) {
                    newp = ନୂତନ AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } ଅନ୍ୟଥା ଯଦି (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcElementBits = 1;
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstElementBits = 1;
                    ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ଅଚଳ AstQueueDType* ଅଚଳ dstQueueDtp = VN_AS(toDtp, QueueDType);
                    ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = ନୂତନ AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  ମିଥ୍ୟା,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } ଅନ୍ୟଥା ଯଦି (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            ଯଦି (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        ଯଦି (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, ମିଥ୍ୟା, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(ସତ୍ୟ);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(ସତ୍ୟ);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->rhsp()->toSInt();
            ଯଦି (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        ଯଦି (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_ଗଣ୍ଠି* ଅଚଳ underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    ଶୂନ୍ୟ visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    ଶୂନ୍ୟ castSized(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* underp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        ଅଚଳ AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        ଯଦି (!underDtp) underDtp = underp->dtypep()->basicp();
        ଯଦି (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା calcWidth = std::max(width, underDtp->width());
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         ମିଥ୍ୟା, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି), outDtp, EXTEND_EXP,
                            ମିଥ୍ୟା);
            VL_DANGLING(underp);
        }
    }
    ଶୂନ୍ୟ visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(Ast_Constraint_ପଦସମଷ୍ଟି* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        ଯଦି (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ଠେଲ_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(ସତ୍ୟ);
            ଫେରନ୍ତୁ;
        }
        nodep->doingWidth(ସତ୍ୟ);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        ଯଦି (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.ଠେଲ_back(nodep);
        ଯଦି (nodep->attrsp()) {
            nodep->attrsp()->foreach([ଏହି, nodep](AstAttrOf* attrp) {
                ଯଦି (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    ଯଦି (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), ଗଣ୍ଠି_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ଠେଲ_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        ଯଦି (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_ଗଣ୍ଠି_DType* dtp = nodep->dtypep();
            Ast_ଗଣ୍ଠି_DType* np = nullptr;
            ଯେତେବେଳେ (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                ଯଦି (ଅଚଳ AstDynArrayDType* ଅଚଳ unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    ଯଦି (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } ଅନ୍ୟଥା {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ଅଚଳ newp
                        = ନୂତନ AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    ଯଦି (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } ଅନ୍ୟଥା {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } ଅନ୍ୟଥା {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        ଯଦି (AstWildcardArrayDType* ଅଚଳ wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        ଯଦି (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(ସତ୍ୟ);
        // Parameters if implicit untyped inherit from what they are assigned to
        ଅଚଳ AstBasicDType* ଅଚଳ bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        ତାର୍କିକ didchk = ମିଥ୍ୟା;
        ଅଚଳ ତାର୍କିକ implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        ଯଦି (implicitParam) {
            ଯଦି (nodep->valuep()) {
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
                ଯଦି (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } ଅନ୍ୟଥା ଯଦି (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } ଅନ୍ୟଥା {
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = 0;
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ ମୂଲ୍ୟ_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(ମୂଲ୍ୟ_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ଅଚଳ AstBasicDType* ଅଚଳ ମୂଲ୍ୟ_Bdtypep = ମୂଲ୍ୟ_DTypep->basicp();
                    ତାର୍କିକ issigned = ମିଥ୍ୟା;
                    ଯଦି (bdtypep->isNosign()) {
                        ଯଦି (ମୂଲ୍ୟ_Bdtypep && ମୂଲ୍ୟ_Bdtypep->isSigned()) issigned = ସତ୍ୟ;
                    } ଅନ୍ୟଥା {
                        issigned = bdtypep->isSigned();
                    }
                    ଯଦି (ମୂଲ୍ୟ_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        ଯଦି (ଅଚଳ auto* ଅଚଳ constp = VN_CAST(nodep->valuep(), Const)) {
                            ଯଦି (constp->ଅଙ୍କ().isString()) {
                                width = constp->ଅଙ୍କ().toString().length() * 8;
                            }
                        }
                        ଯଦି (width < 8) width = 8;
                    } ଅନ୍ୟଥା ଯଦି (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } ଅନ୍ୟଥା {
                        ଯଦି (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ଅଚଳ ତାର୍କିକ is_ମୂଲ୍ୟ_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    ଯଦି (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_ମୂଲ୍ୟ_Fourstate));
                    } ଅନ୍ୟଥା {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_ମୂଲ୍ୟ_Fourstate);
                    }
                    didchk = ସତ୍ୟ;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } ଅନ୍ୟଥା {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } ଅନ୍ୟଥା ଯଦି (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        ଯଦି (nodep->isNet()) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            ଯଦି (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        ଯଦି (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(ସତ୍ୟ);
        nodep->doingWidth(ମିଥ୍ୟା);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        ଯଦି (!nodep->varp()) {
            ଯଦି (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, ମିଥ୍ୟା,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        ଯଦି (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        ଯଦି (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            ଫେରନ୍ତୁ;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        ଯଦି (VN_IS(nodep->backp(), ଗଣ୍ଠି_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        ଯଦି (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(ସତ୍ୟ);
        ଯଦି (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } ଅନ୍ୟଥା ଯଦି (nodep->access().isWriteOrRW() && nodep->varp()->is_ନିବେଶ()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ଅଚଳ ତାର୍କିକ hierRef = !m_curModVars.ଗଣନା_2(nodep->varp());
            ଅଚଳ ତାର୍କିକ netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            ଅଚଳ ତାର୍କିକ contAssign = VN_IS(nodep->abovep(), AssignW);
            ଅଚଳ ତାର୍କିକ pinConn = VN_IS(nodep->abovep(), Pin);
            ଯଦି (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        ଯଦି (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(ସତ୍ୟ);
    }

    ଶୂନ୍ୟ visit(AstEnumDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_ଗଣ୍ଠି_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        ଯଦି (badDtp) {
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
        V3_ସଂଖ୍ୟା ଅଙ୍କ(nodep, nodep->width(), 0);
        ଅଚଳ V3_ସଂଖ୍ୟା one{nodep, nodep->width(), 1};
        ତାର୍କିକ wrapAround = ମିଥ୍ୟା;
        std::map<ଅଚଳ V3_ସଂଖ୍ୟା, AstEnumItem*> inits;
        ପାଇଁ (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ଯଦି (itemp->valuep()) {
                ଯଦି (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_ବୃକ୍ଷ("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                ଯଦି (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                    ଜାରି;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = ମିଥ୍ୟା;  // Explicit value resets wrap-around tracking
            }
            ଯଦି (!itemp->valuep()) {
                ଯଦି (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                ଯଦି (ଅଙ୍କ.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(ନୂତନ AstConst{itemp->fileline(), ଅଙ୍କ});
            }

            ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(itemp->valuep(), Const);
            ଯଦି (constp->ଅଙ୍କ().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            ଅଙ୍କ.opAssign(constp->ଅଙ୍କ());
            // Look for duplicates
            ଅଚଳ auto pair = inits.emplace(ଅଙ୍କ, itemp);
            ଯଦି (!pair.second) {  // IEEE says illegal
                ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ଅଚଳ ତାର୍କିକ prevNeg = constp->ଅଙ୍କ().isNegative();
            ଅଙ୍କ.opAdd(one, constp->ଅଙ୍କ());
            ଯଦି (basicp->isSigned()) {
                wrapAround = !prevNeg && ଅଙ୍କ.isNegative();
            } ଅନ୍ୟଥା {
                wrapAround = ଅଙ୍କ.isEqZero();
            }
        }
    }
    ଶୂନ୍ୟ visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        ଯଦି (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ତାର୍କିକ warnOn = ସତ୍ୟ;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = nodep->valuep();
            ଯଦି (ଅଚଳ AstAdd* ଅଚଳ anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                ଯଦି (anodep->fileline()->equal_First_ଧାଡ଼ି_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const)) {
                ଯଦି (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(constp->ଅଙ୍କ().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = ମିଥ୍ୟା;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(ନୂତନ AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    ଶୂନ୍ୟ visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        ଯଦି (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_ଗଣ୍ଠି* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            ପାଇଁ (; enump; enump = enump->backp()) {
                ଯଦି (VN_IS(enump, EnumDType)) ଭାଙ୍ଗ;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cons* nodep) override {}  // Widthed when created
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cons_Item* nodep) override {}  // Widthed when created
    ଶୂନ୍ୟ visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ଗଣ୍ଠି_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        ଯଦି (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } ଅନ୍ୟଥା ଯଦି (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } ଅନ୍ୟଥା {
            // Take width as maximum across all items
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->exprp()->width();
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା mwidth = nodep->exprp()->widthMin();
            ତାର୍କିକ isFourstate = nodep->exprp()->dtypep()->isFourstate();
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            ଯଦି (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } ଅନ୍ୟଥା {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        ଯଦି (m_constraintp) {
            ତାର୍କିକ canLower = ସତ୍ୟ;
            ପାଇଁ (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                ଯଦି (VN_IS(ditemp->rangep(), InsideRange)) ଜାରି;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                ଯଦି (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = ମିଥ୍ୟା;
                    ଭାଙ୍ଗ;
                }
            }
            ଯଦି (canLower) ଫେରନ୍ତୁ;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
        ପାଇଁ (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* inewp
                = insideItem(nodep, nodep->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), itemp->rangep());
            ଯଦି (!inewp) ଜାରି;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cmpp
                = ନୂତନ AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            ନୂତନ AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, ସତ୍ୟ);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, ସତ୍ୟ);
            inewp = ନୂତନ AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? ନୂତନ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ଯଦି (!newp) newp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ଗଣ୍ଠି_DType* expDTypep = nullptr;

        ଯଦି (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } ଅନ୍ୟଥା ଯଦି (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } ଅନ୍ୟଥା {
            // Take width as maximum across all items
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->exprp()->width();
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା mwidth = nodep->exprp()->widthMin();
            ତାର୍କିକ isFourstate = nodep->exprp()->dtypep()->isFourstate();
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            ଅଚଳ VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            ଯଦି (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } ଅନ୍ୟଥା ଯଦି (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp;
        Ast_ପଦସମଷ୍ଟି_Stmt* ପଦସମଷ୍ଟି_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        ଯଦି (!m_constraintp && !nodep->exprp()->isPure()) {
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ ପଦସମଷ୍ଟି_Dtp = nodep->exprp()->dtypep();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = ପଦସମଷ୍ଟି_Dtp->width();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ tempDTypep
                = ପଦସମଷ୍ଟି_Dtp->widthSized() ? ପଦସମଷ୍ଟି_Dtp
                                        : nodep->findBitOrLogicDType(w, w, ପଦସମଷ୍ଟି_Dtp->numeric(),
                                                                     ପଦସମଷ୍ଟି_Dtp->isFourstate());
            AstVar* ଅଚଳ varp
                = ନୂତନ AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = ନୂତନ AstVarRef{fl, varp, VAccess::READ};
            ପଦସମଷ୍ଟି_Stmtp = ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{fl,
                                        ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            ଯଦି (m_ftaskp) {
                varp->funcLocal(ସତ୍ୟ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ଅନ୍ୟଥା {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(ପଦସମଷ୍ଟି_Stmtp);
        } ଅନ୍ୟଥା {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);  // Will be unlinking
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* inewp;
            ଯଦି (ପଦସମଷ୍ଟି_Stmtp) {
                inewp = insideItem(nodep, ପଦସମଷ୍ଟି_Stmtp, itemp);
                ପଦସମଷ୍ଟି_Stmtp = nullptr;
            } ଅନ୍ୟଥା {
                inewp = insideItem(nodep, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), itemp);
            }
            ଯଦି (!inewp) ଜାରି;
            newp = newp ? ନୂତନ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ଯଦି (ପଦସମଷ୍ଟି_Stmtp) VL_DO_DANGLING(ପଦସମଷ୍ଟି_Stmtp->delete_ବୃକ୍ଷ(), ପଦସମଷ୍ଟି_Stmtp);
        ଯଦି (!newp) newp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* insideItem(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itemp) {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ itemDtp = itemp->dtypep()->skipRefp();
        ଯଦି (AstInsideRange* ଅଚଳ irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            ଫେରନ୍ତୁ irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } ଅନ୍ୟଥା ଯଦି (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ inewp = ନୂତନ AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(ସତ୍ୟ);
            ଫେରନ୍ତୁ inewp;
        } ଅନ୍ୟଥା ଯଦି (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_ବୃକ୍ଷ(), exprp);
            ଫେରନ୍ତୁ nullptr;
        }
        ଫେରନ୍ତୁ AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    ଶୂନ୍ୟ visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        ଯଦି (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } ଅନ୍ୟଥା {
            ଯଦି (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            ଯଦି (m_vup->final()) {
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା expWidth = expDTypep->width();
                ଅଚଳ ତାର୍କିକ waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                ଅଚଳ ତାର୍କିକ waiveRhs = expWidth == 32
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

    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        ଯଦି (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) override {
        ଯଦି (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(ମିଥ୍ୟା);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(ସତ୍ୟ);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        ଯଦି (ଅଚଳ AstUnionDType* ଅଚଳ unionp = VN_CAST(nodep, UnionDType)) {
            ଯଦି (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(ମିଥ୍ୟା);
        // Error checks
        ପାଇଁ (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = itemp->subDTypep()->skipRefp();
            ଯଦି (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            ଯଦି ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ଠେଲ_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        ଅଚଳ ତାର୍କିକ isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ଅଚଳ ତାର୍କିକ inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        ଯଦି (VN_IS(nodep, UnionDType) || nodep->packed()) {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = 0;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = 0;
            ତାର୍କିକ first = ସତ୍ୟ;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            ପାଇଁ (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            ପାଇଁ (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                ଯଦି (itemp->isFourstate()) nodep->isFourstate(ସତ୍ୟ);
                itemp->lsb(lsb);
                ଯଦି (VN_IS(nodep, UnionDType)) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    ଯଦି (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } ଅନ୍ୟଥା {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = ମିଥ୍ୟା;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } ଅନ୍ୟଥା {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(ମିଥ୍ୟା);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    ଶୂନ୍ୟ visit(AstThisRef* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        ଯଦି (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    ଶୂନ୍ୟ visit(AstClassRefDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        userIterateChildren(nodep, nullptr);
    }
    ଶୂନ୍ୟ visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstClassExtends* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        ଯଦି (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    ଶୂନ୍ୟ visit(AstMemberDType* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        ଯଦି (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = ସତ୍ୟ;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        ଅଚଳ AstMemberSel* ଅଚଳ fromSel = VN_CAST(nodep->fromp(), MemberSel);
        ଯଦି (AstClocking* ଅଚଳ clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            ଯଦି (AstVar* ଅଚଳ varp = memberSelClocking(nodep, clockingp)) {
                ଯଦି (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_ବୃକ୍ଷ(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                ଯଦି (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                ଯଦି (AstIfaceRefDType* ଅଚଳ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(ସତ୍ୟ);  // Must not visit again: will confuse scopes
                ଫେରନ୍ତୁ;
            }
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ adtypep = VN_CAST(fromDtp, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ଯଦି (memberSelStruct(nodep, adtypep)) ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (AstClassRefDType* ଅଚଳ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            ଯଦି (memberSelClass(nodep, adtypep)) ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (AstIfaceRefDType* ଅଚଳ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            ଯଦି (Ast_ଗଣ୍ଠି* foundp = memberSelIface(nodep, adtypep)) {
                ଯଦି (AstClocking* ଅଚଳ clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(foundp, Var)) {
                    ଯଦି (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* ଅଚଳ ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(nodep->fromp(), VarRef);
                    ଯଦି (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ଅଚଳ newp = ନୂତନ AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(ସତ୍ୟ);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                        ଫେରନ୍ତୁ;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(ସତ୍ୟ);
                    ଫେରନ୍ତୁ;
                }
                ଯଦି (AstModport* ଅଚଳ modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ଅଚଳ newDtypep = ନୂତନ AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
                ଯଦି (AstCell* ଅଚଳ cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    ଯଦି (VN_IS(cellp->modp(), Iface)) {
                        ଅଚଳ string viftopName = cellp->name() + "__Viftop";
                        Ast_ଗଣ୍ଠି_Module* ଅଚଳ parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            ଯଦି (AstVar* ଅଚଳ vp = VN_CAST(itemp, Var)) {
                                ଯଦି (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    ଭାଙ୍ଗ;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        ଯଦି (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_ଗଣ୍ଠି_DType* subDtypep = viftopVarp->dtypep();
                        ଯଦି (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            ଯଦି (AstIfaceRefDType* ଅଚଳ subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ଅଚଳ newDtypep = ନୂତନ AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(ସତ୍ୟ);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(ସତ୍ୟ);
                        ଫେରନ୍ତୁ;
                    }
                }
                ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp = VN_CAST(foundp, ଗଣ୍ଠି_FTask)) {
                    AstMethodCall* ଅଚଳ newp = ନୂତନ AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ଫେରନ୍ତୁ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ତାର୍କିକ memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        ଯଦି (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* ଅଚଳ newp = ନୂତନ AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            visit(newp);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ଅଚଳ first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        ପାଇଁ (AstClass* classp = first_classp; classp;) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ foundp = m_memberMap.findMember(classp, nodep->name())) {
                ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(foundp, Var)) {
                    ଯଦି (!varp->didWidth()) userIterate(varp, nullptr);
                    ଯଦି (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ଅଚଳ varRefp
                            = ନୂତନ AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                    ଯଦି (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(ସତ୍ୟ);
                    ଯଦି (nodep->fromp()->same_ବୃକ୍ଷ(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
                ଯଦି (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(ନୂତନ AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
                ଯଦି (AstEnumItemRef* ଅଚଳ adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
                ଯଦି (Ast_ଗଣ୍ଠି_FTask* ftaskp = VN_CAST(foundp, ଗଣ୍ଠି_FTask)) {
                    AstMethodCall* newp = ନୂତନ AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
                ଯଦି (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
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
        ପାଇଁ (AstClass* classp = first_classp; classp;) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                ଯଦି (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.ଠେଲ_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        ଅଚଳ string suggest = speller.best_Candidate_ସନ୍ଦେଶ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Caller handles error
    }
    ତାର୍କିକ modportAllowsItem(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ AstIfaceRefDType* adtypep, ଅଚଳ Ast_ଗଣ୍ଠି* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ଅଚଳ AstModport* ଅଚଳ modportp = adtypep->modportp();
        ଯଦି (!modportp) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(itemp, Var)) {
            ଯଦି (varp->isParam()) ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (!VN_IS(itemp, ଗଣ୍ଠି_FTask) && !VN_IS(itemp, Clocking)) {
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        ଯଦି (pair.second) {
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.ଭର୍ତ୍ତି(mpItemp->name());
            }
        }
        ଯଦି (pair.first->second.ଗଣନା_2(itemp->name())) ଫେରନ୍ତୁ ସତ୍ୟ;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    Ast_ଗଣ୍ଠି* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (itemp->name() == nodep->name()) {
                ଯଦି (!modportAllowsItem(nodep, adtypep, itemp)) {
                    ଫେରନ୍ତୁ nullptr;  // Caller handles error
                }
                ଫେରନ୍ତୁ itemp;
            }
            ଯଦି (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ଠେଲ_Candidate(itemp->prettyName());
            }
        }
        ଅଚଳ string suggest = speller.best_Candidate_ସନ୍ଦେଶ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ଫେରନ୍ତୁ nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                ଯଦି (citemp->varp()->name() == nodep->name()) ଫେରନ୍ତୁ citemp->varp();
                speller.ଠେଲ_Candidate(citemp->varp()->prettyName());
            }
        }
        ଅଚଳ string suggest = speller.best_Candidate_ସନ୍ଦେଶ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ଫେରନ୍ତୁ nullptr;  // Caller handles error
    }
    ତାର୍କିକ memberSelStruct(AstMemberSel* nodep, Ast_ଗଣ୍ଠି_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        ଯଦି (AstMemberDType* ଅଚଳ memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            ଯଦି (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } ଅନ୍ୟଥା ଯଦି (adtypep->packed()) {
                AstSel* ଅଚଳ newp = ନୂତନ AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(ସତ୍ୟ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } ଅନ୍ୟଥା {
                AstStructSel* ଅଚଳ newp = ନୂତନ AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(ସତ୍ୟ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    ଶୂନ୍ୟ visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* ଅଚଳ basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        ଯଦି (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } ଅନ୍ୟଥା ଯଦି (AstEnumDType* ଅଚଳ adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } ଅନ୍ୟଥା ଯଦି (AstAssocArrayDType* ଅଚଳ adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } ଅନ୍ୟଥା ଯଦି (AstWildcardArrayDType* ଅଚଳ adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } ଅନ୍ୟଥା ଯଦି (AstDynArrayDType* ଅଚଳ adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } ଅନ୍ୟଥା ଯଦି (AstQueueDType* ଅଚଳ adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } ଅନ୍ୟଥା ଯଦି (AstClassRefDType* ଅଚଳ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } ଅନ୍ୟଥା ଯଦି (AstIfaceRefDType* ଅଚଳ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } ଅନ୍ୟଥା ଯଦି (AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } ଅନ୍ୟଥା ଯଦି (AstConstraintRefDType* ଅଚଳ adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } ଅନ୍ୟଥା ଯଦି (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } ଅନ୍ୟଥା ଯଦି (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } ଅନ୍ୟଥା {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_ଗଣ୍ଠି_FTask_Ref* nodep, ତାର୍କିକ required, ତାର୍କିକ arbReturn,
                              Ast_ଗଣ୍ଠି_DType* returnDtp, Ast_ଗଣ୍ଠି_DType* indexDtp,
                              Ast_ଗଣ୍ଠି_DType* ମୂଲ୍ୟ_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        ଯଦି (AstWith* ଅଚଳ withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->ମୂଲ୍ୟ_Arg_Refp()->dtypep(ମୂଲ୍ୟ_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            ଫେରନ୍ତୁ withp;
        }
        ଯଦି (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ methodOkArguments(Ast_ଗଣ୍ଠି_FTask_Ref* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା minArg, ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxArg) {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା narg = 0;
        ଯଦି (AstWith* ଅଚଳ withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ଠେଲ_Deletep(withp->unlinkFrBack()), withp);
        }
        ପାଇଁ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ଅଚଳ ତାର୍କିକ ok = (narg >= minArg) && (narg <= maxArg);
        ଯଦି (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            ପାଇଁ (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    ନୂତନ AstArg{nodep->fileline(), "",
                               ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            ପାଇଁ (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                ଯେତେବେଳେ (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_ବୃକ୍ଷ(), argp);
            }
        }
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* methodArg(AstMethodCall* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା arg) {
        AstArg* argp = nodep->argsp();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        ଫେରନ୍ତୁ argp->exprp();
    }

    ଶୂନ୍ୟ methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        ଯଦି (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            ଯଦି (nodep->name() == "num") {
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବସ୍ତୁଗୁଡ଼ିକ = 0;
                ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++ବସ୍ତୁଗୁଡ଼ିକ;
                newp = ନୂତନ AstConst(nodep->fileline(), AstConst::Signed32{}, ବସ୍ତୁଗୁଡ଼ିକ);
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "first") {
                ଅଚଳ AstEnumItem* itemp = adtypep->itemsp();
                ଯଦି (!itemp) {
                    newp = ନୂତନ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ଅନ୍ୟଥା {
                    newp = VN_AS(itemp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "last") {
                ଅଚଳ AstEnumItem* itemp = adtypep->itemsp();
                ଯେତେବେଳେ (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                ଯଦି (!itemp) {
                    newp = ନୂତନ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ଅନ୍ୟଥା {
                    newp = VN_AS(itemp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            ଯଦି (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Bad case");
            }

            ଯଦି (nodep->name() != "name" && nodep->argsp()) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, ସତ୍ୟ);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                ଯଦି (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ଅଚଳ uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                ଯଦି (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* ଅଚଳ newp = ନୂତନ AstMethodCall{
                        nodep->fileline(),
                        ନୂତନ AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                    ଫେରନ୍ତୁ;
                } ଅନ୍ୟଥା ଯଦି (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ଠେଲ_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* ଅଚଳ clonep = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    VN_AS(stepp, Const)->ଅଙ୍କ().setLong(1);
                    AstConst* ଅଚଳ constp = ନୂତନ AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* ଅଚଳ argp = ନୂତନ AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* ଅଚଳ newp
                        = ନୂତନ AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    ଶୂନ୍ୟ methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ଯଦି (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ index_exprp = method_Call_Wildcard_Index_ପଦସମଷ୍ଟି(nodep, adtypep);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            ଯଦି (!nodep->argsp()) {
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } ଅନ୍ୟଥା {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ index_exprp = method_Call_Wildcard_Index_ପଦସମଷ୍ଟି(nodep, adtypep);
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ଅଚଳ withp
                = methodWithClause(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ଅଚଳ withp
                = methodWithClause(nodep, ସତ୍ୟ, ମିଥ୍ୟା, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "map") {
            AstWith* ଅଚଳ withp
                = methodWithClause(nodep, ସତ୍ୟ, ମିଥ୍ୟା, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ଯଦି (newp) {
            newp->protect(ମିଥ୍ୟା);
            newp->didWidth(ସତ୍ୟ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        }
    }
    ଶୂନ୍ୟ methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ଯଦି (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        ଯଦି (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->ଚାବି_DTypep(), BOTH);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ index_exprp = method_Call_Assoc_Index_ପଦସମଷ୍ଟି(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->ଚାବି_DTypep(), BOTH);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ index_exprp = method_Call_Assoc_Index_ପଦସମଷ୍ଟି(nodep, adtypep);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            ଯଦି (!nodep->argsp()) {
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } ଅନ୍ୟଥା {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->ଚାବି_DTypep(),
                                  BOTH);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ index_exprp = method_Call_Assoc_Index_ପଦସମଷ୍ଟି(nodep, adtypep);
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ଅଚଳ withp = methodWithClause(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, adtypep->subDTypep(),
                                                    adtypep->ଚାବି_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ଅଚଳ withp = methodWithClause(
                nodep, ମିଥ୍ୟା, ସତ୍ୟ, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            ଯଦି (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->ଚାବି_DTypep()));
            } ଅନ୍ୟଥା {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ଅଚଳ withp = methodWithClause(nodep, ସତ୍ୟ, ମିଥ୍ୟା, nodep->findBitDType(),
                                                    adtypep->ଚାବି_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ଅଚଳ withp = methodWithClause(nodep, ସତ୍ୟ, ମିଥ୍ୟା, nodep->findBitDType(),
                                                    adtypep->ଚାବି_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->ଚାବି_DTypep()));
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "map") {
            AstWith* ଅଚଳ withp = methodWithClause(nodep, ସତ୍ୟ, ମିଥ୍ୟା, adtypep->subDTypep(),
                                                    adtypep->ଚାବି_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ଯଦି (newp) {
            newp->protect(ମିଥ୍ୟା);
            newp->didWidth(ସତ୍ୟ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        }
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* method_Call_Assoc_Index_ପଦସମଷ୍ଟି(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_ଗଣ୍ଠି* ଅଚଳ index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->ଚାବି_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        ଫେରନ୍ତୁ nodep->argsp()->exprp();
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* method_Call_Wildcard_Index_ପଦସମଷ୍ଟି(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_ଗଣ୍ଠି* ଅଚଳ index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        ଫେରନ୍ତୁ nodep->argsp()->exprp();
    }
    ଶୂନ୍ୟ methodCallLValueRecurse(AstMethodCall* nodep, Ast_ଗଣ୍ଠି* childp, ଅଚଳ VAccess& access) {
        ଯଦି (AstCMethodHard* ଅଚଳ ichildp = VN_CAST(childp, CMethodHard)) {
            ତାର୍କିକ methAt = ମିଥ୍ୟା;
            ତାର୍କିକ methAtBack = ମିଥ୍ୟା;
            ତାର୍କିକ methAtWrite = ମିଥ୍ୟା;
            ବାଛ (ichildp->method()) {
            ସ୍ଥିତି VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = ସତ୍ୟ;
                ଭାଙ୍ଗ;
            ସ୍ଥିତି VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = ସତ୍ୟ;
                ଭାଙ୍ଗ;
            ସ୍ଥିତି VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            ସ୍ଥିତି VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            ସ୍ଥିତି VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = ସତ୍ୟ;
                ଭାଙ୍ଗ;
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଭାଙ୍ଗ;
            }
            ଯଦି (methAt || methAtBack || methAtWrite) {
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                ଯଦି (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    ଯଦି (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } ଅନ୍ୟଥା ଯଦି (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                ଫେରନ୍ତୁ;
            }
        }
        ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(childp, ଗଣ୍ଠି_Var_Ref)) {
            varrefp->access(access);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstStructSel* ଅଚଳ ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Sel* ଅଚଳ ichildp = VN_CAST(childp, ଗଣ୍ଠି_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } ଅନ୍ୟଥା {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_ଗଣ୍ଠି_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        ଯଦି (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            ଯଦି (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, ମିଥ୍ୟା, ସତ୍ୟ, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ଅଚଳ withp = methodWithClause(
                nodep, ମିଥ୍ୟା, ସତ୍ୟ, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            ଯଦି (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } ଅନ୍ୟଥା {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ଅଚଳ withp
                = methodWithClause(nodep, ସତ୍ୟ, ମିଥ୍ୟା, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ଅଚଳ withp
                = methodWithClause(nodep, ସତ୍ୟ, ମିଥ୍ୟା, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ଅଚଳ withp
                = methodWithClause(nodep, ସତ୍ୟ, ମିଥ୍ୟା, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        ଫେରନ୍ତୁ newp;
    }
    ଶୂନ୍ୟ methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ଯଦି (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ଅଚଳ withp
                = methodWithClause(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି ((newp = methodCallArray(nodep, adtypep))) {
        } ଅନ୍ୟଥା {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ଯଦି (newp) {
            newp->protect(ମିଥ୍ୟା);
            newp->didWidth(ସତ୍ୟ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        }
    }
    ଶୂନ୍ୟ methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ଯଦି (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            ଯଦି (!nodep->argsp()) {
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } ଅନ୍ୟଥା {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ index_exprp = method_Call_Queue_Index_ପଦସମଷ୍ଟି(nodep);
                ଯଦି (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_କାଢ଼_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } ଅନ୍ୟଥା {
                    newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ index_exprp = method_Call_Queue_Index_ପଦସମଷ୍ଟି(nodep);
            AstArg* ଅଚଳ argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            ଯଦି (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ଠେଲ_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } ଅନ୍ୟଥା {
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ଭର୍ତ୍ତି, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ଅଚଳ argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ଅଚଳ withp
                = methodWithClause(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି ((newp = methodCallArray(nodep, adtypep))) {
        } ଅନ୍ୟଥା {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ଯଦି (newp) {
            newp->protect(ମିଥ୍ୟା);
            newp->didWidth(ସତ୍ୟ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        }
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* method_Call_Queue_Index_ପଦସମଷ୍ଟି(AstMethodCall* nodep) {
        Ast_ଗଣ୍ଠି* ଅଚଳ index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        ଫେରନ୍ତୁ nodep->argsp()->exprp();
    }
    ଶୂନ୍ୟ methodCallWarnTiming(Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ nodep, ଅଚଳ std::string& className) {
        ଯଦି (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } ଅନ୍ୟଥା ଯଦି (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    ଶୂନ୍ୟ methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ଅଚଳ ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), ଗଣ୍ଠି_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            ଯଦି (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                ଫେରନ୍ତୁ;
            }
            ଯଦି (adtypep->isVirtual()) {
                ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    ଯଦି (AstVar* ଅଚଳ mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            ଯଦି (ftaskp->isStatic()) {
                AstArg* ଅଚଳ argsp = nodep->argsp();
                ଯଦି (argsp) argsp->unlinkFrBackWithNext();
                Ast_ଗଣ୍ଠି_FTask_Ref* newp = nullptr;
                ଯଦି (VN_IS(ftaskp, Task)) {
                    newp = ନୂତନ AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } ଅନ୍ୟଥା {
                    newp = ନୂତନ AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            } ଅନ୍ୟଥା {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                ଯଦି (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            ଫେରନ୍ତୁ;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    ଶୂନ୍ୟ handleRandomizeArgs(Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ nodep, AstClass* ଅଚଳ classp) {
        ତାର୍କିକ hasNonNullArgs = ମିଥ୍ୟା;
        AstConst* nullp = nullptr;
        ପାଇଁ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = argp->exprp();
            ଯଦି (AstConst* ଅଚଳ constp = VN_CAST(exprp, Const)) {
                ଯଦି (constp->ଅଙ୍କ().isNull()) {
                    nullp = constp;
                    ଜାରି;
                }
            }
            hasNonNullArgs = ସତ୍ୟ;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            ଯଦି (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp = methodCallp->fromp();
                ଯେତେବେଳେ (AstMemberSel* ଅଚଳ memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ଅଚଳ varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            ଯଦି (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_ବୃକ୍ଷ(), argp);
                ଜାରି;
            }
            ଯେତେବେଳେ (exprp) {
                ଯଦି (AstMemberSel* ଅଚଳ memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } ଅନ୍ୟଥା {
                    ଯଦି (AstVarRef* ଅଚଳ varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } ଅନ୍ୟଥା {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_ବୃକ୍ଷ(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                ଯଦି (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            ଯଦି (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } ଅନ୍ୟଥା ଯଦି (classp->existsMember([&](ଅଚଳ AstClass*, ଅଚଳ AstVar* memberVarp) {
                           ଫେରନ୍ତୁ memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } ଅନ୍ୟଥା {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_ବୃକ୍ଷ(), argp);
            }
        }
        ଯଦି (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    ଶୂନ୍ୟ methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ଅଚଳ first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        ଯଦି (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            ପାଇଁ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                ଯଦି (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        ପାଇଁ (AstClass* classp = first_classp; classp;) {
            ଯଦି (nodep->fileline()->timingOn()) {
                ଯଦି (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ଅଚଳ packagep = getItemPackage(classp);
                    // Check if it's std
                    ଯଦି (packagep && packagep->name() == "std") {
                        ଯଦି (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } ଅନ୍ୟଥା ଯଦି (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), ଗଣ୍ଠି_FTask)) {
                userIterate(ftaskp, nullptr);
                ଯଦି (ftaskp->isStatic()) {
                    AstArg* ଅଚଳ argsp = nodep->argsp();
                    ଯଦି (argsp) argsp->unlinkFrBackWithNext();
                    Ast_ଗଣ୍ଠି_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    ଯଦି (!m_vup) {  // Called as task
                        newp = ନୂତନ AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } ଅନ୍ୟଥା {
                        newp = ନୂତନ AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                } ଅନ୍ୟଥା {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    ଯଦି (VN_IS(ftaskp, Task)) {
                        ଯଦି (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    ଯଦି (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(ସତ୍ୟ);
                v3Global.useRandomizeMethods(ସତ୍ୟ);
                AstCMethodHard* ଅଚଳ newp
                    = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(ସତ୍ୟ);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(ସତ୍ୟ);
                v3Global.useRandomizeMethods(ସତ୍ୟ);
                AstCMethodHard* ଅଚଳ newp
                    = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(ସତ୍ୟ);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(ସତ୍ୟ);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            ପାଇଁ (AstClass* classp = first_classp; classp;) {
                ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    ଯଦି (VN_IS(itemp, ଗଣ୍ଠି_FTask)) speller.ଠେଲ_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            ଅଚଳ string suggest = speller.best_Candidate_ସନ୍ଦେଶ(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    ଶୂନ୍ୟ methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        ଯଦି (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            ଯଦି (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } ଅନ୍ୟଥା {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(ସତ୍ୟ);
        } ଅନ୍ୟଥା {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        ଯଦି (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } ଅନ୍ୟଥା {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        ଗଣନା : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        ଯଦି (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        ଯଦି (methodId) {
            AstWith* ଅଚଳ withp
                = methodWithClause(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            ଯଦି (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ଅଚଳ newp
                    = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                ଯଦି (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(ମିଥ୍ୟା);
                newp->didWidth(ସତ୍ୟ);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ arrayRef = nodep->fromp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ selector = ନୂତନ AstArraySel{fl, arrayRef, i};
                    ଯଦି (!newp) {
                        newp = selector;
                    } ଅନ୍ୟଥା {
                        ବାଛ (methodId) {
                        ସ୍ଥିତି ARRAY_OR: newp = ନୂତନ AstOr{fl, newp, selector}; ଭାଙ୍ଗ;
                        ସ୍ଥିତି ARRAY_AND: newp = ନୂତନ AstAnd{fl, newp, selector}; ଭାଙ୍ଗ;
                        ସ୍ଥିତି ARRAY_XOR: newp = ନୂତନ AstXor{fl, newp, selector}; ଭାଙ୍ଗ;
                        ସ୍ଥିତି ARRAY_SUM: newp = ନୂତନ AstAdd{fl, newp, selector}; ଭାଙ୍ଗ;
                        ସ୍ଥିତି ARRAY_PRODUCT: newp = ନୂତନ AstMul{fl, newp, selector}; ଭାଙ୍ଗ;
                        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            }
        } ଅନ୍ୟଥା ଯଦି (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(ମିଥ୍ୟା);
            newp->didWidth(ସତ୍ୟ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    ଶୂନ୍ୟ methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        ଯଦି (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    ଶୂନ୍ୟ methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        ଯଦି (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp
                = ନୂତନ AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp
                = ନୂତନ AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "compare" || nodep->name() == "icompare") {
            ଅଚଳ ତାର୍କିକ ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ଅଚଳ argp = nodep->argsp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhs = nodep->fromp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhs = argp->exprp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ଅଚଳ arg0p = nodep->argsp();
            AstArg* ଅଚଳ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* ଅଚଳ varrefp
                = ନୂତନ AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstAssign{
                nodep->fileline(), fromp, ନୂତନ AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ଠେଲ_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ଅଚଳ arg0p = nodep->argsp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->fromp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ଅଚଳ arg0p = nodep->argsp();
            AstArg* ଅଚଳ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->fromp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ thsp = arg1p->exprp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            ଯଦି (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp
                = ନୂତନ AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_ଗଣ୍ଠି_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        ଯଦି (AstQueueDType* ଅଚଳ queuep = m_queueDTypeIndexed[indexDTypep]) {
            ଫେରନ୍ତୁ queuep;
        } ଅନ୍ୟଥା {
            AstQueueDType* ଅଚଳ newp
                = ନୂତନ AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            ଫେରନ୍ତୁ newp;
        }
    }

    ଶୂନ୍ୟ visit(AstNew* nodep) override {
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        AstClass* classp = nullptr;
        ତାର୍କିକ assign = ମିଥ୍ୟା;
        ଯଦି (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = ସତ୍ୟ;
            Ast_ଗଣ୍ଠି* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            ଯଦି (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                ଯଦି (AstClass* ଅଚଳ clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ଅଚଳ adtypep
                        = ନୂତନ AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } ଅନ୍ୟଥା {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            ଯଦି (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, ଗଣ୍ଠି_DType)
                           ? ("data type "s + VN_AS(warnp, ଗଣ୍ଠି_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                ଫେରନ୍ତୁ;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } ଅନ୍ୟଥା {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } ଅନ୍ୟଥା {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        ଯଦି (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    ଶୂନ୍ୟ visit(AstNewCopy* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        AstClassRefDType* ଅଚଳ refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        ଯଦି (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            ଫେରନ୍ତୁ;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        ଯଦି (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    ଶୂନ୍ୟ visit(AstNewDynamic* nodep) override {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        AstDynArrayDType* ଅଚଳ adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        ଯଦି (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            ଫେରନ୍ତୁ;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        ଯଦି (!VN_IS(nodep->backp(), ଗଣ୍ଠି_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            ଫେରନ୍ତୁ;
        }
        nodep->dtypep(adtypep);
        ଯଦି (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        ଯଦି (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ଅଚଳ consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            ଯଦି (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_Tagged_ପଦସମଷ୍ଟି* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    ଶୂନ୍ୟ visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    ଶୂନ୍ୟ visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    ଶୂନ୍ୟ visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    ଶୂନ୍ୟ visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    ଶୂନ୍ୟ visit(AstPattern* nodep) override {
        ଯଦି (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            ପାଇଁ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = ମିଥ୍ୟା;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                ଯଦି (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), ଗଣ୍ଠି_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            ଫେରନ୍ତୁ;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = ମିଥ୍ୟା;
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        UINFO(9, "PATTERN " << nodep);
        ଯଦି (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        ଯଦି (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_ଗଣ୍ଠି_DType* dtypep = nodep->dtypep();
        ଯଦି (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            ଯଦି (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                ଫେରନ୍ତୁ;
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            ଯଦି (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                ଫେରନ୍ତୁ;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            ପାଇଁ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା times = visitPatMemberRep(patp);
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 1; i < times; i++) {
                    AstPatMember* ଅଚଳ newp = patp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            ପାଇଁ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ଯଦି (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    ଯେତେବେଳେ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ movep = VN_AS(patp->lhssp()->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_ଗଣ୍ଠି* newkeyp = nullptr;
                        ଯଦି (patp->keyp()) newkeyp = patp->keyp()->clone_ବୃକ୍ଷ(ସତ୍ୟ);
                        AstPatMember* ଅଚଳ newp
                            = ନୂତନ AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            ପାଇଁ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ଅଚଳ nextp = VN_AS(patp->nextp(), PatMember);
                ଯଦି (patp->isDefault()) {
                    ଯଦି (defaultp) {
                        VL_DO_DANGLING(ଠେଲ_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            ଯେତେବେଳେ (ଅଚଳ AstConstDType* ଅଚଳ vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            ଯଦି (auto* ଅଚଳ vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } ଅନ୍ୟଥା ଯଦି (auto* ଅଚଳ vdtypep = VN_CAST(dtypep, ଗଣ୍ଠି_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ଅଚଳ consp = ନୂତନ AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ଅଚଳ consp = ନୂତନ AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } ଅନ୍ୟଥା {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଯଦି (defaultp) VL_DO_DANGLING(ଠେଲ_Deletep(defaultp), defaultp);
        }
    }
    ଶୂନ୍ୟ patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<ଅଚଳ AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ଅଚଳ AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            ଯେତେବେଳେ (patp) {
                କର {
                    ଯଦି (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        ଯଦି (ଅଚଳ AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            ଯଦି (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                ଭାଙ୍ଗ;
                            } ଅନ୍ୟଥା {
                                ଅଚଳ std::pair<PatMap::iterator, ତାର୍କିକ> ret
                                    = patmap.emplace(memp, patp);
                                ଯଦି (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_DType* nodedtypep = VN_CAST(patp->keyp(), ଗଣ୍ଠି_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            ଅଚଳ string dtype = nodedtypep->dtypep()->prettyDTypeName(ସତ୍ୟ);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } ଅନ୍ୟଥା {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } ଅନ୍ୟଥା ଯଦି (memp) {
                        // constant expr
                        ଅଚଳ std::pair<PatMap::iterator, ତାର୍କିକ> ret = patmap.emplace(memp, patp);
                        ଯଦି (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } ଅନ୍ୟଥା {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } ଯେତେବେଳେ (ମିଥ୍ୟା);

                // Next
                ଯଦି (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
        ଯଦି (vdtypep->packed()) {
            ପାଇଁ (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ଅଚଳ auto it = patmap.find(memp);
                ଯଦି (it == patmap.end()) {  // default or default_type assignment
                    ଯଦି (AstStructDType* ଅଚଳ memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } ଅନ୍ୟଥା {
                        AstPatMember* ଅଚଳ patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = ମୂଲ୍ୟ_Concat_pattern_Struct(patp, newp, memp, nodep);
                        ଯଦି (patp) VL_DO_DANGLING(ଠେଲ_Deletep(patp), patp);
                    }
                } ଅନ୍ୟଥା {  // member assignment
                    newp = ମୂଲ୍ୟ_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } ଅନ୍ୟଥା {  // Unpacked
            AstConsStruct* ଅଚଳ consp = ନୂତନ AstConsStruct{nodep->fileline(), vdtypep};
            ପାଇଁ (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ଅଚଳ auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                ଯଦି (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    ଠେଲ_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ଠେଲ_Deletep(patp);  // patp used below
                } ଅନ୍ୟଥା {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = pattern_Member_ମୂଲ୍ୟ_Iterate(patp);
                consp->addItemsp(ନୂତନ AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        ଯଦି (newp) {
            nodep->replaceWith(newp);
        } ଅନ୍ୟଥା {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp,
                                                 AstPattern* nodep, ଅଚଳ DTypeMap& dtypemap) {
        ପାଇଁ (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            ଯଦି (AstStructDType* ଅଚଳ memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } ଅନ୍ୟଥା {
                AstPatMember* ଅଚଳ patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = ମୂଲ୍ୟ_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                ଯଦି (patp) VL_DO_DANGLING(ଠେଲ_Deletep(patp), patp);
            }
        }
        ଫେରନ୍ତୁ newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ଅଚଳ DTypeMap& dtypemap) {
        ଅଚଳ string memp_DType = memp->virtRefDTypep()->prettyDTypeName(ସତ୍ୟ);
        ଅଚଳ auto it = dtypemap.find(memp_DType);
        ଯଦି (it != dtypemap.end()) {
            // default_value for data_type
            ଫେରନ୍ତୁ it->second->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        ଯଦି (defaultp) {
            // default_value for any unmatched member yet
            ଫେରନ୍ତୁ defaultp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        ଯଦି (memp->valuep()) {
            ଫେରନ୍ତୁ ନୂତନ AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ଗଣ୍ଠି_ପଦସମଷ୍ଟି),
                                    ନୂତନ AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        ଫେରନ୍ତୁ nullptr;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ମୂଲ୍ୟ_Concat_pattern_Struct(AstPatMember* patp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        ଯଦି (patp) {
            patp->dtypep(memp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = pattern_Member_ମୂଲ୍ୟ_Iterate(patp);
            ଯଦି (!newp) {
                newp = valuep;
            } ଅନ୍ୟଥା {
                AstConcat* ଅଚଳ concatp = ନୂତନ AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, ସତ୍ୟ);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, ସତ୍ୟ);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        ଫେରନ୍ତୁ newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_ଗଣ୍ଠି_DType* elemDTypep,
                                       ଅଚଳ DTypeMap& dtypemap = DTypeMap{}) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = defaultp->lhssp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDTypeSkipRefp = elemDTypep->skipRefp();
        ଅଚଳ AstStructDType* ଅଚଳ structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ଅଚଳ ତାର୍କିକ unpackedStruct = structp && !structp->packed();

        ଯଦି (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
            ଫେରନ୍ତୁ defaultp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        ଯଦି (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
            ଫେରନ୍ତୁ defaultp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        ଯଦି (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        ତାର୍କିକ wholeElement = unpackedStruct;
        ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ ମୂଲ୍ୟ_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !ମୂଲ୍ୟ_DTypep->skipRefp()->isIntegralOrPacked()
                                          : ମୂଲ୍ୟ_DTypep->similarDType(elemDTypeSkipRefp);
        }
        ଯଦି (wholeElement) {
            VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
            ଫେରନ୍ତୁ defaultp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }

        AstPatMember* ଅଚଳ nestedDefaultp
            = ନୂତନ AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(ସତ୍ୟ);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        ପାଇଁ (ଅଚଳ auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        AstPattern* ଅଚଳ recursivePatternp = ନୂତନ AstPattern{defaultp->fileline(), nestedDefaultp};
        ଫେରନ୍ତୁ ନୂତନ AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* patternDefaultValuep(AstPatMember* defaultp, Ast_ଗଣ୍ଠି_DType* elemDTypep) {
        ଯଦି (!defaultp) ଫେରନ୍ତୁ nullptr;
        AstPatMember* ଅଚଳ elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = pattern_Member_ମୂଲ୍ୟ_Iterate(elemDefaultp);
        VL_DO_DANGLING(ଠେଲ_Deletep(elemDefaultp), elemDefaultp);
        ଫେରନ୍ତୁ valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    ଶୂନ୍ୟ patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        ଅଚଳ VNumRange range = arrayDtp->declRange();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements = range.elements();
        AstConsArray* ଅଚଳ newp = ନୂତନ AstConsArray{nodep->fileline(), arrayDtp};
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା entn = 0;  // Number of elements given so far, from the left
        ତାର୍କିକ error = ମିଥ୍ୟା;  // Reported an error, so the value is not complete
        ପାଇଁ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = pattern_Member_ମୂଲ୍ୟ_Iterate(patp);
            ଅଚଳ uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ଯଦି (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_ମୂଲ୍ୟଗୁଡ଼ିକ(index, valuep);
                ଜାରି;
            }
            ଅଚଳ VCastable castable = Ast_ଗଣ୍ଠି::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            ଯଦି (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = ସତ୍ୟ;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        ଯଦି (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = ସତ୍ୟ;
        }
        ଯଦି (error) {
            VL_DO_DANGLING(ଠେଲ_Deletep(newp), newp);
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } ଅନ୍ୟଥା {
            nodep->replaceWith(newp);
        }
    }
    ଶୂନ୍ୟ patternArray(AstPattern* nodep, Ast_ଗଣ୍ଠି_Array_DType* arrayDtp, AstPatMember* defaultp) {
        ଯଦି (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.ବାମ() << " to " << range.ଡାହାଣ());
        Ast_ଗଣ୍ଠି* newp = nullptr;
        ତାର୍କିକ missed = ମିଥ୍ୟା;  // Some element missed, so not a complete value
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା entn = 0, ent = range.ବାମ(); entn < range.elements();
             ++entn, ent += range.ବାମ_To_ଡାହାଣ_Inc()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = nullptr;
            ଅଚଳ auto it = patmap.find(ent);
            ଯଦି (it == patmap.end()) {
                ଯଦି (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } ଅନ୍ୟଥା {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = ସତ୍ୟ;
                }
            } ଅନ୍ୟଥା {
                AstPatMember* ଅଚଳ patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_ମୂଲ୍ୟ_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ଅଚଳ AstUnpackArrayDType* ଅଚଳ memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                ଯଦି (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
                    valuep = ନୂତନ AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            ଯଦି (valuep) {
                ଯଦି (VN_IS(arrayDtp, UnpackArrayDType)) {
                    ଯଦି (!newp) newp = ନୂତନ AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } ଅନ୍ୟଥା {  // Packed. Convert to concat for now.
                    ଯଦି (!newp) {
                        newp = valuep;
                    } ଅନ୍ୟଥା {
                        AstConcat* ଅଚଳ concatp
                            = ନୂତନ AstConcat{valuep->fileline(), VN_AS(newp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        ଯଦି (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        ଯଦି (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ଠେଲ_Deletep(newp), newp);
            newp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        ଯଦି (newp) {
            nodep->replaceWith(newp);
        } ଅନ୍ୟଥା {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        ଯଦି (VN_IS(patp->keyp(), ଗଣ୍ଠି_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            ଫେରନ୍ତୁ nullptr;
        }
        ଯଦି (patp->ଚାବି_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->ଚାବି_Exprp());  // May change
        AstConst* ଅଚଳ constp = VN_CAST(patp->ଚାବି_Exprp(), Const);
        ଯଦି (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        ଫେରନ୍ତୁ constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_ଗଣ୍ଠି_DType* ଚାବି_DTypep) {
        ଯଦି (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            ଫେରନ୍ତୁ nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        ଯଦି (ଚାବି_DTypep->skipRefp()->isIntegralOrPacked() && patp->ଚାବି_Exprp()) {
            userIterate(patp->ଚାବି_Exprp(), WidthVP{ଚାବି_DTypep, PRELIM}.p());
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = std::max(patp->ଚାବି_Exprp()->width(), ଚାବି_DTypep->width());
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ contextDTypep
                = ଚାବି_DTypep->findBitOrLogicDType(width, width, ଚାବି_DTypep->numeric(), ସତ୍ୟ);
            iterateCheck(patp, "key", patp->ଚାବି_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, ମିଥ୍ୟା);
        }
        AstConst* ଅଚଳ constp = patternConstKeyp(patp);
        ଯଦି (!constp) ଫେରନ୍ତୁ nullptr;
        ଯଦି (constp->ଅଙ୍କ().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            ଫେରନ୍ତୁ nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, ଚାବି_DTypep, EXTEND_EXP, ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ convp = patp->ଚାବି_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        ଫେରନ୍ତୁ VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_ଗଣ୍ଠି_DType* arrayDtp) {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ ଚାବି_DTypep = VN_AS(arrayDtp, AssocArrayDType)->ଚାବି_DTypep();
        ଯଦି (!ଚାବି_DTypep->skipRefp()->isAggregateType() || !patp->ଚାବି_Exprp()) {
            ଫେରନ୍ତୁ patternScalarKeyp(patp, ଚାବି_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->ଚାବି_Exprp(), ଚାବି_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->ଚାବି_Exprp());
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ foldedp = patp->ଚାବି_Exprp();
        ଯଦି (!foldedp->is_Constant_ବୃକ୍ଷ()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            ଫେରନ୍ତୁ nullptr;
        }
        ଯଦି (foldedp->exists([](ଅଚଳ AstConst* constp) { ଫେରନ୍ତୁ constp->ଅଙ୍କ().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            ଫେରନ୍ତୁ nullptr;
        }
        ଫେରନ୍ତୁ foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_ଗଣ୍ଠି_DType* arrayDtp) {
        ଫେରନ୍ତୁ patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_ଗଣ୍ଠି_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        ଫେରନ୍ତୁ patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    ଶୂନ୍ୟ sort_Cons_ବସ୍ତୁଗୁଡ଼ିକ(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        ଯେତେବେଳେ (T_Item* ଅଚଳ itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.ଠେଲ_back(itemp);
        }
        ଅଚଳ auto ଚାବି_Of = [](ଅଚଳ T_Item* itemp) -> ଅଚଳ V3_ସଂଖ୍ୟା& {
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ keyp = itemp->keyp();
            ଫେରନ୍ତୁ VN_AS(keyp, Const)->ଅଙ୍କ();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](ଅଚଳ T_Item* ap, ଅଚଳ T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            ଫେରନ୍ତୁ ଚାବି_Of(ap).isLtXZ(ଚାବି_Of(bp));
        });
        ଅଚଳ T_Item* firstp = nullptr;  // First item with the current key
        ପାଇଁ (T_Item* ଅଚଳ itemp : itemps) {
            ଯଦି (firstp && ଚାବି_Of(firstp).isCaseEq(ଚାବି_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } ଅନ୍ୟଥା {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    ଶୂନ୍ୟ patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_ଗଣ୍ଠି_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        ପାଇଁ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ଅଚଳ auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = pattern_Member_ମୂଲ୍ୟ_Iterate(patp);
            ଯଦି (keyp) {
                consp->addItemsp(ନୂତନ T_Item{patp->fileline(), keyp, valuep});
            } ଅନ୍ୟଥା {
                VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ଅଚଳ AstAssocArrayDType* ଅଚଳ assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        ଯଦି (!assocDtp || !assocDtp->ଚାବି_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_ବସ୍ତୁଗୁଡ଼ିକ<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newConsDynArrayOrQueue(File_ଧାଡ଼ି* fileline, ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଯଦି (VN_IS(dtypep, DynArrayDType)) ଫେରନ୍ତୁ ନୂତନ AstConsDynArray{fileline};
        ଯଦି (VN_IS(dtypep, QueueDType)) ଫେରନ୍ତୁ ନୂତନ AstConsQueue{fileline};
        ଫେରନ୍ତୁ nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    ଶୂନ୍ୟ addConsItem(T_Cons* consp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ଅଚଳ ତାର୍କିକ isElement
            = Ast_ଗଣ୍ଠି::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ଅଚଳ AstQueueDType* ଅଚଳ subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        ଯଦି (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* ଅଚଳ subp = Ast_ଗଣ୍ଠି::cast<T_Cons>(valuep);
            ଯଦି (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                ଯଦି (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(ଠେଲ_Deletep(subp), subp);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (AstConsQueue* ଅଚଳ queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_ବସ୍ତୁଗୁଡ଼ିକ(consp, queuep);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (AstConsDynArray* ଅଚଳ dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_ବସ୍ତୁଗୁଡ଼ିକ(consp, dynp);
                ଫେରନ୍ତୁ;
            }
        }
        T_Item* ଅଚଳ itemp = ନୂତନ T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        ଯଦି (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } ଅନ୍ୟଥା {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    ଶୂନ୍ୟ addConsItem(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* consp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        ଯଦି (AstConsDynArray* ଅଚଳ dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } ଅନ୍ୟଥା {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    ଶୂନ୍ୟ add_Cons_ବସ୍ତୁଗୁଡ଼ିକ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* consp, T_Sub* subp) {
        ଯେତେବେଳେ (auto* ଅଚଳ itemp = subp->itemsp()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(subp), subp);
    }
    ଶୂନ୍ୟ patternDynArrayOrQueue(AstPattern* nodep, Ast_ଗଣ୍ଠି_DType* arrayp, AstPatMember* defaultp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ଅଚଳ AstPatMember* ଅଚଳ firstp = VN_AS(nodep->itemsp(), PatMember);
        ଅଚଳ ତାର୍କିକ keyed = defaultp || (firstp && firstp->keyp());
        ଯଦି (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        ଯଦି (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        ପାଇଁ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ଯଦି (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_ମୂଲ୍ୟ_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    ଶୂନ୍ୟ patternBasic(AstPattern* nodep, Ast_ଗଣ୍ଠି_DType* vdtypep, AstPatMember* defaultp) {
        ଅଚଳ AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ଅଚଳ VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
        ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ent : range.seqHiToLo()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = nullptr;
            ଅଚଳ auto it = patmap.find(ent);
            ଯଦି (it == patmap.end()) {
                ଯଦି (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } ଅନ୍ୟଥା {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } ଅନ୍ୟଥା {
                AstPatMember* ଅଚଳ patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_ମୂଲ୍ୟ_Iterate(patp);
            }
            ଯଦି (valuep) {  // Packed. Convert to concat for now.
                ଯଦି (!newp) {
                    newp = valuep;
                } ଅନ୍ୟଥା {
                    AstConcat* ଅଚଳ concatp = ନୂତନ AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        ଯଦି (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        ଯଦି (newp) {
            nodep->replaceWith(newp);
        } ଅନ୍ୟଥା {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pattern_Member_ମୂଲ୍ୟ_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = patp->lhssp()->unlinkFrBack();
        ଯଦି (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            ଯଦି (AstConst* ଅଚଳ newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        ଫେରନ୍ତୁ valuep;
    }

    ସ୍ଥିର ଶୂନ୍ୟ checkEventAssignment(ଅଚଳ Ast_ଗଣ୍ଠି_Assign* ଅଚଳ asgnp) {
        string unsupEvtAsgn;
        ଯଦି (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        ଯଦି (VN_IS(asgnp->rhsp(), CReset)) ଫେରନ୍ତୁ;
        ଯଦି (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        ଯଦି (!unsupEvtAsgn.empty()) {
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

    ସ୍ଥିର ତାର୍କିକ usesDynamicScheduler(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        ଯେତେବେଳେ (ସତ୍ୟ) {
            AstVarRef* ଅଚଳ vrefp = VN_CAST(nodep, VarRef);
            ଯଦି (vrefp) ଫେରନ୍ତୁ usesDynamicScheduler(vrefp);
            ଯଦି (VN_IS(nodep, MemberSel)) {
                ଫେରନ୍ତୁ ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_Sel* selp = VN_CAST(nodep, ଗଣ୍ଠି_Sel)) {
                nodep = selp->fromp();
            } ଅନ୍ୟଥା {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
    }

    ସ୍ଥିର ତାର୍କିକ usesDynamicScheduler(AstVarRef* vrefp) {
        ଫେରନ୍ତୁ VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    ଶୂନ୍ୟ visit(AstPatMember* nodep) override {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        ଯଦି (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->repp(), Const);
            ଯଦି (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } ଅନ୍ୟଥା {
                times = constp->toUInt();
            }
            ଯଦି (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_ବୃକ୍ଷ();  // Done with replicate before cloning
        }
        ଫେରନ୍ତୁ times;
    }

    ଶୂନ୍ୟ visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_ଗଣ୍ଠି* propp = nodep->propp();
            ଯେତେବେଳେ (AstLogNot* ଅଚଳ notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ଅଚଳ untilp = VN_CAST(propp, Until);
            ଯଦି (untilp && VN_IS(nodep->backp(), ଗଣ୍ଠି_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(ନୂତନ AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ଠେଲ_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            ଯଦି (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    ତାର୍କିକ firstNewStatementOkRecurse(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
            ଯଦି (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (AstAssign* ଅଚଳ aitemp = VN_CAST(nodep, Assign)) {
            ଯଦି (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (AstInitialStaticStmt* ଅଚଳ aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ଯଦି (!firstNewStatementOkRecurse(stmtp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (AstInitialAutomaticStmt* ଅଚଳ aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ଯଦି (!firstNewStatementOkRecurse(stmtp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    ଶୂନ୍ୟ handleCaseType(Ast_ଗଣ୍ଠି* casep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, CaseItem* itemsp) {
        AstAttrOf* ଅଚଳ exprap = VN_CAST(exprp, AttrOf);
        ଯଦି (!exprap) ଫେରନ୍ତୁ;
        ଯଦି (exprap->attrType() != VAttrType::TYPEID) ଫେରନ୍ତୁ;

        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ ପଦସମଷ୍ଟି_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << ପଦସମଷ୍ଟି_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_ଧାଡ଼ି* ଅଚଳ newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, ସତ୍ୟ);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, ସତ୍ୟ);  // Side effect of transform
        casep->fileline(newfl);
        ପାଇଁ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ଗଣ୍ଠି::as<CaseItem>(itemp->nextp())) {
            ଯଦି (itemp->isDefault()) ଜାରି;
            ତାର୍କିକ hit = ମିଥ୍ୟା;
            ପାଇଁ (Ast_ଗଣ୍ଠି* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ଅଚଳ AstAttrOf* ଅଚଳ condAttrp = VN_CAST(condp, AttrOf);
                ଯଦି (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } ଅନ୍ୟଥା {
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ condDtp = condAttrp->dtypep();
                    ଯଦି (Ast_ଗଣ୍ଠି::computeCastable(ପଦସମଷ୍ଟି_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = ସତ୍ୟ;
                        ଭାଙ୍ଗ;
                    }
                }
            }
            ଠେଲ_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(ନୂତନ AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(ନୂତନ AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(ଠେଲ_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    ଶୂନ୍ୟ handleCase(Ast_ଗଣ୍ଠି* casep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_ଗଣ୍ଠି_DType* subDTypep = exprp->dtypep();
        ପାଇଁ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ଗଣ୍ଠି::as<CaseItem>(itemp->nextp())) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ଯଦି (condp->dtypep() == subDTypep) ଜାରି;

                ଯଦି (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } ଅନ୍ୟଥା ଯଦି (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } ଅନ୍ୟଥା {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = std::max(subDTypep->width(), condp->width());
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    ଅଚଳ ତାର୍କିକ issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        ପାଇଁ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ଗଣ୍ଠି::as<CaseItem>(itemp->nextp())) {
            ପାଇଁ (Ast_ଗଣ୍ଠି *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    ଶୂନ୍ୟ visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        ପାଇଁ (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            ପାଇଁ (Ast_ଗଣ୍ଠି *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    ଶୂନ୍ୟ visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    ଶୂନ୍ୟ visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        ପାଇଁ (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            ପାଇଁ (Ast_ଗଣ୍ଠି *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    ଶୂନ୍ୟ visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ itemDTypep = nodep->findUInt32DType();
        ପାଇଁ (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            ପାଇଁ (Ast_ଗଣ୍ଠି *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Foreach* nodep) override {
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        nodep->didWidth(ସତ୍ୟ);
        ଅଚଳ AstForeachHeader* ଅଚଳ headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_ଗଣ୍ଠି_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        ପାଇଁ (Ast_ଗଣ୍ଠି *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            ଅଚଳ ତାର୍କିକ empty = VN_IS(argsp, Empty);
            AstVar* ଅଚଳ varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            ଯଦି (varp) varp->usedLoopIdx(ସତ୍ୟ);
            ଯଦି (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
                ଫେରନ୍ତୁ;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            ଯଦି (VN_IS(fromDtp, ଗଣ୍ଠି_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } ଅନ୍ୟଥା ଯଦି (AstBasicDType* ଅଚଳ adtypep = VN_CAST(fromDtp, BasicDType)) {
                ଯଦି (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
                    ଫେରନ୍ତୁ;
                }
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstAssocArrayDType* ଅଚଳ adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->ଚାବି_DTypep());
            } ଅନ୍ୟଥା {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
                ଫେରନ୍ତୁ;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_ଗଣ୍ଠି* ଅଚଳ bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        ଯଦି (AstForeach* ଅଚଳ loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            ଫେରନ୍ତୁ;
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
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
            ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                ଯଦି (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                    ଫେରନ୍ତୁ;
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
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            ଯଦି (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDtp = lhsDTypep->skipRefp();
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ଅଚଳ ତାର୍କିକ lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ଅଚଳ ତାର୍କିକ rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                ଯଦି (lhsIsUnpackArray && rhsIsUnpackArray) {
                    ଯଦି (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_ଗଣ୍ଠି_Var_Ref, Ast_ଗଣ୍ଠି_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), ଗଣ୍ଠି_Var_Ref), VN_CAST(nodep->rhsp(), ଗଣ୍ଠି_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                ଯଦି (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), ଗଣ୍ଠି_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        ଯଦି (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ controlp = nodep->timingControlp()) {
            ଯଦି (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ(), controlp);
            } ଅନ୍ୟଥା ଯଦି (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } ଅନ୍ୟଥା {
                ଯଦି (nodep->fileline()->timingOn()) {
                    ଯଦି (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } ଅନ୍ୟଥା {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ(), controlp);
            }
        }
        ଯଦି (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            ଯଦି (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
            AstMethodCall* ଅଚଳ newp
                = ନୂତନ AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (ଅଚଳ AstNewDynamic* ଅଚଳ dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            ଯଦି (!dynp->rhsp()) {
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } ଅନ୍ୟଥା {
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(ସତ୍ୟ);
            newp->protect(ମିଥ୍ୟା);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        ଯଦି (AstCMethodHard* ଅଚଳ slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            ଯଦି (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } ଅନ୍ୟଥା ଯଦି (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } ଅନ୍ୟଥା ଯଦି (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } ଅନ୍ୟଥା {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            ଯଦି (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ msbp = slicep->pinsp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* ଅଚଳ newp
                    = ନୂତନ AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(ସତ୍ୟ);
                newp->protect(ମିଥ୍ୟା);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    ଶୂନ୍ୟ visit(AstRSRule* nodep) override {
        ଯଦି (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    ଶୂନ୍ୟ visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    ସ୍ଥିର ତାର୍କିକ isFormatNonNumericArg(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ଫେରନ୍ତୁ dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)
                   && !VN_AS(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)->packed());
    }

    ଶୂନ୍ୟ checkFormatNumericArg(Ast_ଗଣ୍ଠି* argp, ଅକ୍ଷର ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        ଯଦି (std::tolower(ch) == 'd') ଫେରନ୍ତୁ;
        // In contrast, %x of a class reference, causes errors on several simulators.
        ଯଦି (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    ଶୂନ୍ୟ visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    ଶୂନ୍ୟ visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        ଯଦି (m_vup && !m_vup->prelim()) ଫେରନ୍ତୁ;  // Can be called as statement or function
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        ଯଦି (!nodep->ପଦସମଷ୍ଟି_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* oldExprsp = nodep->exprsp();
        ଯଦି (nodep->ପଦସମଷ୍ଟି_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        }
        ଯଦି (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        ଯେତେବେଳେ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            ଯଦି (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            ଯଦି (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                ଜାରି;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = argp->dtypep()->skipRefp();
            ଯଦି (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } ଅନ୍ୟଥା ଯଦି (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } ଅନ୍ୟଥା ଯଦି (isFormatNonNumericArg(dtypep)) {
                ଯଦି (AstClassRefDType* ଅଚଳ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ଯଦି (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(ସତ୍ୟ);
                    }
                } ଅନ୍ୟଥା {
                    // Class handles inside containers print type names, not their members.
                    Ast_ଗଣ୍ଠି_DType* ଗଣ୍ଠି_Dtypep = dtypep;
                    ଯେତେବେଳେ (ଗଣ୍ଠି_Dtypep) {
                        ଗଣ୍ଠି_Dtypep = ଗଣ୍ଠି_Dtypep->skipRefp();
                        ଯଦି (Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ uOrStructDTypep
                            = VN_CAST(ଗଣ୍ଠି_Dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(ସତ୍ୟ);
                        }
                        ଗଣ୍ଠି_Dtypep = ଗଣ୍ଠି_Dtypep->subDTypep();
                    }
                }
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } ଅନ୍ୟଥା ଯଦି (nodep->ପଦସମଷ୍ଟି_Format_2()) {
                ଯଦି (AstEnumDType* ଅଚଳ enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    ଜାରି;
                }
            }
            ଯଦି (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            ଯଦି (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } ଅନ୍ୟଥା {
                nodep->addExprsp(ନୂତନ AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    ଶୂନ୍ୟ visit(AstCReset* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    ଶୂନ୍ୟ visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    ଶୂନ୍ୟ visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    ଶୂନ୍ୟ visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        ଯଦି (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        ଯଦି (!m_paramsOnly) {
            nodep->foreach([ଏହି](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    ନୂତନ AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            ଯଦି (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            ବାଛ (nodep->displayType()) {
            ସ୍ଥିତି VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); ଭାଙ୍ଗ;
            ସ୍ଥିତି VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); ଭାଙ୍ଗ;
            ସ୍ଥିତି VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); ଭାଙ୍ଗ;
            ସ୍ଥିତି VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); ଭାଙ୍ଗ;
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ: UASSERT_OBJ(ମିଥ୍ୟା, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        ଯଦି (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstFError* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ଶୂନ୍ୟ visit(AstFEof* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    ଶୂନ୍ୟ visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        ଯଦି (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ଶୂନ୍ୟ visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ଶୂନ୍ୟ visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstFGetC* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstFGetS* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ଶୂନ୍ୟ visit(AstFUngetC* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    ଶୂନ୍ୟ visit(AstFRead* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            ଯଦି (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            ଯଦି (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    ଶୂନ୍ୟ visit(AstFScanF* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ଶୂନ୍ୟ visit(AstSScanF* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଦା_Trace_F* nodep) override { nodep->dtypeSetString(); }
    ଶୂନ୍ୟ visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstSystemF* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* subp = nullptr;
        ଯଦି (ଅଚଳ AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            ଯଦି (!adtypep->ଚାବି_DTypep()->skipRefp()->basicp()
                || !adtypep->ଚାବି_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstDynArrayDType* ଅଚଳ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } ଅନ୍ୟଥା {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        ଯଦି (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstTestPlusArgs* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    ଶୂନ୍ୟ visit(Ast_ମୂଲ୍ୟ_Plus_Args* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ଶୂନ୍ୟ visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        ଯଦି (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        ଯଦି (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        ଯଦି (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        ଯଦି (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    ଶୂନ୍ୟ visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ଶୂନ୍ୟ visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    ତାର୍କିକ pinInstArrayElement(AstPin* nodep, ଅଚଳ AstRange* rangep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା idx) {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        ଯଦି (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Types don't match, but all dimensions done, still need the usual checks
        ଯଦି (!rangep) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ଅଚଳ AstRange* ଅଚଳ subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା subElems = 1;
        ପାଇଁ (ଅଚଳ AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements = rangep->elementsConst();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା from_ବାମ = idx / subElems;

        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ଅଚଳ uint32_t conDims = conDTypep->dimensions(ମିଥ୍ୟା).second;
        ଅଚଳ uint32_t modDims = modDTypep->dimensions(ମିଥ୍ୟା).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        ଯଦି (arrp && conDims != modDims) {
            ଯଦି (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            // ArraySel index is counted from lo
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = arrp->declRange().ascending() ? from_ବାମ : elements - 1 - from_ବାମ;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = VN_AS(nodep->exprp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)->unlinkFrBack();
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ଠେଲ_Deletep(fromp), fromp);
            } ଅନ୍ୟଥା {
                AstArraySel* ଅଚଳ selp = ନୂତନ AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(ସତ୍ୟ);
                nodep->exprp(selp);
            }
            ଫେରନ୍ତୁ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        ଯଦି (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Packed connection
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା modwidth = modDTypep->width();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        ଯଦି (conwidth == modwidth) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        ଯଦି (conwidth == modwidth * elements * subElems) {
            ଯଦି (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->ବାମ_Const() << ":"
                                                     << rangep->ଡାହାଣ_Const() << "]");
            }
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = VN_AS(nodep->exprp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)->unlinkFrBack();
            AstSel* ଅଚଳ selp = ନୂତନ AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_ବାମ), selwidth};
            selp->didWidth(ସତ୍ୟ);
            nodep->exprp(selp);
            ଫେରନ୍ତୁ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        ଯଦି (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ଅଚଳ concatp = VN_CAST(nodep->exprp(), Replicate);
            ଅଚଳ AstConst* ଅଚଳ countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            ଯଦି (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* ଅଚଳ patternp = ନୂତନ AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ଠେଲ_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } ଅନ୍ୟଥା ଯଦି (!m_paramsOnly) {
            ଯଦି (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            ଯଦି (!nodep->exprp()) {  // No-connect
                ଯଦି (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                ଫେରନ୍ତୁ;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ଅଚଳ VDirection pinDirection = nodep->modVarp()->direction();
            ଯଦି (VN_IS(nodep->exprp(), ଗଣ୍ଠି_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_ଗଣ୍ଠି_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_ଗଣ୍ଠି_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ଅଚଳ AstIfaceRefDType* ଅଚଳ conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                ଯଦି (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_ଗଣ୍ଠି_DType* subDTypep = modDTypep;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା modwidth = modDTypep->width();
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା conwidth = conDTypep->width();
            ଯଦି (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } ଅନ୍ୟଥା {
                ଯଦି (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    ଯଦି (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        ଯଦି (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        ଫେରନ୍ତୁ;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                ଯଦି (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } ଅନ୍ୟଥା ଯଦି (nodep->modVarp()->isTristate()) {
                    ଯଦି (modwidth != conwidth) {
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
                } ଅନ୍ୟଥା ଯଦି (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                ଯଦି ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ ପଦସମଷ୍ଟି_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                ଯଦି (ପଦସମଷ୍ଟି_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ଅନ୍ୟଥା ଯଦି ((ପଦସମଷ୍ଟି_Arrayp && !modArrayp) || (!ପଦସମଷ୍ଟି_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (ପଦସମଷ୍ଟି_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ଅନ୍ୟଥା {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_ଗଣ୍ଠି_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), ଗଣ୍ଠି_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            ଯଦି (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        ଯଦି (!m_paramsOnly) {
            ଯଦି (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_ସନ୍ଦେଶ(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            ଯଦି (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            ଯଦି (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ଠେଲ_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstGatePin* nodep) override {
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ pinDTypep = nodep->exprp()->dtypep();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_Insts = nodep->rangep()->elementsConst();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା modwidth = ଅଙ୍କ_Insts;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା pinwidth = pinDTypep->width();
            ଯଦି (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_ଗଣ୍ଠି* ଅଚଳ newp
                    = ନୂତନ AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(ଅଙ୍କ_Insts)};
                nodep->replaceWith(newp);
            } ଅନ୍ୟଥା {
                ଯଦି (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
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
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        ଯଦି (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(ସତ୍ୟ);
            nodep->didWidth(ସତ୍ୟ);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } ଅନ୍ୟଥା ଯଦି (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        ଯଦି (nodep->classMethod() && nodep->name() == "new") {
            ଯଦି (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            ଯଦି (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_ଗଣ୍ଠି* firstp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ଯଦି (Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ sep = VN_CAST(itemp, Stmt_ପଦସମଷ୍ଟି)) {
                    ଯଦି (AstNew* ଅଚଳ newp = VN_CAST(sep->exprp(), New)) {
                        ଯଦି (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            ଭାଙ୍ଗ;
                        }
                    }
                    ଜାରି;
                }
                ଯଦି (firstNewStatementOkRecurse(itemp)) ଜାରି;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(ସତ୍ୟ);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        ଯଦି (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        ଯଦି (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } ଅନ୍ୟଥା ଯଦି (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        ଯଦି (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(ସତ୍ୟ);
        nodep->doingWidth(ମିଥ୍ୟା);
        ଯଦି (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        ଯଦି (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](ଅଚଳ AstVarRef* varrefp) {
                   ଫେରନ୍ତୁ varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                ଯଦି (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } ଅନ୍ୟଥା ଯଦି (AstAttrOf* ଅଚଳ attrp = VN_CAST(stmtp, AttrOf)) {
                ଯଦି (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                ଯଦି (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        ଯଦି (protop) {
            ପାଇଁ (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                ଯଦି (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    ଭାଙ୍ଗ;
                } ଅନ୍ୟଥା {
                    AstVar* ଅଚଳ portp = ports[i];
                    AstAttrOf* ଅଚଳ rprotop = protos[i];
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ declDtp = portp->dtypep();
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ protoDtp = rprotop->fromp()->dtypep();
                    ଯଦି (portp->name() != rprotop->name()) {
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
                    } ଅନ୍ୟଥା ଯଦି (!similarDTypeRecurse(protoDtp, declDtp)) {
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
    ଶୂନ୍ୟ visit(AstConstraint* nodep) override {
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstProperty* nodep) override {
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        ଯଦି (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(ସତ୍ୟ);
            nodep->didWidth(ସତ୍ୟ);
            ଫେରନ୍ତୁ;
        }
        nodep->doingWidth(ସତ୍ୟ);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        ପାଇଁ (Ast_ଗଣ୍ଠି* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            ଯଦି (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } ଅନ୍ୟଥା {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(ସତ୍ୟ);
        nodep->doingWidth(ମିଥ୍ୟା);
    }
    ଶୂନ୍ୟ visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    ଶୂନ୍ୟ visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        nodep->dtypeSetBit();
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(stmtp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } ଅନ୍ୟଥା {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(ସତ୍ୟ);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_ଗଣ୍ଠି* pkgItemp) {
        ଯେତେବେଳେ (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        ଫେରନ୍ତୁ VN_CAST(pkgItemp->backp(), Package);
    }
    ଶୂନ୍ୟ visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_ଗଣ୍ଠି_FTask_Ref*>(nodep));
        ଯଦି (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            ଯଦି (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } ଅନ୍ୟଥା {
            nodep->dtypeFrom(nodep->taskp());
        }
        ଯଦି (nodep->fileline()->timingOn()) {
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ classp = nodep->classOrPackagep();
            ଯଦି (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ଅଚଳ packagep = getItemPackage(classp);
                ଯଦି (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    ସ୍ଥିର ତାର୍କିକ areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ଅଚଳ std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ଅଚଳ std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        ଯଦି (dims0.size() != dims1.size()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ପାଇଁ (size_t i = 0; i < dims0.size(); ++i) {
            ଯଦି (dims0[i]->elementsConst() != dims1[i]->elementsConst()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    // Makes sure that port and pin have same size and same datatype
    ଶୂନ୍ୟ checkUnpackedArrayArgs(AstVar* portp, Ast_ଗଣ୍ଠି* pinp) {
        ଯଦି (AstUnpackArrayDType* ଅଚଳ portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            ଯଦି (AstUnpackArrayDType* ଅଚଳ pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                ଯଦି (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                ଅଚଳ AstBasicDType* ଅଚଳ portBasicp = portDtypep->basicp();
                ଅଚଳ AstBasicDType* ଅଚଳ pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ଅଚଳ ତାର୍କିକ sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                ଯଦି (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } ଅନ୍ୟଥା {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    ଶୂନ୍ୟ processFTaskRefArgs(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        କର {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ଅଚଳ V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            ଯଦି (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
                ଅଚଳ AstVar* ଅଚଳ portp = tconnect.first;
                AstArg* ଅଚଳ argp = tconnect.second;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pinp = argp->exprp();
                ଯଦି (!pinp) ଜାରି;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                ଯଦି (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argsp = nullptr;
                    ଯେତେବେଳେ (AstArg* ଅଚଳ nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_ବୃକ୍ଷ();  // Remove the call's Arg wrapper
                    }
                    string format;
                    ଯଦି (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->ଅଙ୍କ().toString();
                    } ଅନ୍ୟଥା {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ଠେଲ_Deletep(argp), argp);
                    AstSFormatF* ଅଚଳ newp
                        = ନୂତନ AstSFormatF{nodep->fileline(), format, ମିଥ୍ୟା, argsp};
                    ଯଦି (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(ନୂତନ AstScopeName{newp->fileline(), ସତ୍ୟ});
                    }
                    handle.relink(ନୂତନ AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    ଯାଅ reloop;  // so exit early; next loop will correct it
                }  //
                ଅନ୍ୟଥା ଯଦି (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                ଯଦି (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } ଯେତେବେଳେ (ମିଥ୍ୟା);
        // Stage 2
        {
            ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
                AstVar* ଅଚଳ portp = tconnect.first;
                ଅଚଳ AstArg* ଅଚଳ argp = tconnect.second;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ pinp = argp->exprp();
                ଯଦି (!pinp) ଜାରି;  // Argument error we'll find later
                // Change data types based on above accept completion
                ଯଦି (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                ଯଦି (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
                ଅଚଳ AstVar* ଅଚଳ portp = tconnect.first;
                ଅଚଳ AstArg* ଅଚଳ argp = tconnect.second;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ pinp = argp->exprp();
                ଯଦି (!pinp) ଜାରି;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        ଯଦି (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
                ଅଚଳ AstVar* ଅଚଳ portp = tconnect.first;
                ଅଚଳ AstArg* ଅଚଳ argp = tconnect.second;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pinp = argp->exprp();
                ଯଦି (!pinp) ଜାରି;  // Argument error we'll find later
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ portDTypep = portp->dtypep()->skipRefToEnump();
                ଯଦି (VN_IS(pinp, EmptyQueue)) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    ଯଦି (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = ନୂତନ AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ pinDTypep = pinp->dtypep()->skipRefToEnump();
                ଅଚଳ AstIfaceRefDType* ଅଚଳ portIfacep
                    = VN_CAST(portDTypep->elemDTypep(ସତ୍ୟ), IfaceRefDType);
                ଅଚଳ VCastable ifaceCastable
                    = portIfacep ? Ast_ଗଣ୍ଠି::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ ସତ୍ୟ)
                                 : VCastable{VCastable::UNSUPPORTED};
                ଅଚଳ ତାର୍କିକ matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                ଅଚଳ ତାର୍କିକ pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->ଅଙ୍କ().isNull();
                ଯଦି ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    ଯଦି (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } ଅନ୍ୟଥା {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } ଅନ୍ୟଥା ଯଦି (portIfacep && portIfacep->isVirtual() && portp->is_ନିବେଶ() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } ଅନ୍ୟଥା ଯଦି (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                ଯଦି (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    ଜାରି;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                ଯଦି (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } ଅନ୍ୟଥା ଯଦି (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } ଅନ୍ୟଥା {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    ଶୂନ୍ୟ handleStdRandomizeArgs(Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ nodep) {
        AstConst* nullp = nullptr;
        ପାଇଁ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = argp->exprp();
            ଯଦି (AstConst* ଅଚଳ constp = VN_CAST(exprp, Const)) {
                ଯଦି (constp->ଅଙ୍କ().isNull()) {
                    nullp = constp;
                    ଜାରି;
                }
            }
            ଯଦି (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                ଜାରି;
            }
            ଯଦି (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                ଜାରି;
            }
            ଯଦି (ଅଚଳ AstCMethodHard* ଅଚଳ methodp = VN_CAST(exprp, CMethodHard)) {
                ଯଦି (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    ଜାରି;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        ଯଦି (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        ଯଦି (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(ସତ୍ୟ);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            ଫେରନ୍ତୁ;  // Handled in V3Randomize
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ଅଚଳ classp = VN_CAST(nodep->classOrPackagep(), Class);
            ଯଦି (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(ସତ୍ୟ);
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                ପାଇଁ (ଅଚଳ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                ଯଦି (withp) nodep->withp(withp);
                nodep->didWidth(ସତ୍ୟ);
                ଫେରନ୍ତୁ;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            ଯଦି (nodep->name() == "randomize") {
                AstClassRefDType* ଅଚଳ adtypep
                    = ନୂତନ AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                ପାଇଁ (ଅଚଳ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(ସତ୍ୟ);
                v3Global.useRandomizeMethods(ସତ୍ୟ);
                AstCExpr* ଅଚଳ newp
                    = ନୂତନ AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(ସତ୍ୟ);
                v3Global.useRandomizeMethods(ସତ୍ୟ);
                AstCExpr* ଅଚଳ newp
                    = ନୂତନ AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(ମିଥ୍ୟା, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ଯଦି (nodep->didWidth()) {
            ଯଦି (withp) nodep->withp(withp);
            ଫେରନ୍ତୁ;
        }
        ଯଦି ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            ତାର୍କିକ allow = ମିଥ୍ୟା;
            ଯଦି (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                ଯଦି (ଅଚଳ AstFuncRef* ଅଚଳ funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstTaskRef* ଅଚଳ taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                ଯଦି (!allow) {
                    ଅଚଳ AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    ଯଦି (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    ଅଚଳ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ଯଦି (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            ଯଦି (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ଅଚଳ AstClass* ଅଚଳ cgClassp = m_containingClassFinder.find(nodep);
                ଯଦି (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ଅଚଳ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ଯଦି (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            ଯଦି (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        ଯଦି (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(ନୂତନ AstScopeName{nodep->fileline(), ମିଥ୍ୟା});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        ଯଦି (withp) nodep->withp(withp);
        nodep->didWidth(ସତ୍ୟ);
        // See steps that follow in visit(AstFuncRef*)
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        ଯଦି (nodep->isComboStar()) ଫେରନ୍ତୁ;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        ଯଦି (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* ଅଚଳ concatOrReplp = VN_CAST(nodep->sensp(), ଗଣ୍ଠି_Biop);
            auto* ଅଚଳ rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            ଯଦି (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* ଅଚଳ lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(ନୂତନ AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(ନୂତନ AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            ଯଦି (nodep->edgeType().anEdge()) {
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ sensDtp = nodep->sensp()->dtypep()->skipRefp();
                ଯଦି (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } ଅନ୍ୟଥା ଯଦି (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ଅଚଳ refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    ଶୂନ୍ୟ visit(AstWait* nodep) override {
        ଯଦି (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->fileline()->timingOn()) {
            ଯଦି (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(nodep->condp(), ଗଣ୍ଠି_Var_Ref)) {
                    ଯଦି (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } ଅନ୍ୟଥା {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->ମୂଲ୍ୟ_Arg_Refp(), nullptr);
        ଯଦି (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } ଅନ୍ୟଥା {
            ଯଦି (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } ଅନ୍ୟଥା {
                ଯଦି (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } ଅନ୍ୟଥା {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            ଯଦି (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } ଅନ୍ୟଥା {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    ଶୂନ୍ୟ visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        ଯଦି (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } ଅନ୍ୟଥା {
            nodep->dtypeFrom(m_withp->ମୂଲ୍ୟ_Arg_Refp());
        }
    }
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    ଶୂନ୍ୟ visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } ଅନ୍ୟଥା ଯଦି (!nodep->dtypep()) {
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
        nodep->didWidth(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visitClass(AstClass* nodep) {
        ଯଦି (nodep->didWidthAndSet()) ଫେରନ୍ତୁ;

        // If the class is std::process
        ଯଦି (nodep->name() == "process") {
            AstPackage* ଅଚଳ packagep = getItemPackage(nodep);
            ଯଦି (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                ଯଦି (AstVar* ଅଚଳ varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->getChildDTypep();
                    ଯଦି (!varp->dtypep()) {
                        VL_DO_DANGLING(ଠେଲ_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* ଅଚଳ newdtypep = ନୂତନ AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), ଗଣ୍ଠି_FTask)) {
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
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } ଅନ୍ୟଥା {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([ଏହି](ଅଚଳ AstVar* varp) { m_curModVars.ଭର୍ତ୍ତି(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    ଶୂନ୍ୟ visit_Or_Lu64(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    ଶୂନ୍ୟ visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            ଯଦି (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    ନୂତନ AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            }
        }
    }
    ଶୂନ୍ୟ visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    ଶୂନ୍ୟ visit_Os32_Lr(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    ଶୂନ୍ୟ visit_Ou64_Lr(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    ଶୂନ୍ୟ visit_log_not(AstLogNot* nodep) {
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
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            ଯଦି (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ଅଚଳ newp
                    = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        }
    }
    ଶୂନ୍ୟ visit_log_and_or(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    ଶୂନ୍ୟ visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit_red_and_or(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    ଶୂନ୍ୟ visit_red_unknown(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    ତାର୍କିକ isEquivalentDType(ଅଚଳ Ast_ଗଣ୍ଠି_DType* lhs, ଅଚଳ Ast_ଗଣ୍ଠି_DType* rhs) {
        // a) If two types match, they are equivalent.
        ଯଦି (!lhs || !rhs) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        ଯଦି (lhs == rhs) ଫେରନ୍ତୁ ସତ୍ୟ;
        // If both are basic types, check if they are the same type
        ଯଦି (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ଅଚଳ auto* lb = VN_CAST(lhs, BasicDType);
            ଅଚଳ auto* rb = VN_CAST(rhs, BasicDType);
            ଯଦି (lb->isString() != rb->isString()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ଅଚଳ ତାର୍କିକ lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ଅଚଳ ତାର୍କିକ rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        ଯଦି (lhsIsUnpackArray || rhsIsUnpackArray) {
            ଯଦି (lhsIsUnpackArray && rhsIsUnpackArray) {
                ଅଚଳ AstUnpackArrayDType* ଅଚଳ lhsp = VN_CAST(lhs, UnpackArrayDType);
                ଅଚଳ AstUnpackArrayDType* ଅଚଳ rhsp = VN_CAST(rhs, UnpackArrayDType);
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsz = lhsp->elementsConst();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rsz = rhsp->elementsConst();
                ଯଦି (lsz >= 0 && rsz >= 0 && lsz != rsz) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                ଫେରନ୍ତୁ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ଅଚଳ ତାର୍କିକ lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ଅଚଳ ତାର୍କିକ rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ଅଚଳ ତାର୍କିକ lhsIsQueue = VN_IS(lhs, QueueDType);
        ଅଚଳ ତାର୍କିକ rhsIsQueue = VN_IS(rhs, QueueDType);
        ଅଚଳ ତାର୍କିକ lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ଅଚଳ ତାର୍କିକ rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        ଯଦି (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            ଯଦି (ଅଚଳ AstDynArrayDType* ଅଚଳ lhsp = VN_CAST(lhs, DynArrayDType)) {
                ଯଦି (ଅଚଳ AstDynArrayDType* ଅଚଳ rhsp = VN_CAST(rhs, DynArrayDType)) {
                    ଫେରନ୍ତୁ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ଯଦି (ଅଚଳ AstQueueDType* ଅଚଳ lhsp = VN_CAST(lhs, QueueDType)) {
                ଯଦି (ଅଚଳ AstQueueDType* ଅଚଳ rhsp = VN_CAST(rhs, QueueDType)) {
                    ଫେରନ୍ତୁ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ଯଦି (ଅଚଳ AstAssocArrayDType* ଅଚଳ lhsp = VN_CAST(lhs, AssocArrayDType)) {
                ଯଦି (ଅଚଳ AstAssocArrayDType* ଅଚଳ rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    ଫେରନ୍ତୁ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->ଚାବି_DTypep(), rhsp->ଚାବି_DTypep());
                }
            }

            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        ଯଦି (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            ଯଦି (lhs->width() != rhs->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (lhs->isFourstate() != rhs->isFourstate()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (lhs->isSigned() != rhs->isSigned()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        ଯଦି (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ !(VN_IS(lhs, ଗଣ୍ଠି_UOr_Struct_DType) || VN_IS(rhs, ଗଣ୍ଠି_UOr_Struct_DType));
    }

    ସ୍ଥିର ତାର୍କିକ isAggregateType(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep = nodep->dtypep();
        ଯଦି (!dtypep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        dtypep = dtypep->skipRefp();
        ଯଦି (!dtypep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ dtypep->isAggregateType();
    }

    ଶୂନ୍ୟ visit_cmp_eq_gt(Ast_ଗଣ୍ଠି_Biop* nodep, ଅଚଳ ComparisonType cmpType) {
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
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ଅଚଳ ତାର୍କିକ lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            ଅଚଳ ତାର୍କିକ rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            ଯଦି (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } ଅନ୍ୟଥା ଯଦି (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } ଅନ୍ୟଥା {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            ଅଚଳ ତାର୍କିକ isAggrLhs = isAggregateType(nodep->lhsp());
            ଅଚଳ ତାର୍କିକ isAggrRhs = isAggregateType(nodep->rhsp());

            ଯଦି ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDType = nodep->lhsp()->dtypep();
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ rhsDType = nodep->rhsp()->dtypep();

                ଯଦି (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp
                        = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } ଅନ୍ୟଥା ଯଦି (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                ଯଦି (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp
                        = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
                ଯଦି (Ast_ଗଣ୍ଠି_Biop* ଅଚଳ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ଅନ୍ୟଥା ଯଦି (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                ଯଦି (Ast_ଗଣ୍ଠି_Biop* ଅଚଳ newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ଅନ୍ୟଥା {
                ଅଚଳ ତାର୍କିକ signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                ଯଦି (Ast_ଗଣ୍ଠି_Biop* ଅଚଳ newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                ତାର୍କିକ warnOn = ସତ୍ୟ;
                ଯଦି (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    ଯଦି ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = ମିଥ୍ୟା;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    ଯଦି ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = ମିଥ୍ୟା;
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
    ଶୂନ୍ୟ visit_cmp_real(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit_cmp_string(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ଶୂନ୍ୟ visit_cmp_type(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            ଅଚଳ AstAttrOf* ଅଚଳ lhsap = VN_AS(nodep->lhsp(), AttrOf);
            ଅଚଳ AstAttrOf* ଅଚଳ rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDtp = lhsap->dtypep();
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ଅଚଳ ତାର୍କିକ invert = VN_IS(nodep, NeqT);
            ଅଚଳ ତାର୍କିକ identical
                = Ast_ଗଣ୍ଠି::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ଅଚଳ ତାର୍କିକ eq = invert ^ identical;
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    ଶୂନ୍ୟ visit_negate_not(Ast_ଗଣ୍ଠି_Uniop* nodep, ତାର୍କିକ real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ଯଦି (!real_ok) checkCvtUS(nodep->lhsp(), ମିଥ୍ୟା);
        }
        ଯଦି (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            ଯଦି (Ast_ଗଣ୍ଠି_Uniop* ଅଚଳ newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                ଫେରନ୍ତୁ;
            }
        } ଅନ୍ୟଥା {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        ଯଦି (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        ଯଦି (m_vup->final()) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = expDTypep;
            // Some warning suppressions
            ତାର୍କିକ lhsWarn = ସତ୍ୟ;
            ଯଦି (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                ଯଦି (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = ମିଥ୍ୟା;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    ଶୂନ୍ୟ visit_signed_unsigned(Ast_ଗଣ୍ଠି_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ସତ୍ୟ);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->lhsp()->width();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ଶୂନ୍ୟ visit_shift(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (ଶୂନ୍ୟ)newp;  // Ununused
    }
    ଶୂନ୍ୟ iterate_shift_prelim(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ମିଥ୍ୟା);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ଯଦି (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_ଗଣ୍ଠି_Biop* iterate_shift_final(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // Nodep maybe edited
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->final()) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            ଯଦି (VN_IS(nodep, ShiftRS)) {
                ଯଦି (Ast_ଗଣ୍ଠି_Biop* ଅଚଳ newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            ତାର୍କିକ warnOn = ସତ୍ୟ;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            ଯଦି (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), ଗଣ୍ଠି_Assign)) warnOn = ମିଥ୍ୟା;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ଅଚଳ shiftp = VN_CAST(nodep->rhsp(), Const);
            ଯଦି (shiftp && !shiftp->ଅଙ୍କ().isFourState() && shiftp->width() <= 32) {
                ଅଚଳ int64_t shiftVal = shiftp->ଅଙ୍କ().toSQuad();
                ଯଦି (VN_IS(nodep, ShiftL)) {
                    ଯଦି (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = ମିଥ୍ୟା;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            ଯଦି (nodep->rhsp()->width() > 32) {
                ଯଦି (shiftp && shiftp->ଅଙ୍କ().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_ସଂଖ୍ୟା ଅଙ୍କ{shiftp, 32, shiftp->ଅଙ୍କ()};
                    Ast_ଗଣ୍ଠି* ଅଚଳ shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(ନୂତନ AstConst{shiftrhsp->fileline(), ଅଙ୍କ});
                    VL_DO_DANGLING(shiftrhsp->delete_ବୃକ୍ଷ(), shiftrhsp);
                }
            }
        }
        ଫେରନ୍ତୁ nodep;  // May edit
    }

    ଶୂନ୍ୟ visit_boolexpr_and_or(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ମିଥ୍ୟା);
            checkCvtUS(nodep->rhsp(), ମିଥ୍ୟା);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            ଅଚଳ ତାର୍କିକ expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        ଯଦି (m_vup->final()) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ଶୂନ୍ୟ visit_add_sub_replace(Ast_ଗଣ୍ଠି_Biop* nodep, ତାର୍କିକ real_ok, ଅଚଳ ତାର୍କିକ modOrDiv) {
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
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ଯଦି (!real_ok) {
                checkCvtUS(nodep->lhsp(), ମିଥ୍ୟା);
                checkCvtUS(nodep->rhsp(), ମିଥ୍ୟା);
            }
            ଯଦି (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                ଯଦି (Ast_ଗଣ୍ଠି_Biop* ଅଚଳ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                ଅଚଳ ତାର୍କିକ expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ଯଦି (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            ଯଦି (Ast_ଗଣ୍ଠି_Biop* ଅଚଳ newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            ତାର୍କିକ lhsWarn = ସତ୍ୟ;
            ତାର୍କିକ rhsWarn = ସତ୍ୟ;
            ଯଦି (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                ଯଦି (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = ମିଥ୍ୟା;
                ଯଦି (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = ମିଥ୍ୟା;
                ଯଦି (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = ମିଥ୍ୟା;  // do_increment + ...
                ଯଦି (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = ମିଥ୍ୟା;  // ... + do_increment
            } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                ଯଦି (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = ମିଥ୍ୟା;
                ଯଦି (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = ମିଥ୍ୟା;
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
    ଶୂନ୍ୟ visit_real_add_sub(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    ଶୂନ୍ୟ visit_real_neg_ceil(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_ପଦସମଷ୍ଟି(nodep);
        ଯଦି (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    ଶୂନ୍ୟ visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->ପଦସମଷ୍ଟି_Format_2(), nodep, "Assumes constant format");
        ତାର୍କିକ inPct = ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argp = nodep->exprsp();
        string newFormat;
        ପାଇଁ (ଅକ୍ଷର ch : nodep->text()) {
            ଯଦି (!inPct && ch == '%') {
                inPct = ସତ୍ୟ;
                newFormat += ch;
            } ଅନ୍ୟଥା ଯଦି (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } ଅନ୍ୟଥା ଯଦି (!inPct) {  // Normal text
                newFormat += ch;
            } ଅନ୍ୟଥା {
                inPct = ମିଥ୍ୟା;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nextp = argp ? VN_AS(argp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି) : nullptr;
                AstSFormatArg* ଅଚଳ fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subargp = fargp ? fargp->exprp() : argp;
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                ବାଛ (ch) {
                ସ୍ଥିତି '?':  // Unspecified by user, guess
                    ଯଦି (dtypep->isDouble()) {
                        ch = 'g';
                    } ଅନ୍ୟଥା ଯଦି (dtypep->isString()) {
                        ch = 's';
                    } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } ଅନ୍ୟଥା {
                        ch = 'p';
                    }
                    argp = nextp;
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି '%': ଭାଙ୍ଗ;  // %% - just output a %
                ସ୍ଥିତି 'm': ଭାଙ୍ଗ;  // %m - auto insert "name"
                ସ୍ଥିତି 'l': ଭାଙ୍ଗ;  // %m - auto insert "library"
                ସ୍ଥିତି 'd':
                    ଯଦି (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ଯଦି (subargp->isDouble()) spliceCvtS(subargp, ସତ୍ୟ, 64);
                        argp = nextp;
                    }
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 'b':  // FALLTHRU
                ସ୍ଥିତି 'o':  // FALLTHRU
                ସ୍ଥିତି 'x':
                    ଯଦି (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 't':  // Decimal/float time
                    ଯଦି (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 'e':  // FALLTHRU
                ସ୍ଥିତି 'f':  // FALLTHRU
                ସ୍ଥିତି 'g':
                    ଯଦି (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ଯଦି (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 'p':  // FALLTHRU
                ସ୍ଥିତି 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    ଯଦି (subargp && !fargp) {
                        ଯଦି (AstEnumDType* ଅଚଳ enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    ଭାଙ୍ଗ;
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // Most operators, just move to next argument
                    argp = nextp;
                    ଭାଙ୍ଗ;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    ସ୍ଥିର AstEnumDType* formatEnumDType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        ଯଦି (!enumDtp) {
            ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        ଫେରନ୍ତୁ enumDtp;
    }

    Ast_ପଦସମଷ୍ଟି_Stmt* newEnumCapture(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep, AstEnumDType* dtypep) {
        File_ଧାଡ଼ି* ଅଚଳ flp = valuep->fileline();
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(ସତ୍ୟ);
        ଯଦି (m_ftaskp) {
            varp->funcLocal(ସତ୍ୟ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ exprp = ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{
            flp, ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            ନୂତନ AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        ଫେରନ୍ତୁ exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep, AstEnumDType* dtypep) {
        ଯଦି (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* keyp;
        ଯଦି (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        } ଅନ୍ୟଥା {
            Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        }
        ଅଚଳ VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ଅଚଳ argp = ନୂତନ AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        ଫେରନ୍ତୁ argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    ତାର୍କିକ widthBad(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_DType* expDTypep) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା expWidth = expDTypep->width();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        ଯଦି (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        ଯଦି (expWidthMin == 0) expWidthMin = expWidth;
        ଯଦି (nodep->dtypep()->width() == expWidth) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ fixWidthExtend(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, Ast_ଗଣ୍ଠି_DType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(extendRule) << ") old: " << nodep);
        ଯଦି (extendRule == EXTEND_OFF) ଫେରନ୍ତୁ;
        AstConst* ଅଚଳ constp = VN_CAST(nodep, Const);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା expWidth = expDTypep->width();
        ଯଦି (constp && !constp->ଅଙ୍କ().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_ସଂଖ୍ୟା ଅଙ୍କ{nodep, expWidth, constp->ଅଙ୍କ()};
            ଅଙ୍କ.isSigned(ମିଥ୍ୟା);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), ଅଙ୍କ};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ଅନ୍ୟଥା ଯଦି (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(ସତ୍ୟ);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } ଅନ୍ୟଥା {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            ତାର୍କିକ doSigned = ମିଥ୍ୟା;
            ବାଛ (extendRule) {
            ସ୍ଥିତି EXTEND_ZERO: doSigned = ମିଥ୍ୟା; ଭାଙ୍ଗ;
            ସ୍ଥିତି EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); ଭାଙ୍ଗ;
            ସ୍ଥିତି EXTEND_LHS: doSigned = nodep->isSigned(); ଭାଙ୍ଗ;
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ: nodep->v3fatalSrc("bad case");
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
                = (doSigned ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(ସତ୍ୟ);
            linker.relink(newp);
            nodep = newp;
        }
        ଯଦି (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = spliceCvtD(nodep);
            nodep = newp;
        }
        ଯଦି (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } ଅନ୍ୟଥା {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    ଶୂନ୍ୟ fixWidthReduce(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା expWidth = 1;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା expSigned = ମିଥ୍ୟା;
        ଯଦି (nodep->width() == expWidth) ଫେରନ୍ତୁ;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ଅଚଳ constp = VN_CAST(nodep, Const);
        ଯଦି (constp) {
            V3_ସଂଖ୍ୟା ଅଙ୍କ(nodep, expWidth);
            ଅଙ୍କ.opRedOr(constp->ଅଙ୍କ());
            ଅଙ୍କ.isSigned(expSigned);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), ଅଙ୍କ};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_ବୃକ୍ଷ(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ଅନ୍ୟଥା {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypeSkipp = nodep->dtypep()->skipRefp();
            ଯଦି (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = ନୂତନ AstNeq{nodep->fileline(),
                                  ନୂତନ AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } ଅନ୍ୟଥା {
                // But, AstRedOr can be faster on wide
                newp = ନୂତନ AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    ତାର୍କିକ fixAutoExtend(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& nodepr, ପୂର୍ଣ୍ଣସଂଖ୍ୟା expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        ଯଦି (AstConst* ଅଚଳ constp = VN_CAST(nodepr, Const)) {
            ଯଦି (constp->ଅଙ୍କ().autoExtend() && !constp->ଅଙ୍କ().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_ସଂଖ୍ୟା ଅଙ୍କ(constp, expWidth);
                ଅଙ୍କ.opRepl(constp->ଅଙ୍କ(), expWidth);  // {width{'1}}
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstConst{constp->fileline(), ଅଙ୍କ};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ବୃକ୍ଷ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            ଅନ୍ୟଥା ଯଦି (!constp->ଅଙ୍କ().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->ଅଙ୍କ().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_ସଂଖ୍ୟା ଅଙ୍କ(constp, expWidth);
                ଅଙ୍କ.opExtendXZ(constp->ଅଙ୍କ(), constp->width());
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstConst{constp->fileline(), ଅଙ୍କ};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ବୃକ୍ଷ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;  // No change
    }
    ତାର୍କିକ isBaseClassRecurse(ଅଚଳ AstClass* ଅଚଳ cls1p, ଅଚଳ AstClass* ଅଚଳ cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        ଯଦି (cls1p == cls2p) ଫେରନ୍ତୁ ସତ୍ୟ;
        ପାଇଁ (ଅଚଳ AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            ଯଦି (isBaseClassRecurse(cls1p, cextp->classp())) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // Lines to end a message on nodep with the data types of its operands
    ସ୍ଥିର string warnDTypes(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_DType* lhsDtp,
                             ଅଚଳ Ast_ଗଣ୍ଠି_DType* rhsDtp, ଅଚଳ string& lhsName = "Left-hand",
                             ଅଚଳ string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        ଯଦି (lhsState == rhsState) lhsState = rhsState = "";
        ଫେରନ୍ତୁ "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    ସ୍ଥିର ତାର୍କିକ unpackedArraySizesMatch(ଅଚଳ Ast_ଗଣ୍ଠି_DType* lhsDtp, ଅଚଳ Ast_ଗଣ୍ଠି_DType* rhsDtp) {
        ଯେତେବେଳେ (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            ଯଦି (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    ଶୂନ୍ୟ checkUnpackedArrayAssignmentCompatible(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ T* ଅଚଳ lhsRefp,
                                                ଅଚଳ N* ଅଚଳ rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::ମୂଲ୍ୟ || std::is_same<T, Ast_ଗଣ୍ଠି_Var_Ref>::ମୂଲ୍ୟ)
                && (std::is_same<N, AstVar>::ମୂଲ୍ୟ || std::is_same<N, Ast_ଗଣ୍ଠି_Var_Ref>::ମୂଲ୍ୟ),
            "Unsupported types provided.");
        ଯଦି (!lhsRefp || !rhsRefp) ଫେରନ୍ତୁ;
        std::string lhsName;
        std::string rhsName;
        ଯଦି (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } ଅନ୍ୟଥା {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDtp = lhsRefp->dtypep()->skipRefp();
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ rhsDtp = rhsRefp->dtypep()->skipRefp();
        ଅଚଳ ତାର୍କିକ isLhsAggregate = lhsDtp->isAggregateType();
        ଅଚଳ ତାର୍କିକ isRhsAggregate = rhsDtp->isAggregateType();
        ଯଦି (!isLhsAggregate && !isRhsAggregate) ଫେରନ୍ତୁ;
        ଯଦି (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            ଫେରନ୍ତୁ;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(ମିଥ୍ୟା),
                                      rhsDim = rhsDtp->dimensions(ମିଥ୍ୟା);
        // Check if unpacked array dimensions are matching
        ଯଦି (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ଫେରନ୍ତୁ;
        }

        ଅଚଳ Ast_ଗଣ୍ଠି_DType* lhsDtpIterp = lhsDtp;
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* rhsDtpIterp = rhsDtp;
        ପାଇଁ (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            ଯଦି (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                ଫେରନ୍ତୁ;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        ଯଦି (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    ଶୂନ୍ୟ checkClassAssign(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* rhsp,
                          Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDTypep, ତାର୍କିକ isOutputArg = ମିଥ୍ୟା) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsRawDTypep = lhsDTypep->skipRefp();
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ rhsRawDTypep = rhsp->dtypep()->skipRefp();
        ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ଅଚଳ AstClass* ଅଚଳ dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ଅଚଳ AstClass* ଅଚଳ srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                ଯଦି (isBaseClassRecurse(dstp, srcp)) ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (rhsp->isNull()) {
                ଫେରନ୍ତୁ;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } ଅନ୍ୟଥା ଯଦି (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        ଯଦି (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                ନୂତନ AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି), lhsDTypep});
        }
    }
    ସ୍ଥିର ତାର୍କିକ similarDTypeRecurse(ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ node1p,
                                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ node2p) {
        ଫେରନ୍ତୁ node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    ଶୂନ୍ୟ iterateCheckFileDesc(Ast_ଗଣ୍ଠି* parentp, Ast_ଗଣ୍ଠି* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ଶୂନ୍ୟ)underp;  // cppcheck
    }
    ଶୂନ୍ୟ iterateCheckReal(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    ଶୂନ୍ୟ iterateCheckSigned8(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    ଶୂନ୍ୟ iterateCheckSigned32(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    ଶୂନ୍ୟ iterateCheckUInt32(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    ଶୂନ୍ୟ iterateCheckDelay(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        ଯଦି (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        ଯଦି (stage & FINAL) {
            Ast_ଗଣ୍ଠି_DType* expDTypep;
            ଯଦି (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } ଅନ୍ୟଥା {
                File_ଧାଡ଼ି* ଅଚଳ newFl = ନୂତନ File_ଧାଡ଼ି{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, ସତ୍ୟ);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, ମିଥ୍ୟା);
        }
        (ଶୂନ୍ୟ)underp;  // cppcheck
    }
    ଶୂନ୍ୟ iterateCheckString(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    ଶୂନ୍ୟ iterateCheckTyped(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp,
                           Ast_ଗଣ୍ଠି_DType* expDTypep, Stage stage) {
        ଯଦି (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        ଯଦି (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ଶୂନ୍ୟ)underp;  // cppcheck
    }
    ଶୂନ୍ୟ iterateCheckTypedSelfPrelim(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp,
                                     Ast_ଗଣ୍ଠି_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        ଯଦି (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        ଯଦି (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ଶୂନ୍ୟ)underp;  // cppcheck
    }
    ଶୂନ୍ୟ iterateCheckIntegralSelf(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ iterateCheckSelf(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Determ determ,
                          Stage stage, ତାର୍କିକ integralOnly = ମିଥ୍ୟା) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ଯଦି (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି) ? checkCvtUS(VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି), integralOnly) : underp;
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ଶୂନ୍ୟ)underp;  // cppcheck
    }
    ଶୂନ୍ୟ iterateCheckSizedSelf(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Determ determ,
                               Stage stage, ତାର୍କିକ deferUntyped = ମିଥ୍ୟା) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ଯଦି (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି) ? checkCvtUS(VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି), ମିଥ୍ୟା) : underp;
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        ଯଦି (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (ଶୂନ୍ୟ)underp;  // cppcheck
    }
    ଶୂନ୍ୟ iterateCheckAssign(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* rhsp, Stage stage,
                            Ast_ଗଣ୍ଠି_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        ଯଦି (AstConst* constp = VN_CAST(rhsp, Const)) {
            ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                ଯଦି (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    ଯଦି (basicp->width() == 8 && constp->ଅଙ୍କ().isFromString()) {
                        AstConsArray* newp = ନୂତନ AstConsArray{
                            constp->fileline(), lhsDTypep,
                            ନୂତନ AstConst{constp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 8, 0}};
                        ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା aindex : arrayp->declRange().seqLoToHi()) {
                            ପୂର୍ଣ୍ଣସଂଖ୍ୟା cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_ସଂଖ୍ୟା selected{constp, 8};
                            selected.opSel(constp->ଅଙ୍କ(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            ଯଦି (!selected.isFourState()) {
                                ଯଦି (ଅଚଳ uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 ନୂତନ AstConst{constp->fileline(),
                                                              AstConst::Widthed_ମୂଲ୍ୟ{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        ଅଚଳ ତାର୍କିକ lhsStream = (VN_IS(parentp, ଗଣ୍ଠି_Assign)
                                && VN_IS(VN_AS(parentp, ଗଣ୍ଠି_Assign)->lhsp(), ଗଣ୍ଠି_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, ସତ୍ୟ, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (ଶୂନ୍ୟ)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* convertUnpackedOrder(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ଯଦି (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) ଫେରନ୍ତୁ exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        ଫେରନ୍ତୁ newp;
    }

    ସ୍ଥିର ତାର୍କିକ isLogicalOperator(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        ଫେରନ୍ତୁ VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    ଶୂନ୍ୟ iterateCheckBool(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Stage stage) {
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
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        ଯଦି (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp
                = ନୂତନ AstNeqD{parentp->fileline(), VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି),
                              ନୂତନ AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } ଅନ୍ୟଥା ଯଦି (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp
                = ନୂତନ AstNeqN{parentp->fileline(), VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି),
                              ନୂତନ AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)), underp);  // Changed
        } ଅନ୍ୟଥା ଯଦି (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(ନୂତନ AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(underp), underp);
        } ଅନ୍ୟଥା {
            ଅଚଳ ତାର୍କିକ bad = widthBad(underp, parentp->findBitDType());
            ଯଦି (bad) {
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
            ଯଦି (Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ ପଦସମଷ୍ଟି_Stmtp = VN_CAST(underp, ପଦସମଷ୍ଟି_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(ପଦସମଷ୍ଟି_Stmtp->resultp());
                ପଦସମଷ୍ଟି_Stmtp->dtypeFrom(ପଦସମଷ୍ଟି_Stmtp->resultp());
            } ଅନ୍ୟଥା ଯଦି (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)), underp);  // Changed
            }
        }
    }

    ସ୍ଥିର ଶୂନ୍ୟ checkEnum(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* parentp, ଅଚଳ Ast_ଗଣ୍ଠି_DType* targetDTypep,
                          ଅଚଳ Ast_ଗଣ୍ଠି_DType* sourceDTypep, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sourceExprp) {
        ଯଦି (ଅଚଳ AstEnumDType* ଅଚଳ targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            ଅଚଳ VCastable castable
                = Ast_ଗଣ୍ଠି::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            ଯଦି (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    Ast_ଗଣ୍ଠି* iterateCheck(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side, Ast_ଗଣ୍ଠି* underp, Determ determ,
                          Stage stage, Ast_ଗଣ୍ଠି_DType* expDTypep, ExtendRule extendRule,
                          ତାର୍କିକ warnOn = ସତ୍ୟ, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ଅଚଳ StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        ଯଦି (rhsAccess.isReadOnly()) {
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = VN_CAST(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        ଯଦି (VN_IS(underp, ଗଣ୍ଠି_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } ଅନ୍ୟଥା ଯଦି (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } ଅନ୍ୟଥା ଯଦି (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } ଅନ୍ୟଥା ଯଦି (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_ଗଣ୍ଠି* ଅଚଳ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } ଅନ୍ୟଥା ଯଦି (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_ଗଣ୍ଠି* ଅଚଳ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି), ସତ୍ୟ, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } ଅନ୍ୟଥା ଯଦି (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_ଗଣ୍ଠି* ଅଚଳ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } ଅନ୍ୟଥା {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ underDtp = underp->dtypep()->skipRefp();
            ଯଦି (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_ଗଣ୍ଠି* ଅଚଳ newp
                    = ନୂତନ AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(underp), underp);
                underp = newp;
            }
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ expDtp = expDTypep->skipRefp();
            ଯଦି (determ == ASSIGN && VN_IS(expDtp, ଗଣ୍ଠି_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            ଯଦି (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ଅଚଳ AstBasicDType* ଅଚଳ expBasicp = expDTypep->basicp();
            ଅଚଳ AstBasicDType* ଅଚଳ underBasicp = underp->dtypep()->basicp();
            ଯଦି (expBasicp && underBasicp) {
                ଯଦି (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    ଯଦି (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
                    }
                    ଯଦି (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_ଗଣ୍ଠି_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                ଯଦି (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } ଅନ୍ୟଥା ଯଦି (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    ଯଦି (underp->isSigned() != subDTypep->isSigned()
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
                } ଅନ୍ୟଥା {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି), expDTypep, extendRule,
                                warnOn);
            } ଅନ୍ୟଥା ଯଦି (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ଅଚଳ AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                ଯଦି (VN_IS(underp, Const) && VN_AS(underp, Const)->ଅଙ୍କ().isNull()) {
                    // '= null' is ok
                } ଅନ୍ୟଥା ଯଦି (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } ଅନ୍ୟଥା ଯଦି (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } ଅନ୍ୟଥା ଯଦି (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } ଅନ୍ୟଥା {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } ଅନ୍ୟଥା {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        ଫେରନ୍ତୁ underp;
    }

    ଶୂନ୍ୟ
    widthCheckSized(Ast_ଗଣ୍ଠି* parentp, ଅଚଳ ଅକ୍ଷର* side,
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* underp,  // Node to be checked or have typecast added in front of
                    Ast_ଗଣ୍ଠି_DType* expDTypep, ExtendRule extendRule, ତାର୍କିକ warnOn = ସତ୍ୟ) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ଅଚଳ AstBasicDType* ଅଚଳ expBasicp = expDTypep->basicp();
        ଅଚଳ AstBasicDType* ଅଚଳ underBasicp = underp->dtypep()->basicp();
        ଯଦି (expDTypep == underp->dtypep()) {
            ଫେରନ୍ତୁ;  // Same type must match
        } ଅନ୍ୟଥା ଯଦି (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#ଯଦି VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା expWidth = expDTypep->width();
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା expWidthMin = expDTypep->widthMin();
            ଯଦି (expWidthMin == 0) expWidthMin = expWidth;
            ଅଚଳ ତାର୍କିକ bad = widthBad(underp, expDTypep);
            ଯଦି ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                ଫେରନ୍ତୁ;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            ଯଦି (VN_IS(parentp->backp(), CastSize)) warnOn = ମିଥ୍ୟା;
            ଯଦି (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = ମିଥ୍ୟା;
            ଯଦି (VN_IS(underp, Const) && VN_AS(underp, Const)->ଅଙ୍କ().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = ମିଥ୍ୟା;
            }
            ଯଦି ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = ମିଥ୍ୟା;
            }
            ଯଦି (bad && warnOn) {
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
            ଯଦି (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ଅଚଳ Ast_ଗଣ୍ଠି_Assign* assignp = VN_CAST(parentp, ଗଣ୍ଠି_Assign);
                ଅଚଳ AstPin* pinp = VN_CAST(parentp, Pin);
                ଯଦି (assignp && VN_IS(assignp->lhsp(), ଗଣ୍ଠି_Stream)) {
                } ଅନ୍ୟଥା ଯଦି (pinp && pinp->modVarp()->direction() != VDirection::ନିବେଶ) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } ଅନ୍ୟଥା ଯଦି (assignp && VN_IS(underp, ଗଣ୍ଠି_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* ଅଚଳ widenedp = ନୂତନ AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(ସତ୍ୟ);
                    // Shift left so zeros fill on the right
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ shiftedp = ନୂତନ AstShiftL{
                        underp->fileline(), widenedp,
                        ନୂତନ AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } ଅନ୍ୟଥା {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* checkCvtUS(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ତାର୍କିକ fatal) {
        ଯଦି (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            ଯଦି (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } ଅନ୍ୟଥା {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, ମିଥ୍ୟା, 32);
        }
        ଫେରନ୍ତୁ nodep;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* spliceCvtD(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        ଯଦି (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
            ଯଦି (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = ନୂତନ AstISToRD{nodep->fileline(), nodep};
            } ଅନ୍ୟଥା {
                newp = ନୂତନ AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            ଫେରନ୍ତୁ newp;
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ nodep;
        }
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* spliceCvtS(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ତାର୍କିକ warnOn, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ଯଦି (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                ଯଦି (constp->isDouble() && V3_ସଂଖ୍ୟା::epsilonIntegral(constp->ଅଙ୍କ().toDouble())) {
                    warnOn = ମିଥ୍ୟା;
                }
            }
            ଯଦି (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            ଫେରନ୍ତୁ newp;
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ nodep;
        }
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* spliceCvtString(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ଯଦି (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            ଫେରନ୍ତୁ newp;
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ nodep;
        }
    }
    Ast_ଗଣ୍ଠି_Biop* replaceWithUOrSVersion(Ast_ଗଣ୍ଠି_Biop* nodep, ତାର୍କିକ signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ଯଦି (signedFlavorNeeded == nodep->signedFlavor()) ଫେରନ୍ତୁ nullptr;
        ଯଦି (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        ବାଛ (nodep->type()) {
        ସ୍ଥିତି VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); ଫେରନ୍ତୁ nullptr;
        ସ୍ଥିତି VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); ଫେରନ୍ତୁ nullptr;
        ସ୍ଥିତି VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ଫେରନ୍ତୁ nullptr;
        ସ୍ଥିତି VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ଫେରନ୍ତୁ nullptr;
        ସ୍ଥିତି VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ଫେରନ୍ତୁ nullptr;
        ସ୍ଥିତି VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ଫେରନ୍ତୁ nullptr;
        ସ୍ଥିତି VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); ଫେରନ୍ତୁ nullptr;
        ସ୍ଥିତି VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); ଫେରନ୍ତୁ nullptr;
        ସ୍ଥିତି VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); ଫେରନ୍ତୁ nullptr;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଭାଙ୍ଗ;
        }
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_Biop* newp = nullptr;
        ବାଛ (nodep->type()) {
        ସ୍ଥିତି VNType::Gt: newp = ନୂତନ AstGtS{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::GtS: newp = ନୂତନ AstGt{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Gte: newp = ନୂତନ AstGteS{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::GteS: newp = ନୂତନ AstGte{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Lt: newp = ନୂତନ AstLtS{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::LtS: newp = ନୂତନ AstLt{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Lte: newp = ନୂତନ AstLteS{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::LteS: newp = ନୂତନ AstLte{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Div: newp = ନୂତନ AstDivS{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::DivS: newp = ନୂତନ AstDiv{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::ModDiv: newp = ନୂତନ AstModDivS{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::ModDivS: newp = ନୂତନ AstModDiv{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Mul: newp = ନୂତନ AstMulS{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::MulS: newp = ନୂତନ AstMul{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::ShiftR: newp = ନୂତନ AstShiftRS{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::ShiftRS: newp = ନୂତନ AstShiftR{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            ଭାଙ୍ଗ;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ newp;
    }
    Ast_ଗଣ୍ଠି_Biop* replaceWithDVersion(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ଯଦି (nodep->doubleFlavor()) ଫେରନ୍ତୁ nullptr;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        ବାଛ (nodep->type()) {
        ସ୍ଥିତି VNType::Add: newp = ନୂତନ AstAddD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Sub: newp = ନୂତନ AstSubD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Pow: newp = ନୂତନ AstPowD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Eq:
        ସ୍ଥିତି VNType::EqCase: newp = ନୂତନ AstEqD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Neq:
        ସ୍ଥିତି VNType::NeqCase: newp = ନୂତନ AstNeqD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Gt:
        ସ୍ଥିତି VNType::GtS: newp = ନୂତନ AstGtD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Gte:
        ସ୍ଥିତି VNType::GteS: newp = ନୂତନ AstGteD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Lt:
        ସ୍ଥିତି VNType::LtS: newp = ନୂତନ AstLtD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Lte:
        ସ୍ଥିତି VNType::LteS: newp = ନୂତନ AstLteD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Div:
        ସ୍ଥିତି VNType::DivS: newp = ନୂତନ AstDivD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Mul:
        ସ୍ଥିତି VNType::MulS: newp = ନୂତନ AstMulD{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ଭାଙ୍ଗ;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ newp;
    }
    Ast_ଗଣ୍ଠି_Biop* replaceWithNVersion(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        ଯଦି (nodep->stringFlavor()) ଫେରନ୍ତୁ nullptr;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        ବାଛ (nodep->type()) {
        ସ୍ଥିତି VNType::Eq:
        ସ୍ଥିତି VNType::EqCase:
        ସ୍ଥିତି VNType::EqWild: newp = ନୂତନ AstEqN{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Neq:
        ସ୍ଥିତି VNType::NeqCase:
        ସ୍ଥିତି VNType::NeqWild: newp = ନୂତନ AstNeqN{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Gt:
        ସ୍ଥିତି VNType::GtS: newp = ନୂତନ AstGtN{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Gte:
        ସ୍ଥିତି VNType::GteS: newp = ନୂତନ AstGteN{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Lt:
        ସ୍ଥିତି VNType::LtS: newp = ନୂତନ AstLtN{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ସ୍ଥିତି VNType::Lte:
        ସ୍ଥିତି VNType::LteS: newp = ନୂତନ AstLteN{fl, lhsp, rhsp}; ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            ଭାଙ୍ଗ;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ newp;
    }
    Ast_ଗଣ୍ଠି_Uniop* replaceWithDVersion(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ଯଦି (nodep->doubleFlavor()) ଫେରନ୍ତୁ nullptr;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_Uniop* newp = nullptr;
        ବାଛ (nodep->type()) {
        ସ୍ଥିତି VNType::Negate: newp = ନୂତନ AstNegateD{fl, lhsp}; ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ଭାଙ୍ଗ;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    ଶୂନ୍ୟ replaceWithSFormat(AstMethodCall* nodep, ଅଚଳ string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ଅଚଳ argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstAssign{
            nodep->fileline(), fromp,
            ନୂତନ AstSFormatF{nodep->fileline(), format, ମିଥ୍ୟା, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        ଠେଲ_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_ଗଣ୍ଠି_DType* iterateEditMoveDTypep(Ast_ଗଣ୍ଠି* parentp, Ast_ଗଣ୍ଠି_DType* dtnodep) {
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
        ଅଚଳ ତାର୍କିକ child1 = (parentp->getChildDTypep() == dtnodep);
        ଅଚଳ ତାର୍କିକ child2 = (parentp->getChild2DTypep() == dtnodep);
        ଯଦି (child1 || child2) {
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
        ଯଦି (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        ଫେରନ୍ତୁ dtnodep;
    }

    AstConst* dimension_ମୂଲ୍ୟ(File_ଧାଡ଼ି* fileline, Ast_ଗଣ୍ଠି_DType* nodep, VAttrType attrType,
                             ପୂର୍ଣ୍ଣସଂଖ୍ୟା dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_ଗଣ୍ଠି_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ adtypep = VN_CAST(dtypep, ଗଣ୍ଠି_Array_DType)) {
                declRange = adtypep->declRange();
                ଯଦି (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                ଜାରି;
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ adtypep
                       = VN_CAST(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                ଭାଙ୍ଗ;  // Sub elements don't look like arrays and can't iterate into
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ adtypep = VN_CAST(dtypep, BasicDType)) {
                ଯଦି (adtypep->isRanged()) declRange = adtypep->declRange();
                ଭାଙ୍ଗ;
            }
            ଭାଙ୍ଗ;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା val = 0;
        ବାଛ (attrType) {
        ସ୍ଥିତି VAttrType::DIM_BITS: {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା bits = 1;
            ଯେତେବେଳେ (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ adtypep = VN_CAST(dtypep, ଗଣ୍ଠି_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    ଜାରି;
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ adtypep
                           = VN_CAST(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    ଭାଙ୍ଗ;
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    ଭାଙ୍ଗ;
                }
                ଭାଙ୍ଗ;
            }
            ଯଦି (dim == 0) {
                val = 0;
            } ଅନ୍ୟଥା ଯଦି (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } ଅନ୍ୟଥା {
                val = bits;
            }
            ଭାଙ୍ଗ;  // LCOV_EXCL_LINE
        }
        ସ୍ଥିତି VAttrType::DIM_ଉପର: val = !declRange.ranged() ? 0 : declRange.hi(); ଭାଙ୍ଗ;
        ସ୍ଥିତି VAttrType::DIM_ବାମ: val = !declRange.ranged() ? 0 : declRange.ବାମ(); ଭାଙ୍ଗ;
        ସ୍ଥିତି VAttrType::DIM_ତଳ: val = !declRange.ranged() ? 0 : declRange.lo(); ଭାଙ୍ଗ;
        ସ୍ଥିତି VAttrType::DIM_ଡାହାଣ: val = !declRange.ranged() ? 0 : declRange.ଡାହାଣ(); ଭାଙ୍ଗ;
        ସ୍ଥିତି VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: nodep->v3fatalSrc("Missing DIM ATTR type case"); ଭାଙ୍ଗ;
        }
        ଯଦି (!valp) valp = ନୂତନ AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        ଫେରନ୍ତୁ valp;
    }
    AstVarRef* dimensionVarRefp(Ast_ଗଣ୍ଠି_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        ଯଦି (cachedpr) ଫେରନ୍ତୁ cachedpr->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);

        Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ vardtypep = ନୂତନ AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), ନୂତନ AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* ଅଚଳ consp = ନୂତନ AstConsArray{nodep->fileline(), vardtypep};
        ପାଇଁ (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_ମୂଲ୍ୟ(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_ବୃକ୍ଷ(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(ସତ୍ୟ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(ସତ୍ୟ);
        ଠେଲ_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ଫେରନ୍ତୁ cachedpr->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
    }
    ସ୍ଥିର uint64_t enum_Max_ମୂଲ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି* errNodep, ଅଚଳ AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        ପାଇଁ (ଅଚଳ AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ଅଚଳ AstConst* ଅଚଳ vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            ଯଦି (!vconstp->ଅଙ୍କ().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        ଯଦି (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            ଫେରନ୍ତୁ 64;
        }
        ଫେରନ୍ତୁ maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ଅଚଳ nodep, VAttrType attrType, ତାର୍କିକ assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        ଯଦି (cachedpr) ଫେରନ୍ତୁ cachedpr->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି_DType* basep;
        ଯଦି (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } ଅନ୍ୟଥା ଯଦି (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } ଅନ୍ୟଥା {
            basep = nodep->dtypep();
        }
        Ast_ଗଣ୍ଠି_DType* vardtypep;
        ଯଦି (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = ନୂତନ AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } ଅନ୍ୟଥା {
            vardtypep = ନୂତନ AstUnpackArrayDType{flp, basep, ନୂତନ AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        ଯଦି (attrType == VAttrType::ENUM_NAME) {
            defaultp = ନୂତନ AstConst{flp, AstConst::String{}, ""};
        } ଅନ୍ୟଥା ଯଦି (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = ନୂତନ AstConst{flp, V3_ସଂଖ୍ୟା{nodep, nodep->width(), 0}};
        } ଅନ୍ୟଥା ଯଦି (attrType == VAttrType::ENUM_VALID) {
            defaultp = ନୂତନ AstConst{flp, AstConst::BitFalse{}};
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> ମୂଲ୍ୟଗୁଡ଼ିକ;
        {
            uint64_t ordinal = 0;
            AstEnumItem* ଅଚଳ firstp = nodep->itemsp();
            ଅଚଳ AstEnumItem* prevp = firstp;  // Prev must start with last item
            ଯେତେବେଳେ (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            ପାଇଁ (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* ଅଚଳ nextp = VN_AS(itemp->nextp(), EnumItem);
                ଅଚଳ AstConst* ଅଚଳ vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                ଯଦି (!vconstp->ଅଙ୍କ().isAnyXZ()) {  // Can 2-state runtime decode
                    ଅଚଳ uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    ଯଦି (attrType == VAttrType::ENUM_NAME) {
                        ମୂଲ୍ୟଗୁଡ଼ିକ[i] = ନୂତନ AstConst{flp, AstConst::String{},
                                                 V3_ସଂଖ୍ୟା::displayedEnumName(itemp)};
                    } ଅନ୍ୟଥା ଯଦି (attrType == VAttrType::ENUM_NEXT) {
                        ମୂଲ୍ୟଗୁଡ଼ିକ[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);  // A const
                    } ଅନ୍ୟଥା ଯଦି (attrType == VAttrType::ENUM_PREV) {
                        ମୂଲ୍ୟଗୁଡ଼ିକ[i] = prevp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);  // A const
                    } ଅନ୍ୟଥା ଯଦି (attrType == VAttrType::ENUM_VALID) {
                        ମୂଲ୍ୟଗୁଡ଼ିକ[i] = ନୂତନ AstConst{flp, AstConst::BitTrue{}};
                    } ଅନ୍ୟଥା {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = nullptr;
        ଯଦି (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ଅଚଳ consp = ନୂତନ AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            ପାଇଁ (ଅଚଳ auto& itr : ମୂଲ୍ୟଗୁଡ଼ିକ) {
                V3_ସଂଖ୍ୟା ଚାବି{nodep, nodep->width()};
                ଚାବି.setQuad(itr.first);
                consp->addItemsp(ନୂତନ AstConsAssocItem{flp, ନୂତନ AstConst{flp, ଚାବି}, itr.second});
            }
            valuep = consp;
        } ଅନ୍ୟଥା {
            AstConsArray* ଅଚଳ consp = ନୂତନ AstConsArray{flp, vardtypep, defaultp};
            ପାଇଁ (uint64_t i = 0; i < (msbdim + 1); ++i) {
                ଯଦି (ମୂଲ୍ୟଗୁଡ଼ିକ[i]) consp->valuep(i, ମୂଲ୍ୟଗୁଡ଼ିକ[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(ସତ୍ୟ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(ସତ୍ୟ);
        ଠେଲ_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ଫେରନ୍ତୁ cachedpr->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enumSelect(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        ଯଦି (attrType == VAttrType::ENUM_NAME) {
            ଯଦି (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->ଅଙ୍କ().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ newp;
            }
        }
        ଯଦି (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ capturep = newEnumCapture(nodep, adtypep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            ଫେରନ୍ତୁ capturep;
        }
        ଯଦି (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* indexp = ନୂତନ AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            ପାଇଁ (ଅଚଳ AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(itemp->valuep(), Const);
                ଯଦି (constp->ଅଙ୍କ().isAnyXZ()) ଜାରି;
                indexp = ନୂତନ AstCond{nodep->fileline(),
                                     ନୂତନ AstEq{nodep->fileline(), nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                               itemp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)},
                                     ନୂତନ AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ଅଚଳ tabRefp = enumVarRefp(adtypep, attrType, ମିଥ୍ୟା, ordinal);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ newp;
        }
        ଅଚଳ uint64_t msbdim = enum_Max_ମୂଲ୍ୟ(nodep, adtypep);
        ଅଚଳ ତାର୍କିକ assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
        ଯଦି (assoc) {
            AstVarRef* ଅଚଳ tabRefp = enumVarRefp(adtypep, attrType, ସତ୍ୟ, 0);
            newp = ନୂତନ AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } ଅନ୍ୟଥା {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା selwidth = V3_ସଂଖ୍ୟା::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ଅଚଳ tabRefp
                = enumVarRefp(adtypep, attrType, ମିଥ୍ୟା, (1ULL << selwidth) - 1);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? ନୂତନ AstGt{nodep->fileline(), nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                  ନୂତନ AstConst{nodep->fileline(),
                                               V3_ସଂଖ୍ୟା{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = ନୂତନ AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                ନୂତନ AstSel{nodep->fileline(), nodep, 0, selwidth}};
            ଯଦି (boundp) {
                newp = ନୂତନ AstCond{nodep->fileline(), boundp,
                                   ନୂତନ AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        ଯଦି (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } ଅନ୍ୟଥା ଯଦି (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } ଅନ୍ୟଥା {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        ଫେରନ୍ତୁ newp;
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enumTestValid(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valp, AstEnumDType* enumDtp) {
        ଅଚଳ uint64_t maxval = enum_Max_ମୂଲ୍ୟ(valp, enumDtp);
        ଅଚଳ ତାର୍କିକ assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* testp = nullptr;
        File_ଧାଡ଼ି* ଅଚଳ fl_novalue = ନୂତନ File_ଧାଡ଼ି{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, ସତ୍ୟ);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, ସତ୍ୟ);
        ଯଦି (assoc) {
            AstVarRef* ଅଚଳ tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ସତ୍ୟ, 0);
            testp = ନୂତନ AstAssocSel{fl_novalue, tabRefp, valp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
        } ଅନ୍ୟଥା {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା selwidth = V3_ସଂଖ୍ୟା::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ଅଚଳ tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ମିଥ୍ୟା, (1ULL << selwidth) - 1);
            File_ଧାଡ଼ି* ଅଚଳ fl_nowidth = ନୂତନ File_ଧାଡ଼ି{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, ସତ୍ୟ);
            testp = ନୂତନ AstCond{
                fl_novalue,
                ନୂତନ AstGt{fl_nowidth, valp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                          ନୂତନ AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                ନୂତନ AstConst{fl_novalue, AstConst::BitFalse{}},
                ନୂତନ AstArraySel{fl_novalue, tabRefp,
                                ନୂତନ AstSel{fl_novalue, valp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), 0, selwidth}}};
        }
        ଫେରନ୍ତୁ testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, ଅଚଳ VNumRange& range) {
        PatVecMap patmap;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା element = range.ବାମ();
        ପାଇଁ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ଯଦି (patp->keyp()) {
                ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = patternConstKeyp(patp)) {
                    ଯଦି (constp->ଅଙ୍କ().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } ଅନ୍ୟଥା {
                        element = constp->toSInt();
                    }
                }
            }
            ଅଚଳ ତାର୍କିକ newEntry = patmap.emplace(element, patp).second;
            ଯଦି (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.ବାମ_To_ଡାହାଣ_Inc();
        }
        ଫେରନ୍ତୁ patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    ଶୂନ୍ୟ patConcatConvertRecurse(AstPattern* patternp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (AstConcat* ଅଚଳ concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } ଅନ୍ୟଥା {
            AstPatMember* ଅଚଳ newp
                = ନୂତନ AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(ସତ୍ୟ);
            patternp->addItemsp(newp);
        }
    }

    ଶୂନ୍ୟ makeOpenArrayShell(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_ଗଣ୍ଠି_FTask* ଅଚଳ oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_ଗଣ୍ଠି_FTask* ଅଚଳ newTaskp = oldTaskp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        newTaskp->dpiOpenChild(ସତ୍ୟ);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            AstVar* ଅଚଳ portp = tconnect.first;
            ଅଚଳ AstArg* ଅଚଳ argp = tconnect.second;
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ pinp = argp->exprp();
            ଯଦି (!pinp) ଜାରି;  // Argument error we'll find later
            ଯଦି (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_ଗଣ୍ଠି* nodep) {
                    ଯଦି (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    ତାର୍କିକ markHasOpenArray(Ast_ଗଣ୍ଠି_FTask* nodep) {
        ତାର୍କିକ hasOpen = ମିଥ୍ୟା;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                ଯଦି (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(ସତ୍ୟ);
                    hasOpen = ସତ୍ୟ;
                }
            }
        }
        ଫେରନ୍ତୁ hasOpen;
    }
    ତାର୍କିକ hasOpenArrayDTypeRecurse(Ast_ଗଣ୍ଠି_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        ଯଦି (VN_IS(nodep, UnsizedArrayDType)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (nodep->subDTypep()) ଫେରନ୍ତୁ hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    Ast_ଗଣ୍ଠି_DType* dtypeNotIntAtomOrVecRecurse(Ast_ଗଣ୍ଠି_DType* nodep, ତାର୍କିକ ranged = ମିଥ୍ୟା) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        ଯଦି (AstBasicDType* ଅଚଳ dtp = VN_CAST(nodep, BasicDType)) {
            ଯଦି (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                ଫେରନ୍ତୁ dtp;  // Packed when already packed
            ଯଦି (dtp->keyword().isIntNumeric()) ଫେରନ୍ତୁ nullptr;
            ଫେରନ୍ତୁ dtp;
        } ଅନ୍ୟଥା ଯଦି (AstPackArrayDType* ଅଚଳ dtp = VN_CAST(nodep, PackArrayDType)) {
            ଯଦି (ranged) ଫେରନ୍ତୁ dtp;  // Packed when already packed
            ଫେରନ୍ତୁ dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), ସତ୍ୟ);
        }
        ଫେରନ୍ତୁ nodep;
    }
    Ast_ଗଣ୍ଠି_DType* dtypeNot4StateIntegralRecurse(Ast_ଗଣ୍ଠି_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        ଯଦି (AstBasicDType* ଅଚଳ dtp = VN_CAST(nodep, BasicDType)) {
            ଯଦି (!dtp->keyword().isFourstate()) ଫେରନ୍ତୁ dtp;
            ଫେରନ୍ତୁ nullptr;
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ dtp = VN_CAST(nodep, ଗଣ୍ଠି_Array_DType)) {
            ଫେରନ୍ତୁ dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ dtp = VN_CAST(nodep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ପାଇଁ (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                ଯଦି (badDtp) ଫେରନ୍ତୁ badDtp;
            }
            ଫେରନ୍ତୁ nullptr;
        }
        ଫେରନ୍ତୁ nodep;
    }
    ଶୂନ୍ୟ checkForceReleaseLhs(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    ଶୂନ୍ୟ assert_At_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    ଶୂନ୍ୟ assertAtStatement(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    ଶୂନ୍ୟ checkConstantOrReplace(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ noFourState, ଅଚଳ string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ଅଚଳ constp = VN_CAST(nodep, Const);
        ଯଦି (!constp || (noFourState && constp->ଅଙ୍କ().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ଠେଲ_Deletep(nodep), nodep, constp);
            ଫେରନ୍ତୁ;
        }
    }
    Ast_ଗଣ୍ଠି* ଗଣ୍ଠି_For_Unsized_Warning(Ast_ଗଣ୍ଠି* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        ଯଦି (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            ଫେରନ୍ତୁ nodep->op1p();
        } ଅନ୍ୟଥା ଯଦି (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            ଫେରନ୍ତୁ nodep->op2p();
        }
        ଫେରନ୍ତୁ nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_ଗଣ୍ଠି* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        ଯଦି (VL_LIKELY(!nodep)) ଫେରନ୍ତୁ nullptr;
        ଯଦି (auto* ଅଚଳ refp = VN_CAST(nodep, RefDType)) {
            ଯଦି (refp->typedefp() == typedefp) ଫେରନ୍ତୁ refp;
        }
        ଯଦି (auto* ଅଚଳ refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) ଫେରନ୍ତୁ refp;
        ଯଦି (auto* ଅଚଳ refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) ଫେରନ୍ତୁ refp;
        ଯଦି (auto* ଅଚଳ refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) ଫେରନ୍ତୁ refp;
        ଯଦି (auto* ଅଚଳ refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) ଫେରନ୍ତୁ refp;
        ଫେରନ୍ତୁ nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    ସ୍ଥିର ତାର୍କିକ isIfaceArrayDType(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଯଦି (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstIfaceRefDType* ଅଚଳ irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        ଫେରନ୍ତୁ irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    ସ୍ଥିର std::string unpackedArraySize(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        ଯେତେବେଳେ (ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            ଯଦି (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        ଫେରନ୍ତୁ size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newIfaceArrayElement(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) {
        AstConsArray* ଅଚଳ consp = VN_CAST(fromp, ConsArray);
        ଯଦି (!consp || !isIfaceArrayDType(consp->dtypep())) ଫେରନ୍ତୁ nullptr;
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        ଯଦି (bit < 0 || bit >= arrp->elementsConst()) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ consp->valuep(bit)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* new_Iface_Array_ମୂଲ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* refp, Ast_ଗଣ୍ଠି_DType* dtypep,
                                    ଅଚଳ std::string& name, ଅଚଳ std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_ଧାଡ଼ି* ଅଚଳ flp = refp->fileline();
        AstUnpackArrayDType* ଅଚଳ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        ଯଦି (!arrp) {
            AstVar* ଅଚଳ varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_ଗଣ୍ଠି_Var_Ref* newp;
            ଯଦି (ଅଚଳ AstVarXRef* ଅଚଳ xrefp = VN_CAST(refp, VarXRef)) {
                newp = ନୂତନ AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } ଅନ୍ୟଥା {
                newp = ନୂତନ AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            ଫେରନ୍ତୁ newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ଅଚଳ consp = ନୂତନ AstConsArray{flp, arrp};
        ଅଚଳ VNumRange range = arrp->declRange();
        ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା i : range.seq_ବାମ_To_ଡାହାଣ()) {
            ଅଚଳ std::string s = name + "__BRA__" + Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_ମୂଲ୍ୟ(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        ଫେରନ୍ତୁ consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    ଶୂନ୍ୟ replaceSplitIfaceArrayRef(Ast_ଗଣ୍ଠି_Var_Ref* nodep) {
        AstVar* ଅଚଳ varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        ଯଦି (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_ମୂଲ୍ୟ(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    ଶୂନ୍ୟ expandIfaceArrayPinDimensions(AstPin* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep,
                                       Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* conp, ଅଚଳ std::string& suffix,
                                       AstVar*& elemVarpr) {
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        ଯଦି (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ଅଚଳ newp = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            ଯଦି (conp) newp->exprp(conp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
            nodep->addNextHere(newp);
            ଫେରନ୍ତୁ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ଅଚଳ VNumRange range = arrp->declRange();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = range.elements() - 1; n >= 0; --n) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = range.ବାମ() + n * range.ବାମ_To_ଡାହାଣ_Inc();
            ଅଚଳ std::string s = suffix + "__BRA__" + Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elemConnp = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                ଯଦି (!conp) ଫେରନ୍ତୁ nullptr;
                ଅଚଳ AstUnpackArrayDType* ଅଚଳ conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements = conArrp->elementsConst();
                // Counted from lo
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                ଫେରନ୍ତୁ resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    ଶୂନ୍ୟ expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ଅଚଳ portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        ପାଇଁ (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = VN_AS(nodep->exprp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        ଯଦି (exprp) ଠେଲ_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ଅଚଳ ତାର୍କିକ connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_ଗଣ୍ଠି* userIterateSubtreeReturnEdits(Ast_ଗଣ୍ଠି* nodep, WidthVP* vup) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ nullptr;
        Ast_ଗଣ୍ଠି* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        ଫେରନ୍ତୁ ret;
    }
    ଶୂନ୍ୟ userIterate(Ast_ଗଣ୍ଠି* nodep, WidthVP* vup) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    ଶୂନ୍ୟ userIterateAndNext(Ast_ଗଣ୍ଠି* nodep, WidthVP* vup) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ;
        ଯଦି (nodep->didWidth()) ଫେରନ୍ତୁ;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    ଶୂନ୍ୟ userIterateChildren(Ast_ଗଣ୍ଠି* nodep, WidthVP* vup) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ userIterateChildrenBackwardsConst(Ast_ଗଣ୍ଠି* nodep, WidthVP* vup) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    WidthVisitor(ତାର୍କିକ paramsOnly,  // [in] TRUE if we are considering parameters only.
                 ତାର୍କିକ doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_ଗଣ୍ଠି* mainAcceptEdit(Ast_ଗଣ୍ଠି* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        ପାଇଁ (AstVar* ଅଚଳ varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(ଠେଲ_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        ଫେରନ୍ତୁ nodep;
    }
    ସ୍ଥିର ତାର୍କିକ lowerAsFixedAggregate(ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) {
        ଫେରନ୍ତୁ dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    ଶୂନ୍ୟ streamAssignLower(Ast_ଗଣ୍ଠି_Assign* nodep) {
        Ast_ଗଣ୍ଠି_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        ଯଦି (Ast_ଗଣ୍ଠି_Stream* ଅଚଳ streamp = VN_CAST(nodep->rhsp(), ଗଣ୍ଠି_Stream)) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lwidth = lhsDTypeSkippedRefp->widthStream();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            ଯଦି (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            ଯଦି (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            ଯଦି (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    ନୂତନ AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        ଯଦି (Ast_ଗଣ୍ଠି_Stream* ଅଚଳ streamp = VN_CAST(nodep->lhsp(), ଗଣ୍ଠି_Stream)) {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lwidth = lhsStreamDTypep->widthStream();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rwidth = rhsDTypep->widthStream();
            ଯଦି (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            ଯଦି (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ dstp = streamp->lhsp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcp = nodep->rhsp()->unlinkFrBack();
                ଯଦି (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } ଅନ୍ୟଥା {
                    ଯଦି (srcp->width() > lwidth) {
                        srcp = ନୂତନ AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ଠେଲ_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(ନୂତନ AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(ନୂତନ AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Width class functions

ଶୂନ୍ୟ V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ଅଚଳ WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ଅଚଳ VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{ମିଥ୍ୟା, ମିଥ୍ୟା};
        (ଶୂନ୍ୟ)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (ଶୂନ୍ୟ)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("width", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_ଗଣ୍ଠି* V3Width::widthParamsEdit(Ast_ଗଣ୍ଠି* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{ସତ୍ୟ, ମିଥ୍ୟା};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ଫେରନ୍ତୁ nodep;
}

ଶୂନ୍ୟ V3Width::streamAssignLowerEdit(Ast_ଗଣ୍ଠି_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{ମିଥ୍ୟା, ମିଥ୍ୟା};
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
Ast_ଗଣ୍ଠି* V3Width::widthGenerateParamsEdit(
    Ast_ଗଣ୍ଠି* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{ସତ୍ୟ, ସତ୍ୟ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ଫେରନ୍ତୁ nodep;
}

Ast_ଗଣ୍ଠି* V3Width::selectNonConstantRecurse(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ inSel) {
    // If node has a non-constant select, return that select
    Ast_ଗଣ୍ଠି* resultp = nullptr;
    ଯଦି (Ast_ଗଣ୍ଠି_Sel* ଅଚଳ anodep = VN_CAST(nodep, ଗଣ୍ଠି_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        ଯଦି (resultp) ଫେରନ୍ତୁ resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), ସତ୍ୟ);
    } ଅନ୍ୟଥା ଯଦି (AstSel* ଅଚଳ anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        ଯଦି (resultp) ଫେରନ୍ତୁ resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), ସତ୍ୟ);
    } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ anodep = VN_CAST(nodep, ଗଣ୍ଠି_Var_Ref)) {
        ଯଦି (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) ଫେରନ୍ତୁ anodep;
    } ଅନ୍ୟଥା {
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        ଯଦି (resultp) ଫେରନ୍ତୁ resultp;
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        ଯଦି (resultp) ଫେରନ୍ତୁ resultp;
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        ଯଦି (resultp) ଫେରନ୍ତୁ resultp;
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    ଫେରନ୍ତୁ resultp;
}
