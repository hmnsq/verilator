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
#تعریف_کرو VERILATOR_V3_چوڑائی_CPP
#شامل "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ثابت Ast_گرہ*, AstClass*> m_cache;

    AstClass* ڈھونڈو_Cached(Ast_گرہ* nodep) {
        اگر (!nodep) واپس خالی_اشاریہ;
        ثابت خودکار it = m_cache.ڈھونڈو(nodep);
        اگر (it != m_cache.انتہا()) واپس it->دوسرا;

        AstClass* classp = خالی_اشاریہ;
        اگر (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = ڈھونڈو_Cached(nodep->backp());
        } ورنہ اگر (AstClass* ثابت parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } ورنہ اگر (AstClassPackage* ثابت packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } ورنہ {
            classp = ڈھونڈو_Cached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        واپس classp;
    }

عوامی:
    AstClass* ڈھونڈو(Ast_گرہ* nodep) {
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) واپس classp;
        اگر (AstClassPackage* ثابت packagep = VN_CAST(nodep, ClassPackage)) {
            واپس packagep->classp();
        }
        واپس ڈھونڈو_Cached(nodep);
    }
};

//######################################################################

شمار Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& عامل<<(std::ostream& str, ثابت Stage& rhs) {
    واپس str << ("-PFB"[static_cast<صحیح>(rhs)]);
}

شمار Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    تفویض_3  // Assignment-like where sign comes from RHS only
};
std::ostream& عامل<<(std::ostream& str, ثابت Determ& rhs) {
    ساکن constexpr ثابت حرف* ثابت s_det[] = {"SELF", "CNTX", "ASSN"};
    واپس str << s_det[rhs];
}

شمار دھارا_Use : uint8_t {
    دھارا_USE_NONE,  // Stream concatenation is not allowed in this context
    دھارا_USE_تفویض,  // Assignment-like context permits stream concatenation
    دھارا_USE_CAST,  // Explicit cast context permits stream concatenation
};

#تعریف_کرو v3widthWarn(lhs, rhs, پیغام_2) \
    v3warn_رمز(((lhs) < (rhs)   ? V3_Error_رمز::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3_Error_رمز::WIDTHEXPAND \
                                : V3_Error_رمز::چوڑائی_2), \
               پیغام_2)

//######################################################################
// Width state, as a visitor of each AstNode

جماعت چوڑائی_VP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_گرہ_DType* ثابت m_dtypep;  // Parent's data type to resolve to
    ثابت Stage m_stage;  // If true, report errors
    ثابت دھارا_Use m_دھارا_Use;  // Current expression may be a stream concat

عوامی:
    چوڑائی_VP(Ast_گرہ_DType* dtypep, Stage stage, دھارا_Use دھارا_Use_2 = دھارا_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_دھارا_Use{دھارا_Use_2} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    چوڑائی_VP(Determ determ, Stage stage, دھارا_Use دھارا_Use_2 = دھارا_USE_NONE)
        : m_dtypep{خالی_اشاریہ}
        , m_stage{stage}
        , m_دھارا_Use{دھارا_Use_2} {
        اگر (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    چوڑائی_VP* p() { واپس یہ; }
    منطقی selfDtm() ثابت { واپس m_dtypep == خالی_اشاریہ; }
    دھارا_Use دھارا_Use_2() ثابت { واپس m_دھارا_Use; }
    Ast_گرہ_DType* dtypep() ثابت {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        واپس m_dtypep;
    }
    Ast_گرہ_DType* dtypeNullp() ثابت { واپس m_dtypep; }
    Ast_گرہ_DType* dtypeNullSkipRefp() ثابت {
        Ast_گرہ_DType* dtp = dtypeNullp();
        اگر (dtp) dtp = dtp->skipRefp();
        واپس dtp;
    }
    Ast_گرہ_DType* dtypeOverridep(Ast_گرہ_DType* ثابت defaultp) ثابت {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        اگر (!m_dtypep) واپس defaultp;
        اگر (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            واپس defaultp->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(m_dtypep->چوڑائی(), m_dtypep->چوڑائی_Min(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        واپس m_dtypep;
    }
    صحیح چوڑائی() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        واپس m_dtypep->چوڑائی();
    }
    صحیح چوڑائی_Min() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        واپس m_dtypep->چوڑائی_Min();
    }
    منطقی prelim() ثابت { واپس m_stage & PRELIM; }
    منطقی final() ثابت { واپس m_stage & FINAL; }
    خالی dump(std::ostream& str) ثابت {
        اگر (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } ورنہ {
            str << "  VUP(s=" << m_stage << ",dt=" << cvt_مقصد_Hex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& عامل<<(std::ostream& str, ثابت چوڑائی_VP* vup) {
    اگر (vup) vup->dump(str);
    واپس str;
}

//######################################################################

جماعت چوڑائی_Clear_Visitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    خالی clear_چوڑائی_Recurse(Ast_گرہ* nodep) {
        برائے (; nodep; nodep = nodep->nextp()) {
            nodep->did_چوڑائی(جھوٹ);
            اگر (Ast_گرہ* ثابت refp = nodep->op1p()) clear_چوڑائی_Recurse(refp);
            اگر (Ast_گرہ* ثابت refp = nodep->op2p()) clear_چوڑائی_Recurse(refp);
            اگر (Ast_گرہ* ثابت refp = nodep->op3p()) clear_چوڑائی_Recurse(refp);
            اگر (Ast_گرہ* ثابت refp = nodep->op4p()) clear_چوڑائی_Recurse(refp);
        }
    }

عوامی:
    // CONSTRUCTORS
    صریح چوڑائی_Clear_Visitor(AstNetlist* nodep) { clear_چوڑائی_Recurse(nodep); }
    مجازی ~چوڑائی_Clear_Visitor() = طےشدہ;
};

//######################################################################

#تعریف_کرو accept اندر_چوڑائی_Visitor_use_Ast_گرہ_iterate_instead_کا_Ast_گرہ_accept

//######################################################################

جماعت چوڑائی_Visitor final : عوامی VNVisitor {
    // TYPES
    استعمال PatVecMap = std::map<صحیح, AstPatMember*>;
    استعمال DTypeMap = std::map<ثابت std::string, AstPatMember*>;

    // STATE
    V3_Unique_نام m_enum_عارضی_نام;  // Captured enum expressions
    V3_Unique_نام m_inside_عارضی_نام;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ثابت AstModport*, std::unordered_سیٹ<string>>
        m_modport_نام;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    چوڑائی_VP* m_vup = خالی_اشاریہ;  // Current node state
    منطقی m_underFork = جھوٹ;  // Visiting under a fork
    منطقی m_underSExpr = جھوٹ;  // Visiting under a sequence expression
    منطقی m_underPackedArray = جھوٹ;  // Visiting under a AstPackArrayDType
    منطقی m_underMemberSel = جھوٹ;  // Viting under a MemberSel
    Ast_گرہ* m_seqUnsupp = خالی_اشاریہ;  // Property has unsupported node
    منطقی m_hasSExpr = جھوٹ;  // Property has a sequence expression
    ثابت AstCell* m_cellp = خالی_اشاریہ;  // Current cell for arrayed instantiations
    std::سمتیہ<AstVar*> m_بانٹو_Iface_Array_Varps;  // Interface arrays split by V3Param
    ثابت Ast_Enum_شے* m_enumItemp = خالی_اشاریہ;  // Current enum item
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current function/task
    Ast_گرہ_Module* m_modep = خالی_اشاریہ;  // Current module
    ثابت AstConstraint* m_constraintp = خالی_اشاریہ;  // Current constraint
    Ast_گرہ_Procedure* m_procedurep = خالی_اشاریہ;  // Current final/always
    ثابت AstWith* m_withp = خالی_اشاریہ;  // Current 'with' statement
    ثابت AstFunc* m_funcp = خالی_اشاریہ;  // Current function
    ثابت Ast_Attr_کا* m_attrp = خالی_اشاریہ;  // Current attribute
    ثابت Ast_گرہ_اظہاریہ* m_randomizeFromp = خالی_اشاریہ;  // Current randomize method call fromp
    ثابت منطقی m_paramsOnly;  // Computing parameter value; limit operation
    ثابت منطقی m_doGenerate;  // Do errors later inside generate statement
    منطقی m_دھارا_Concat = جھوٹ;  // True if visiting arguments of stream concatenation
    منطقی m_اندر_Param_Override = جھوٹ;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ثابت Ast_گرہ_DType*, VAttrType>, AstVarRef*> m_جدول_Map;
    // Queues with given index type
    std::map<ثابت Ast_گرہ_DType*, Ast_قطار_DType*> m_قطار_DType_اشاریہ_دار;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_سیٹ<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_سیٹ<ثابت AstVar*> m_curModVars;  // Variables declared in current module

    ساکن constexpr صحیح ENUM_LOOKUP_بٹ = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    شمار Extend_قاعدہ : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    شمار جماعت ComparisonType : uint8_t {
        معمول_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        معمول_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    ساکن دھارا_Use دھارا_Use_For_Determ(Determ determ) {
        واپس determ == تفویض_3 ? دھارا_USE_تفویض : دھارا_USE_NONE;
    }

    دھارا_Use current_دھارا_Use() ثابت { واپس m_vup ? m_vup->دھارا_Use_2() : دھارا_USE_NONE; }

    ساکن خالی packIfUnpacked(Ast_گرہ_اظہاریہ* ثابت nodep) {
        اگر (AstUnpackArrayDType* ثابت unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ثابت صحیح elements_عدد = unpackDTypep->arrayUnpackedElements();
            ثابت صحیح unpack_Min_بٹ = elements_عدد * unpackDTypep->subDTypep()->چوڑائی_Min();
            ثابت صحیح unpack_بٹ = elements_عدد * unpackDTypep->subDTypep()->چوڑائی();
            VNRelinker relinker;
            nodep->unlink_Fr_عقب(&relinker);
            relinker.relink(نیا Ast_Cvt_Array_مقصد_Packed{
                nodep->fileline(), nodep,
                nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(unpack_بٹ, unpack_Min_بٹ, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    ساکن Ast_گرہ_DType* fromDTypep(Ast_گرہ* fromp) {
        اگر (Ast_گرہ_DType* ثابت dtypep = VN_CAST(fromp, گرہ_DType))
            واپس dtypep->skipRefOrNullp();
        واپس fromp ? fromp->dtypep() : خالی_اشاریہ;
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
    خالی دورہ(AstLogNot* nodep) override { دورہ_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    خالی دورہ(AstLogAnd* nodep) override { دورہ_log_and_or(nodep); }
    خالی دورہ(AstLogOr* nodep) override { دورہ_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    خالی دورہ(AstSAnd* nodep) override { دورہ_log_and_or(nodep); }
    خالی دورہ(AstSIntersect* nodep) override { دورہ_log_and_or(nodep); }
    خالی دورہ(AstSOr* nodep) override { دورہ_log_and_or(nodep); }
    خالی دورہ(AstSWithin* nodep) override { دورہ_log_and_or(nodep); }
    خالی دورہ(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        دورہ_log_and_or(nodep);
    }
    خالی دورہ(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        دورہ_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    خالی دورہ(AstRedAnd* nodep) override { دورہ_red_and_or(nodep); }
    خالی دورہ(AstRedOr* nodep) override { دورہ_red_and_or(nodep); }
    خالی دورہ(AstRedXor* nodep) override { دورہ_red_and_or(nodep); }
    خالی دورہ(AstOneHot* nodep) override { دورہ_red_and_or(nodep); }
    خالی دورہ(AstOneHot0* nodep) override { دورہ_red_and_or(nodep); }
    خالی دورہ(AstIsUnknown* nodep) override {
        دورہ_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    خالی دورہ(AstEq* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_EQ); }
    خالی دورہ(AstNeq* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_EQ); }
    خالی دورہ(AstGt* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_GL); }
    خالی دورہ(AstGte* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_GL); }
    خالی دورہ(AstLt* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_GL); }
    خالی دورہ(AstLte* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_GL); }
    خالی دورہ(AstGtS* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_GL); }
    خالی دورہ(AstGteS* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_GL); }
    خالی دورہ(AstLtS* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_GL); }
    خالی دورہ(AstLteS* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::معمول_GL); }
    خالی دورہ(AstEqCase* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::CASE); }
    خالی دورہ(AstNeqCase* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    خالی دورہ(AstEqWild* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::WILD); }
    خالی دورہ(AstNeqWild* nodep) override { دورہ_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    خالی دورہ(AstEqD* nodep) override { دورہ_cmp_real(nodep); }
    خالی دورہ(AstNeqD* nodep) override { دورہ_cmp_real(nodep); }
    خالی دورہ(AstLtD* nodep) override { دورہ_cmp_real(nodep); }
    خالی دورہ(AstLteD* nodep) override { دورہ_cmp_real(nodep); }
    خالی دورہ(AstGtD* nodep) override { دورہ_cmp_real(nodep); }
    خالی دورہ(AstGteD* nodep) override { دورہ_cmp_real(nodep); }
    // ...    String compares
    خالی دورہ(AstEqN* nodep) override { دورہ_cmp_string(nodep); }
    خالی دورہ(AstNeqN* nodep) override { دورہ_cmp_string(nodep); }
    خالی دورہ(AstLtN* nodep) override { دورہ_cmp_string(nodep); }
    خالی دورہ(AstLteN* nodep) override { دورہ_cmp_string(nodep); }
    خالی دورہ(AstGtN* nodep) override { دورہ_cmp_string(nodep); }
    خالی دورہ(AstGteN* nodep) override { دورہ_cmp_string(nodep); }
    // ...    Data type compares
    خالی دورہ(AstEqT* nodep) override { دورہ_cmp_type(nodep); }
    خالی دورہ(AstNeqT* nodep) override { دورہ_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    خالی دورہ(AstAnd* nodep) override { دورہ_boolexpr_and_or(nodep); }
    خالی دورہ(AstOr* nodep) override { دورہ_boolexpr_and_or(nodep); }
    خالی دورہ(AstXor* nodep) override { دورہ_boolexpr_and_or(nodep); }
    خالی دورہ(AstBufIf1* nodep) override {
        دورہ_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    خالی دورہ(Ast_جوڑو* nodep) override { دورہ_جوڑو_sub_replace(nodep, سچ, جھوٹ); }
    خالی دورہ(AstSub* nodep) override { دورہ_جوڑو_sub_replace(nodep, سچ, جھوٹ); }
    خالی دورہ(AstDiv* nodep) override { دورہ_جوڑو_sub_replace(nodep, سچ, سچ); }
    خالی دورہ(AstMul* nodep) override { دورہ_جوڑو_sub_replace(nodep, سچ, جھوٹ); }
    // These can't promote to real
    خالی دورہ(AstModDiv* nodep) override { دورہ_جوڑو_sub_replace(nodep, جھوٹ, سچ); }
    خالی دورہ(AstModDivS* nodep) override { دورہ_جوڑو_sub_replace(nodep, جھوٹ, سچ); }
    خالی دورہ(AstMulS* nodep) override { دورہ_جوڑو_sub_replace(nodep, جھوٹ, جھوٹ); }
    خالی دورہ(AstDivS* nodep) override { دورہ_جوڑو_sub_replace(nodep, جھوٹ, سچ); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    خالی دورہ(AstNegate* nodep) override { دورہ_negate_not(nodep, سچ); }
    // Unary never real
    خالی دورہ(AstNot* nodep) override { دورہ_negate_not(nodep, جھوٹ); }

    // Real: inputs and output real
    خالی دورہ(Ast_جوڑو_D* nodep) override { دورہ_real_جوڑو_sub(nodep); }
    خالی دورہ(AstSubD* nodep) override { دورہ_real_جوڑو_sub(nodep); }
    خالی دورہ(AstDivD* nodep) override { دورہ_real_جوڑو_sub(nodep); }
    خالی دورہ(AstMulD* nodep) override { دورہ_real_جوڑو_sub(nodep); }
    خالی دورہ(AstPowD* nodep) override { دورہ_real_جوڑو_sub(nodep); }
    خالی دورہ(Ast_گرہ_System_Biop_D* nodep) override { دورہ_real_جوڑو_sub(nodep); }
    // Real: Output real
    خالی دورہ(AstNegateD* nodep) override { دورہ_real_neg_ceil(nodep); }
    خالی دورہ(Ast_گرہ_System_Uniop_D* nodep) override { دورہ_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    خالی دورہ(AstSigned* nodep) override { دورہ_signed_unsigned(nodep, VSigning::SIGNED); }
    خالی دورہ(AstUnsigned* nodep) override { دورہ_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    خالی دورہ(Ast_سرکاؤ_L* nodep) override { دورہ_سرکاؤ_2(nodep); }
    خالی دورہ(Ast_سرکاؤ_R* nodep) override { دورہ_سرکاؤ_2(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    خالی دورہ(Ast_سرکاؤ_RS* nodep) override { دورہ_سرکاؤ_2(nodep); }

    //========
    // Widths: Output real, input integer signed
    خالی دورہ(Ast_بٹ_مقصد_Real_D* nodep) override { دورہ_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    خالی دورہ(AstRToIS* nodep) override { دورہ_Os32_Lr(nodep); }
    خالی دورہ(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->چوڑائی_Sized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    خالی دورہ(Ast_Real_مقصد_بٹ* nodep) override { دورہ_Ou64_Lr(nodep); }

    // Output integer, input string
    خالی دورہ(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سیٹ_Int();
        }
    }
    خالی دورہ(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        اگر (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_سیٹ_String();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    خالی دورہ(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        اگر (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Sized(8, VSigning::UNSIGNED);
        }
    }
    خالی دورہ(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        اگر (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Sized(8, VSigning::UNSIGNED);
        }
    }
    خالی دورہ(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        اگر (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_سیٹ_String();
        }
    }
    خالی دورہ(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سیٹ_Int();
        }
    }
    خالی دورہ(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            اگر (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtype_سیٹ_Double();
            } ورنہ {
                nodep->dtype_سیٹ_Integer();
            }
        }
    }
    خالی دورہ(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    خالی دورہ(Ast_وقت* nodep) override { nodep->dtype_سیٹ_Time2_State(); }
    خالی دورہ(Ast_وقت_D* nodep) override { nodep->dtype_سیٹ_Double(); }
    خالی دورہ(Ast_وقت_Precision* nodep) override { nodep->dtype_سیٹ_Integer2_State(); }
    خالی دورہ(Ast_Get_Initial_Random_بیج* nodep) override { nodep->dtype_سیٹ_Int(); }
    خالی دورہ(Ast_وقت_Unit* nodep) override {
        nodep->replaceWith(
            نیا AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().power_کا_Ten()});
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstScopeName* nodep) override {
        nodep->dtype_سیٹ_UInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    ساکن منطقی is_Override_نمونہ(ثابت Ast_گرہ* nodep) {
        اگر (ثابت Ast_نمونہ* ثابت patternp = VN_CAST(nodep, نمونہ)) {
            واپس !patternp->بچہ_DTypep();
        }
        ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond);
        واپس condp && is_Override_نمونہ(condp->thenp()) && is_Override_نمونہ(condp->elsep());
    }
    خالی دورہ(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_اظہاریہ(nodep);
        اگر (m_اندر_Param_Override) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ثابت صحیح errors = V3Error::error_گنتی();
            {
                VL_RESTORER(m_اندر_Param_Override);
                m_اندر_Param_Override = جھوٹ;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const);
            ثابت منطقی معلوم = constp && !constp->عدد().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            اگر (!معلوم && V3Error::error_گنتی() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_گرہ_اظہاریہ* ثابت keepp
                = (معلوم && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlink_Fr_عقب();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            userIterate(keepp, چوڑائی_VP{SELF, BOTH}.p());
            واپس;
        }
        اگر (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            user_Iterate_And_اگلا(nodep->thenp(), چوڑائی_VP{m_vup->dtypeNullp(), PRELIM}.p());
            user_Iterate_And_اگلا(nodep->elsep(), چوڑائی_VP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_گرہ_DType* ثابت thenDTypep = nodep->thenp()->dtypep();
            Ast_گرہ_DType* ثابت elseDTypep = nodep->elsep()->dtypep();
            Ast_گرہ_DType* ثابت thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_گرہ_DType* ثابت elseDTypeSkippedp = elseDTypep->skipRefp();
            اگر (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->ڈھونڈو_Null_DType());
            } ورنہ اگر (nodep->thenp()->is_Class_Handle_قدر()
                       || nodep->elsep()->is_Class_Handle_قدر()) {
                Ast_گرہ_DType* commonClassTypep = خالی_اشاریہ;
                اگر (nodep->thenp()->is_Class_Handle_قدر() && nodep->elsep()->is_Class_Handle_قدر()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_گرہ::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                اگر (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } ورنہ {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } ورنہ اگر (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                اگر (Ast_گرہ::computeCastable(elseDTypep, thenDTypep, خالی_اشاریہ).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } ورنہ {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    اگر (!Ast_گرہ::computeCastable(thenDTypep, elseDTypep, خالی_اشاریہ)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } ورنہ اگر (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtype_سیٹ_Double();
            } ورنہ اگر (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtype_سیٹ_String();
            } ورنہ {
                ثابت منطقی is_Any_نتیجہ_Fourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                منطقی handled = جھوٹ;
                ثابت منطقی isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_گرہ_DType* ثابت thenEnumDTypep = thenDTypep->skip_Ref_مقصد_Enump();
                اگر (VN_IS(thenDTypeSkippedp, گرہ_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_درخت(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = سچ;
                } ورنہ اگر (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_درخت(elseDTypep->skip_Ref_مقصد_Enump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    اگر (is_Any_نتیجہ_Fourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = سچ;
                    } ورنہ {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(نیا File_سطر{nodep->fileline()});
                        nodep->fileline()->warnOff(V3_Error_رمز::ENUMVALUE, سچ);
                    }
                }
                اگر (!handled) {
                    ثابت صحیح چوڑائی = std::max(thenDTypep->چوڑائی(), elseDTypep->چوڑائی());
                    ثابت صحیح چوڑائی_Min = std::max(thenDTypep->چوڑائی_Min(), elseDTypep->چوڑائی_Min());
                    ثابت VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ثابت منطقی isFourstate = is_Any_نتیجہ_Fourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    اگر ((چوڑائی == elseDTypeSkippedp->چوڑائی()
                         && چوڑائی_Min == elseDTypeSkippedp->چوڑائی_Min())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } ورنہ اگر ((چوڑائی == thenDTypeSkippedp->چوڑائی()
                                && چوڑائی_Min == thenDTypeSkippedp->چوڑائی_Min())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } ورنہ {
                        // None of the children dtypes match generate one
                        nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(چوڑائی, چوڑائی_Min, numeric, isFourstate);
                    }
                }
            }
        }
        اگر (m_vup->final()) {
            Ast_گرہ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرہ_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    خالی دورہ(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_گرہ_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, چوڑائی_VP{SELF, BOTH}.p());
        // Conversions
        اگر (Ast_گرہ_اظہاریہ* ثابت newp = new_Cons_Dyn_Array_Or_قطار(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{vdtypep, PRELIM}.p());
            user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            جوڑو_Cons_شے(newp, nodep->lhsp()->unlink_Fr_عقب());
            جوڑو_Cons_شے(newp, nodep->rhsp()->unlink_Fr_عقب());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        اگر (VN_IS(vdtypep, UnpackArrayDType)) {
            Ast_نمونہ* ثابت newp = نیا Ast_نمونہ{nodep->fileline(), خالی_اشاریہ};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            واپس;
        }

        // Concat handling
        اگر (m_vup->prelim()) {
            اگر (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, قطار_DType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            اگر (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtype_سیٹ_String();
            } ورنہ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                اگر (m_دھارا_Concat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(
                    nodep->lhsp()->چوڑائی() + nodep->rhsp()->چوڑائی(),
                    nodep->lhsp()->چوڑائی_Min() + nodep->rhsp()->چوڑائی_Min(), VSigning::UNSIGNED,
                    Ast_گرہ_اظہاریہ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            اگر (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->lhsp(), Replicate)) {
                اگر (repp->چوڑائی() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlink_Fr_عقب());
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
            }
            اگر (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->rhsp(), Replicate)) {
                اگر (repp->چوڑائی() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlink_Fr_عقب());
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
            }
        }
        اگر (m_vup->final()) {
            اگر (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_گرہ_اظہاریہ* lhsp = nodep->lhsp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* rhsp = nodep->rhsp()->unlink_Fr_عقب();
                اگر (!lhsp->isString()) lhsp = نیا AstCvtPackString{lhsp->fileline(), lhsp};
                اگر (!rhsp->isString()) rhsp = نیا AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_گرہ* ثابت newp = نیا AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            }
            اگر (!nodep->dtypep()->چوڑائی_Sized()) {
                // See also error in V3Number
                گرہ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    خالی دورہ(AstConcatN* nodep) override {
        اگر (nodep->did_چوڑائی()) واپس;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سیٹ_String();
        }
        اگر (m_vup->final()) {
            nodep->did_چوڑائی(سچ);
            اگر (!nodep->dtypep()->چوڑائی_Sized()) {
                // See also error in V3Number
                گرہ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    خالی دورہ(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    خالی دورہ(AstDelay* nodep) override {
        اگر (nodep->is_چکر_Delay() && m_underSExpr) {
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
            اگر (nodep->rhsp() && !nodep->isUnbounded()) {
                user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            اگر (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                واپس;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::check_Sva_چکر_Delay(nodep, nodep);
            واپس;
        }
        اگر (Ast_گرہ_اظہاریہ* ثابت fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            واپس;
        }
        اگر (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        اگر (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        اگر (nodep->fileline()->timingOn()) {
            اگر (v3Global.opt.timing().is_سیٹ_True()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterate_And_اگلا_Null(nodep->stmtsp());
                واپس;
            } ورنہ اگر (v3Global.opt.timing().is_سیٹ_False()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } ورنہ {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        اگر (nodep->stmtsp()) nodep->جوڑو_اگلا_یہاں(nodep->stmtsp()->unlink_Fr_عقب_With_اگلا());
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    خالی دورہ(AstFork* nodep) override {
        اگر (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        اگر (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().is_سیٹ_True()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ثابت newp = نیا AstBegin{nodep->fileline(), nodep->name(), خالی_اشاریہ, جھوٹ};
            nodep->replaceWith(newp);
            اگر (nodep->declsp()) newp->جوڑو_Declsp(nodep->declsp()->unlink_Fr_عقب_With_اگلا());
            اگر (nodep->stmtsp()) newp->جوڑو_Stmtsp(nodep->stmtsp()->unlink_Fr_عقب_With_اگلا());
            اگر (nodep->forksp()) newp->جوڑو_Stmtsp(nodep->forksp()->unlink_Fr_عقب_With_اگلا());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ اگر (v3Global.opt.timing().is_سیٹ_True()) {
            VL_RESTORER(m_underFork);
            m_underFork = سچ;
            iterateChildren(nodep);
        } ورنہ اگر (v3Global.opt.timing().is_سیٹ_False()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        } ورنہ {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    خالی دورہ_Wait_Or_Disable_Fork(Ast_گرہ* nodep) {
        اگر (nodep->fileline()->timingOn()) {
            اگر (v3Global.opt.timing().is_سیٹ_False()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
                واپس;
            } ورنہ اگر (!v3Global.opt.timing().is_سیٹ_True()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstDisableFork* nodep) override { دورہ_Wait_Or_Disable_Fork(nodep); }
    خالی دورہ(AstWaitFork* nodep) override { دورہ_Wait_Or_Disable_Fork(nodep); }
    خالی دورہ(Ast_مقصد_زیریں_N* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سیٹ_String();
        }
    }
    خالی دورہ(Ast_مقصد_بالائی_N* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سیٹ_String();
        }
    }
    خالی دورہ(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
            اگر (constp) {
                اگر (constp->عدد().isFourState()
                    || (constp->dtypep()->isSigned() && constp->عدد().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } ورنہ {
                    times = constp->مقصد_SInt();
                }
            }

            Ast_گرہ_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
            اگر (VN_IS(vdtypep, قطار_DType) || VN_IS(vdtypep, DynArrayDType)
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
                    user_Iterate_And_اگلا(nodep->srcp(), چوڑائی_VP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlink_Fr_عقب());
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                } ورنہ {  // int a[] = {lhs} -> same as '{lhs}
                    Ast_نمونہ* ثابت newp = نیا Ast_نمونہ{
                        nodep->fileline(),
                        نیا AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlink_Fr_عقب(),
                                         خالی_اشاریہ, خالی_اشاریہ}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    واپس;
                }
            }
            اگر (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            اگر (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } ورنہ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            اگر ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_گرہ* ثابت newp
                    = نیا AstReplicateN{nodep->fileline(), nodep->srcp()->unlink_Fr_عقب(),
                                        nodep->countp()->unlink_Fr_عقب()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            } ورنہ {
                اگر (!constp) nodep->v3error("Replication value isn't a constant.");
                اگر (times == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    times = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(
                    (nodep->srcp()->چوڑائی() * times), (nodep->srcp()->چوڑائی_Min() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        اگر (m_vup->final()) {
            اگر (!nodep->dtypep()->چوڑائی_Sized()) {
                // See also error in V3Number
                گرہ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    خالی دورہ(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtype_سیٹ_String();
        }
        اگر (m_vup->final()) {
            اگر (!nodep->dtypep()->چوڑائی_Sized()) {
                // See also error in V3Number
                گرہ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    خالی دورہ(Ast_گرہ_Dist_Biop* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    خالی دورہ(Ast_گرہ_Dist_Triop* nodep) override {
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    منطقی دھارا_Implicit_Use_Allowed(ثابت Ast_گرہ_دھارا* nodep) ثابت {
        اگر (m_دھارا_Concat) واپس سچ;
        اگر (m_vup && m_vup->دھارا_Use_2() != دھارا_USE_NONE) واپس سچ;

        ثابت Ast_گرہ* ثابت backp = nodep->backp();
        اگر (!backp) واپس سچ;  // Error elsewhere

        واپس VN_IS(backp, گرہ_تفویض);
    }
    خالی دورہ(Ast_گرہ_دھارا* nodep) override {
        VL_RESTORER(m_دھارا_Concat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            m_دھارا_Concat = سچ;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_دھارا_Concat = جھوٹ;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const)) {
                اگر (constp->مقصد_UInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } ورنہ {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ثابت Ast_گرہ_DType* ثابت lhsDtypep = nodep->lhsp()->dtypep()->skip_Ref_مقصد_Enump();
            اگر (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, قطار_DType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, گرہ_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, گرہ_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtype_سیٹ_دھارا();
            } ورنہ اگر (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtype_سیٹ_دھارا();
            } ورنہ {
                nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(
                    nodep->lhsp()->چوڑائی(), nodep->lhsp()->چوڑائی_Min(), VSigning::UNSIGNED,
                    Ast_گرہ_اظہاریہ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        اگر (m_vup->final()) {
            اگر (!دھارا_Implicit_Use_Allowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warn_مزید() << "... Suggest use a cast");
            }
            اگر (!nodep->dtypep()->چوڑائی_Sized()) {
                // See also error in V3Number
                گرہ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    خالی دورہ(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), سچ,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), سچ,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        اگر (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ثابت صحیح چوڑائی = nodep->elementsConst();
            اگر (nodep->fromBracket() && nodep->بایاں_Const() > nodep->دایاں_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-چوڑائی + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } ورنہ اگر (چوڑائی > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << چوڑائی << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ثابت منطقی اندر_Dead_Module = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ثابت منطقی اندر_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ثابت منطقی اندر_Type_جدول = !m_modep;
            ثابت Ast_گرہ* basep = nodep->backp();
            جب (VN_IS(basep, Range)) basep = basep->backp();
            اگر (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !اندر_Dead_Module && !اندر_Parameterized_Template && !اندر_Type_جدول) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->بایاں_Const() << ":" << nodep->دایاں_Const()
                                            << "]");
            }
        }
    }

    خالی دورہ(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        اگر (nodep->did_چوڑائی()) واپس;
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_اگلا(nodep->lsbp(), چوڑائی_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), جھوٹ);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            صحیح چوڑائی = nodep->چوڑائی_Const();
            اگر (چوڑائی <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtype_سیٹ_ثنائی_ہندسہ();
                واپس;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ثابت منطقی isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ثابت منطقی اندر_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            اگر (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                چوڑائی = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Sized(چوڑائی, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->چوڑائی_Const(چوڑائی);
                ڈالو_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(نیا AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            اگر (nodep->fromp()->چوڑائی() < چوڑائی && !اندر_Parameterized_Template) {
                // If it is not a lvalue, extend it
                اگر (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << چوڑائی << " bits from only "
                                                          << nodep->fromp()->چوڑائی()
                                                          << " bit number");
                    Ast_گرہ_DType* ثابت subDTypep = nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                        چوڑائی, چوڑائی, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    چوڑائی_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    جھوٹ /*noerror*/);
                } ورنہ {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << چوڑائی << " bits to only "
                                                         << nodep->fromp()->چوڑائی()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            صحیح frommsb = nodep->fromp()->چوڑائی() - 1;
            صحیح fromlsb = 0;
            ثابت صحیح elw = nodep->decl_El_چوڑائی();  // Must adjust to tell user bit ranges
            اگر (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } ورنہ {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ثابت صحیح selwidth = V3_نمبر::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_گرہ_DType* ثابت selwidthDTypep = nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, FINAL}.p());
            user_Iterate_And_اگلا(nodep->lsbp(), چوڑائی_VP{SELF, FINAL}.p());
            اگر (چوڑائی_Bad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->چوڑائی() != 32) {
                اگر (!nodep->fileline()->warnIsOff(V3_Error_رمز::چوڑائی_2)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->چوڑائی() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->چوڑائی() / elw)
                            << (nodep->lsbp()->چوڑائی() != nodep->lsbp()->چوڑائی_Min()
                                    ? " or " + cvt_مقصد_Str(nodep->lsbp()->چوڑائی_Min() / elw)
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
                } ورنہ اگر (!اندر_Parameterized_Template) {
                    اگر (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } ورنہ {
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
                    ثابت صحیح extend_مقصد = nodep->msbConst() + 1;
                    Ast_گرہ_DType* ثابت subDTypep = nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                        extend_مقصد, extend_مقصد, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    چوڑائی_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    جھوٹ /*noerror*/);
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
                چوڑائی_Check_Sized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                جھوٹ /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    خالی دورہ(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            اگر (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->bitp(), Const)) {
                    ثابت صحیح ثنائی_ہندسہ_2 = constp->مقصد_SInt();
                    Ast_گرہ_اظہاریہ* newp = newIfaceArrayElement(nodep->fromp(), ثنائی_ہندسہ_2);
                    اگر (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ثابت AstUnpackArrayDType* ثابت arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << ثنائی_ہندسہ_2 + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
            }
            //
            صحیح frommsb;
            صحیح fromlsb;
            ثابت Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                اگر (fromlsb > frommsb) std::تبادلہ(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                اگر (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } ورنہ {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ثابت صحیح selwidth = V3_نمبر::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_گرہ_DType* ثابت selwidthDTypep = nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            اگر (چوڑائی_Bad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->چوڑائی() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->چوڑائی(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->چوڑائی()
                                       << (nodep->bitp()->چوڑائی() != nodep->bitp()->چوڑائی_Min()
                                               ? " or " + cvt_مقصد_Str(nodep->bitp()->چوڑائی_Min())
                                               : "")
                                       << " bits.");
                اگر (!nodep->fileline()->warnIsOff(V3_Error_رمز::چوڑائی_2)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            اگر (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                اگر (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->مقصد_SInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->مقصد_SInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    اگر (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->مقصد_SInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                چوڑائی_Check_Sized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                جھوٹ /*NOWARN*/);
            }
        }
    }

    خالی دورہ(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            ثابت Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
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

    خالی دورہ(AstAlias* nodep) override {
        اگر (!nodep->did_چوڑائی_And_سیٹ()) {
            user_Iterate_And_اگلا(nodep->itemsp(), چوڑائی_VP{SELF, BOTH}.p());
        }

        ثابت خودکار check_If_اظہاریہ_Ok = [یہ](ثابت Ast_گرہ_اظہاریہ* ثابت exprp) {
            اگر (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                واپس جھوٹ;
            }

            ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef);
            اگر (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                واپس جھوٹ;
            }
            AstVar* ثابت varp = varRefp->varp();
            اگر (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                واپس جھوٹ;
            }
            اگر (m_aliasedVars.ڈھونڈو(varp) != m_aliasedVars.انتہا()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                واپس جھوٹ;
            } ورنہ {
                m_aliasedVars.داخل_کرو(varp);
            }
            واپس سچ;
        };

        check_If_اظہاریہ_Ok(nodep->itemsp());
        Ast_گرہ_DType* ثابت پہلا_شے_Dtypep = nodep->itemsp()->dtypep();
        برائے (Ast_گرہ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_اظہاریہ_Ok(VN_AS(itemp, گرہ_اظہاریہ));
            اگر (!پہلا_شے_Dtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << پہلا_شے_Dtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    خالی دورہ(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            ثابت Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
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
                              adtypep->ڈھونڈو_String_DType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    خالی دورہ(Ast_قاش_Sel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ثابت Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            اگر (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_گرہ_DType* ثابت newDtp
                = نیا AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          نیا AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(newDtp);
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
                    nodep->replaceWith(نیا AstConsArray{
                        nodep->fileline(), newDtp,
                        نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}}});
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                } ورنہ اگر ((declRange.ascending() != adtypep->declRange().ascending())
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
                AstConsArray* ثابت newp = نیا AstConsArray{nodep->fileline(), newDtp};
                ثابت VNumRange range = nodep->declRange();
                برائے (صحیح n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_درخت(جھوٹ));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            }
        }
    }

    خالی دورہ(Ast_Sel_ثنائی_ہندسہ* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->bitp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->thsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->attrp(), چوڑائی_VP{SELF, BOTH}.p());
        Ast_گرہ* ثابت selp = V3_چوڑائی::چوڑائی_Sel_No_Iter_Edit(nodep);
        اگر (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            واپس;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    خالی دورہ(Ast_Sel_نکالو* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->leftp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->rightp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->attrp(), چوڑائی_VP{SELF, BOTH}.p());
        Ast_گرہ* ثابت selp = V3_چوڑائی::چوڑائی_Sel_No_Iter_Edit(nodep);
        اگر (selp != nodep) {
            nodep = خالی_اشاریہ;
            userIterate(selp, m_vup);
            واپس;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    خالی دورہ(AstSelPlus* nodep) override {
        user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->bitp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->widthp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->attrp(), چوڑائی_VP{SELF, BOTH}.p());
        Ast_گرہ* ثابت selp = V3_چوڑائی::چوڑائی_Sel_No_Iter_Edit(nodep);
        اگر (selp != nodep) {
            nodep = خالی_اشاریہ;
            userIterate(selp, m_vup);
            واپس;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    خالی دورہ(AstSelMinus* nodep) override {
        user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->bitp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->widthp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_اگلا(nodep->attrp(), چوڑائی_VP{SELF, BOTH}.p());
        Ast_گرہ* ثابت selp = V3_چوڑائی::چوڑائی_Sel_No_Iter_Edit(nodep);
        اگر (selp != nodep) {
            nodep = خالی_اشاریہ;
            userIterate(selp, m_vup);
            واپس;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    خالی دورہ(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        اگر (m_vup && m_vup->prelim()) {
            اگر (VN_IS(nodep->dtypep()->skip_Ref_مقصد_Enump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } ورنہ اگر (nodep->عدد().isNull()) {
                nodep->dtypep(nodep->ڈھونڈو_Null_DType());
            } ورنہ اگر (nodep->عدد().isString()) {
                nodep->dtype_سیٹ_String();
            } ورنہ اگر (nodep->عدد().isDouble()) {
                nodep->dtype_Chg_چوڑائی(nodep->عدد().چوڑائی(), nodep->عدد().sized()
                                                               ? nodep->عدد().چوڑائی()
                                                               : nodep->عدد().چوڑائی_مقصد_Fit());
            } ورنہ {
                nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(
                    nodep->عدد().چوڑائی(),
                    nodep->عدد().sized() ? nodep->عدد().چوڑائی() : nodep->عدد().چوڑائی_مقصد_Fit(),
                    nodep->dtypep()->numeric(), nodep->عدد().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    خالی دورہ(Ast_خالی_خانے_قطار* nodep) override {
        nodep->dtype_سیٹ_خالی_خانے_قطار();
        اگر (!VN_IS(nodep->backp(), تفویض) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    خالی دورہ(AstFell* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ سچ);
            userIterate(nodep->sentreep(), خالی_اشاریہ);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstFalling* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ سچ);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstFuture* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ سچ);
            userIterate(nodep->sentreep(), خالی_اشاریہ);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    خالی دورہ(AstPast* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ سچ);
            nodep->dtypeFrom(nodep->exprp());
            اگر (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ سچ);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ثابت AstConst* ثابت constp = VN_CAST(nodep->ticksp(), Const);
                اگر (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_عقب()->delete_درخت();
                } ورنہ اگر (constp->مقصد_SInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_عقب()->delete_درخت();
                } ورنہ اگر (V3AssertCommon::check_Sva_قدر_حد_بندی(nodep, constp, "$past tick value")) {
                    اگر (constp->مقصد_SInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->مقصد_SInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), خالی_اشاریہ);
        }
    }
    خالی دورہ(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{SELF, BOTH}.p());
            user_Iterate_And_اگلا(nodep->countp(), چوڑائی_VP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
            اگر (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } ورنہ اگر (minConstp->مقصد_SInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } ورنہ اگر (!nodep->unbounded() && !nodep->maxCountp() && minConstp->مقصد_SInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            اگر (nodep->maxCountp()) {
                user_Iterate_And_اگلا(nodep->maxCountp(), چوڑائی_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
                اگر (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } ورنہ اگر (minConstp && maxConstp->مقصد_SInt() < minConstp->مقصد_SInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } ورنہ اگر (maxConstp->مقصد_SInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->propp(), چوڑائی_VP{SELF, BOTH}.p());
            Ast_گرہ_اظہاریہ* propp = nodep->propp();
            اگر (!VN_IS(nodep->loBoundp(), Unbounded)) {
                user_Iterate_And_اگلا(nodep->loBoundp(), چوڑائی_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            اگر (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                user_Iterate_And_اگلا(nodep->hiBoundp(), چوڑائی_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ثابت منطقی loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ثابت منطقی hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            اگر (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_گرہ* ثابت boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtype_سیٹ_ثنائی_ہندسہ();
                واپس;
            }
            اگر (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtype_سیٹ_ثنائی_ہندسہ();
                واپس;
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
            اگر (loConstp && loConstp->مقصد_SInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } ورنہ اگر (loConstp) {
                V3AssertCommon::check_Sva_قدر_حد_بندی(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            اگر (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_قدر_حد_بندی(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            اگر (!hiUnbounded && loConstp && hiConstp && hiConstp->مقصد_SInt() < loConstp->مقصد_SInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            منطقی hasPropertyOp = propp->is_Multi_چکر_Sva();
            اگر (!hasPropertyOp) {
                propp->foreach([&](ثابت Ast_گرہ* np) {
                    اگر (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = سچ;
                    }
                });
            }
            اگر (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fix_چوڑائی_Reduce(propp), propp);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstRising* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ سچ);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstRose* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ سچ);
            userIterate(nodep->sentreep(), خالی_اشاریہ);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }

    خالی دورہ(AstSampled* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ سچ);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    خالی دورہ(AstSetuphold* nodep) override {
        File_سطر* ثابت flp = nodep->fileline();
        Ast_تفویض_W* newp = خالی_اشاریہ;
        اگر (nodep->delrefp()) {
            newp = convert_Setuphold_مقصد_تفویض(flp, nodep->refevp(), nodep->delrefp());
        }
        اگر (nodep->deldatap()) {
            اگر (!newp) {
                newp = convert_Setuphold_مقصد_تفویض(flp, nodep->dataevp(), nodep->deldatap());
            } ورنہ {
                newp->جوڑو_اگلا_یہاں(
                    convert_Setuphold_مقصد_تفویض(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        اگر (!newp) {
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        nodep->replaceWith(نیا AstAlways{newp});
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    Ast_تفویض_W* convert_Setuphold_مقصد_تفویض(File_سطر* ثابت flp, Ast_گرہ_اظہاریہ* ثابت evp,
                                         Ast_گرہ_اظہاریہ* ثابت delp) {
        Ast_گرہ_اظہاریہ* ثابت lhsp = delp->clone_درخت_خالص(جھوٹ);
        Ast_گرہ_اظہاریہ* ثابت rhsp = evp->clone_درخت_خالص(جھوٹ);
        UASSERT_OBJ(VN_IS(lhsp, گرہ_Var_Ref) || VN_IS(lhsp, گرہ_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        اگر (Ast_گرہ_Var_Ref* varRefp = VN_CAST(lhsp, گرہ_Var_Ref)) {
            اگر (varRefp->varp()->direction() == VDirection::داخلہ) {
                VL_DO_DANGLING(ڈالو_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ڈالو_Deletep(rhsp), rhsp);
                واپس خالی_اشاریہ;
            }
            varRefp->access(VAccess::WRITE);
        }
        اگر (Ast_گرہ_Pre_Sel* selp = VN_CAST(lhsp, گرہ_Pre_Sel)) {
            اگر (Ast_گرہ_Var_Ref* varRefp = VN_CAST(selp->fromp(), گرہ_Var_Ref)) {
                اگر (varRefp->varp()->direction() == VDirection::داخلہ) {
                    VL_DO_DANGLING(ڈالو_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ڈالو_Deletep(rhsp), rhsp);
                    واپس خالی_اشاریہ;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        واپس نیا Ast_تفویض_W{flp, lhsp, rhsp};
    }

    خالی دورہ(AstStable* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ سچ);
            userIterate(nodep->sentreep(), خالی_اشاریہ);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstSteady* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ سچ);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }

    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{SELF, BOTH}.p());
    }

    خالی دورہ(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }

    خالی دورہ(AstUntil* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            اگر (nodep->lhsp()->چوڑائی() != 1) fix_چوڑائی_Reduce(nodep->lhsp());
            اگر (nodep->rhsp()->چوڑائی() != 1) fix_چوڑائی_Reduce(nodep->rhsp());
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstAbortOn* nodep) override { دورہ_Abort_Prop(nodep); }

    خالی دورہ(AstRand* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            اگر (nodep->urandom()) {
                nodep->dtype_سیٹ_UInt32();  // Says the spec
            } ورنہ {
                nodep->dtype_سیٹ_Int();  // Says the spec
            }
            اگر (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    خالی دورہ(AstSEventually* nodep) override {
        اگر (v3Global.opt.timing().is_سیٹ_False() || !v3Global.opt.timing().is_سیٹ_True()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::Widthed_قدر{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            واپس;
        }
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstSGotoRep* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_اگلا(nodep->countp(), چوڑائی_VP{SELF, BOTH}.p());
            اگر (nodep->maxCountp()) {
                چوڑائی_Check_Goto_Rep_Range(nodep, "Goto");
            } ورنہ {
                چوڑائی_Check_Rep_گنتی(nodep);
            }
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstSNonConsRep* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_اگلا(nodep->countp(), چوڑائی_VP{SELF, BOTH}.p());
            اگر (nodep->maxCountp()) {
                چوڑائی_Check_Goto_Rep_Range(nodep, "Nonconsecutive");
            } ورنہ {
                چوڑائی_Check_Rep_گنتی(nodep);
            }
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    سانچہ <قسم_نام T_Rep>
    خالی چوڑائی_Check_Rep_گنتی(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
        اگر (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } ورنہ اگر (constp->dtypep()->isSigned() && constp->عدد().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } ورنہ اگر (constp->عدد().most_سیٹ_ثنائی_ہندسہ_P1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } ورنہ اگر (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    سانچہ <قسم_نام T_Rep>
    خالی چوڑائی_Check_Goto_Rep_Range(T_Rep* nodep, ثابت حرف* قسم) {
        user_Iterate_And_اگلا(nodep->maxCountp(), چوڑائی_VP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
        ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
        اگر (!minConstp || !maxConstp) {
            nodep->v3error(std::string{قسم}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            واپس;
        }
        اگر (minConstp->مقصد_SInt() < 0) {
            nodep->v3error(std::string{قسم}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            واپس;
        }
        اگر (maxConstp->مقصد_SInt() < minConstp->مقصد_SInt()) {
            nodep->v3error(std::string{قسم}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            واپس;
        }
        اگر (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + قسم
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    خالی دورہ(AstSThroughout* nodep) override {
        m_hasSExpr = سچ;
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = جھوٹ;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = سچ;
            iterate(nodep->rhsp());
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = سچ;
        m_hasSExpr = سچ;
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            اگر (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = سچ;
        m_hasSExpr = سچ;
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            برائے (Ast_Sen_شے* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), Sen_شے)) {
                user_Iterate_And_اگلا(senp->sensp(), چوڑائی_VP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstURandomRange* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            nodep->dtype_سیٹ_UInt32();  // Says the spec
            Ast_گرہ_DType* ثابت expDTypep = nodep->ڈھونڈو_UInt32_DType();
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    خالی دورہ(AstUnbounded* nodep) override {
        nodep->dtype_سیٹ_Int();  // Used in int context
        اگر (VN_IS(nodep->backp(), IsUnbounded)) واپس;  // Ok, leave
        اگر (VN_IS(nodep->backp(), BracketArrayDType)) واپس;  // Ok, leave
        اگر (VN_IS(nodep->backp(), InsideRange)) واپس;  // Ok, leave
        اگر (ثابت خودکار* ثابت varp = VN_CAST(nodep->backp(), Var)) {
            اگر (varp->isParam()) واپس;  // Ok, leave
        }
        Ast_گرہ* backp = nodep->backp();
        اگر (VN_IS(backp, Sub)) backp = backp->backp();
        اگر (ثابت خودکار* ثابت selp = VN_CAST(backp, Sel_نکالو)) {
            اگر (VN_IS(selp->fromp()->dtypep()->skipRefp(), قطار_DType)) واپس;
        }
        اگر (ثابت خودکار* ثابت selp = VN_CAST(backp, Sel_ثنائی_ہندسہ)) {
            اگر (VN_IS(selp->fromp()->dtypep()->skipRefp(), قطار_DType)) واپس;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    خالی دورہ(AstInferredDisable* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) nodep->dtype_سیٹ_ثنائی_ہندسہ();
    }
    خالی دورہ(AstIsUnbounded* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        اگر (m_vup && m_vup->prelim()) {
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Unsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
        }
        اگر (m_vup->final()) {
            Ast_گرہ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            اگر (nodep->چوڑائی() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    خالی دورہ(AstCLog2* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    سانچہ <قسم_نام T_Option>
    خالی چوڑائی_Cover_وزن(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_گرہ_اظہاریہ* ثابت valuep = nodep->valuep();
        ثابت std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            اگر (!constp->عدد().isNegative()) واپس;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->مقصد_SInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } ورنہ اگر (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } ورنہ {
            واپس;
        }
        // Continue with the default weight
        valuep->replaceWith(نیا AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    سانچہ <قسم_نام T_Option>
    خالی چوڑائی_Cover_Int(T_Option* nodep) {
        user_Iterate_And_اگلا(nodep->valuep(), چوڑائی_VP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    خالی دورہ(Ast_Cg_Option_تفویض* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ثابت cgClassp = m_containingClassFinder.ڈھونڈو(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        اگر (nodep->optType() == VCoverOptionType::وزن) {
            چوڑائی_Cover_وزن(nodep);
            واپس;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        اگر (nodep->optType() == VCoverOptionType::AT_کم_سے_کم) {
            چوڑائی_Cover_Int(nodep);
            واپس;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        اگر (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            اگر (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->مقصد_SInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->مقصد_SInt() << سطر_ختم);
            } ورنہ {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } ورنہ اگر (nodep->optType() == VCoverOptionType::ملاؤ_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_گرہ_اظہاریہ* ثابت valuep = nodep->valuep();
            اگر (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(نیا AstConst{valuep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
                VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
            }
            واپس;
        } ورنہ اگر (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            واپس;
        } ورنہ اگر (nodep->optType() == VCoverOptionType::DISTRIBUTE_پہلا) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ثابت AstConst* ثابت constp = VN_CAST(nodep->valuep(), Const);
            اگر (!constp || !constp->عدد().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstCoverOption* nodep) override {
        اگر (nodep->optType() == VCoverOptionType::وزن) {
            چوڑائی_Cover_وزن(nodep);
            واپس;
        }
        اگر (nodep->optType() == VCoverOptionType::AT_کم_سے_کم
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            چوڑائی_Cover_Int(nodep);
            واپس;
        }
        user_Iterate_And_اگلا(nodep->valuep(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstCoverCross* nodep) override {
        user_Iterate_And_اگلا(nodep->itemsp(), خالی_اشاریہ);
        اگر (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_اگلا(nodep->optionsp(), خالی_اشاریہ);
        user_Iterate_And_اگلا(nodep->binsp(), خالی_اشاریہ);
    }
    خالی دورہ(AstCoverCrossBin* nodep) override {
        user_Iterate_And_اگلا(nodep->selectp(), خالی_اشاریہ);
        اگر (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    خالی دورہ(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        user_Iterate_And_اگلا(nodep->binsp(), چوڑائی_VP{nodep->exprp()->dtypep(), BOTH}.p());
        اگر (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_اگلا(nodep->optionsp(), خالی_اشاریہ);
    }
    خالی چوڑائی_Covergroup_Ranges(Ast_گرہ* rangesp, صحیح fill_چوڑائی) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ثابت خودکار fill = [&](Ast_گرہ* nodep) {
            اگر (!fill_چوڑائی) واپس;
            Ast_گرہ_اظہاریہ* exprp = VN_AS(nodep, گرہ_اظہاریہ);
            fixAutoExtend(exprp /*ref*/, fill_چوڑائی);
        };
        برائے (Ast_گرہ *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (AstInsideRange* ثابت rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, خالی_اشاریہ);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } ورنہ اگر (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } ورنہ {
                itemp = userIterateSubtreeReturnEdits(itemp, چوڑائی_VP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    خالی دورہ(AstCoverBinsof* nodep) override {
        user_Iterate_And_اگلا(nodep->pointp(), خالی_اشاریہ);
        چوڑائی_Covergroup_Ranges(nodep->rangesp(), 0);
    }
    خالی دورہ(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        چوڑائی_Covergroup_Ranges(nodep->rangesp(), m_vup ? m_vup->dtypep()->چوڑائی() : 0);
        اگر (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        اگر (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            user_Iterate_And_اگلا(nodep->arraySizep(), چوڑائی_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        user_Iterate_And_اگلا(nodep->transp(), m_vup);
    }
    خالی دورہ(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        user_Iterate_And_اگلا(nodep->itemp(), خالی_اشاریہ);
        اگر (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        اگر (!VN_IS(nodep->subp(), CoverpointRef)) {
            چوڑائی_Covergroup_Ranges(nodep->subp(), m_vup->dtypep()->چوڑائی());
        }
        چوڑائی_Cover_Filter(nodep->filterp(), "Bin");
    }
    خالی دورہ(AstCoverCrossWith* nodep) override {
        user_Iterate_And_اگلا(nodep->selectp(), خالی_اشاریہ);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        user_Iterate_And_اگلا(nodep->varsp(), خالی_اشاریہ);
        چوڑائی_Cover_Filter(nodep->filterp(), "Cross bin");
        اگر (Ast_گرہ_اظہاریہ* ثابت countp = nodep->countp()) {
            اگر (VN_IS(countp, Unbounded)) {
                countp->dtype_سیٹ_Int();  // 'matches $'
            } ورنہ {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                user_Iterate_And_اگلا(countp, چوڑائی_VP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    خالی چوڑائی_Cover_Filter(Ast_گرہ_اظہاریہ* filterp, ثابت حرف* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, چوڑائی_VP{SELF, BOTH}.p()), گرہ_اظہاریہ);
        File_سطر* ثابت fl = filterp->fileline();
        ثابت Ast_گرہ_DType* ثابت resultp = filterp->dtypep()->skipRefp();
        اگر (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlink_Fr_عقب(&relinker);
            relinker.relink(
                نیا AstNeqD{fl, filterp, نیا AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } ورنہ اگر (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(نیا AstConst{fl, AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(filterp), filterp);
        } ورنہ {
            fix_چوڑائی_Reduce(filterp);
        }
    }
    خالی دورہ(Ast_Cover_Trans_سیٹ* nodep) override { user_Iterate_And_اگلا(nodep->itemsp(), m_vup); }
    خالی دورہ(Ast_Cover_Trans_شے* nodep) override {
        چوڑائی_Covergroup_Ranges(nodep->valuesp(), m_vup ? m_vup->dtypep()->چوڑائی() : 0);
    }
    خالی دورہ(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            اگر (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                واپس;
            }
            checkCvtUS(nodep->lhsp(), جھوٹ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            اگر (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->عدد().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtype_سیٹ_Logic_Unsized(nodep->چوڑائی(), nodep->چوڑائی_Min(),
                                            nodep->dtypep()->numeric());
            }
        }

        اگر (m_vup->final()) {
            Ast_گرہ_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرہ_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(expDTypep->چوڑائی(), expDTypep->چوڑائی_Min(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_گرہ* newp = خالی_اشاریہ;  // No change
            اگر (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = نیا AstPowSS{nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب(),
                                    nodep->rhsp()->unlink_Fr_عقب()};
            } ورنہ اگر (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = نیا AstPowSU{nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب(),
                                    nodep->rhsp()->unlink_Fr_عقب()};
            } ورنہ اگر (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = نیا AstPowUS{nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب(),
                                    nodep->rhsp()->unlink_Fr_عقب()};
            }
            اگر (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            }
        }
    }
    خالی دورہ(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
        user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
        user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
        user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(Ast_گنتی_بٹ* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحیح چوڑائی_Min = V3_نمبر::log2b(nodep->lhsp()->چوڑائی()) + 1;
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Unsized(32, چوڑائی_Min, VSigning::SIGNED);
        }
    }
    خالی دورہ(Ast_گنتی_Ones* nodep) override {
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحیح چوڑائی_Min = V3_نمبر::log2b(nodep->lhsp()->چوڑائی()) + 1;
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Unsized(32, چوڑائی_Min, VSigning::SIGNED);
        }
    }
    خالی دورہ(AstCvtPackString* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        // Opaque returns, so arbitrary
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH, current_دھارا_Use()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        اگر (Ast_گرہ_DType* ثابت dt = nodep->lhsp()->dtypep()) {
            اگر (VN_IS(dt->skip_Ref_مقصد_Enump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    خالی دورہ(Ast_Cvt_Packed_مقصد_Array* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        // Opaque returns, so arbitrary
        user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, BOTH, current_دھارا_Use()}.p());
        // Type set in constructor
    }
    خالی دورہ(Ast_Cvt_Array_مقصد_Array* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        // Opaque returns, so arbitrary
        user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, BOTH, current_دھارا_Use()}.p());
        // Type set in constructor
    }
    خالی دورہ(Ast_Cvt_Array_مقصد_Packed* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        // Opaque returns, so arbitrary
        user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, BOTH, current_دھارا_Use()}.p());
        // Type set in constructor
    }
    خالی دورہ(Ast_Cvt_Unpack_مقصد_قطار* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        // Opaque returns, so arbitrary
        user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, BOTH}.p());
        // Type set in constructor
    }
    خالی دورہ(Ast_Cvt_Unpack_مقصد_Unpack* nodep) override {
        // Type set when created, the value converted is of its own type
        اگر (m_vup->final()) {
            user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    خالی دورہ(Ast_وقت_Import* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ثابت دوہرا وقت_Prescale = constp->عدد().مقصد_Double();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ثابت دوہرا وقت_2 = وقت_Prescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ثابت newp = نیا AstConst{nodep->fileline(), AstConst::RealDouble{}, وقت_2};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(AstEventControl* nodep) override {
        اگر (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }
        اگر (nodep->fileline()->timingOn()) {
            اگر (v3Global.opt.timing().is_سیٹ_True()) {
                iterateChildren(nodep);
                واپس;
            } ورنہ اگر (v3Global.opt.timing().is_سیٹ_False()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warn_مزید()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } ورنہ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        اگر (nodep->stmtsp()) nodep->جوڑو_اگلا_یہاں(nodep->stmtsp()->unlink_Fr_عقب());
        VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    ساکن string typenameExtends(ثابت AstClass* classp) {
        // An interface class may extend several, so those are not named
        اگر (!classp || classp->isInterfaceClass()) واپس "";
        برائے (ثابت AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            اگر (extendsp->isImplements()) جاری;
            ثابت AstClass* ثابت basep = extendsp->classOrNullp();
            واپس basep ? " extends class " + basep->dtypeName(سچ) : "";
        }
        واپس "";
    }
    خالی دورہ(Ast_Attr_کا* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, BOTH}.p());
        اگر (nodep->dimp()) user_Iterate_And_اگلا(nodep->dimp(), چوڑائی_VP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        انتخاب (nodep->attrType()) {
        حالت VAttrType::DIM_DIMENSIONS:
        حالت VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_گرہ_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ثابت std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(سچ);
            ثابت صحیح val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.دوسرا
                                                                       : (dim.پہلا + dim.دوسرا));
            nodep->replaceWith(نیا AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            توڑو;
        }
        حالت VAttrType::DIM_بٹ_OR_نمبر: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            اگر (VN_IS(nodep->fromp(), گرہ_اظہاریہ)) {
                nodep->replaceWith(nodep->fromp()->unlink_Fr_عقب());
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            } ورنہ {
                Ast_گرہ* newp = نیا Ast_Attr_کا{nodep->fileline(), VAttrType::DIM_بٹ,
                                              nodep->fromp()->unlink_Fr_عقب()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                user_Iterate_And_اگلا(newp, چوڑائی_VP{SELF, BOTH}.p());  // Convert AttrOf
            }
            واپس;
        }
        حالت VAttrType::DIM_بٹ:
        حالت VAttrType::DIM_بلند:
        حالت VAttrType::DIM_INCREMENT:
        حالت VAttrType::DIM_بایاں:
        حالت VAttrType::DIM_کم:
        حالت VAttrType::DIM_دایاں:
        حالت VAttrType::DIM_سائز: {
            Ast_گرہ_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            اگر (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ثابت Ast_گرہ_Module* ثابت ownModp = v3Global.rootp()->containingModule(dtypep);
                اگر (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtype_سیٹ_Int();
                    واپس;
                }
            }

            اگر (VN_IS(dtypep, قطار_DType) || VN_IS(dtypep, DynArrayDType)) {
                انتخاب (nodep->attrType()) {
                حالت VAttrType::DIM_سائز: {
                    Ast_گرہ_اظہاریہ* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_عقب(), گرہ_اظہاریہ);
                    Ast_گرہ* ثابت newp
                        = نیا AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_سائز};
                    newp->dtype_سیٹ_Int();
                    newp->did_چوڑائی(سچ);
                    newp->protect(جھوٹ);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    توڑو;
                }
                حالت VAttrType::DIM_بایاں:
                حالت VAttrType::DIM_کم: {
                    Ast_گرہ* ثابت newp = نیا AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    توڑو;
                }
                حالت VAttrType::DIM_دایاں:
                حالت VAttrType::DIM_بلند: {
                    Ast_گرہ_اظہاریہ* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_عقب(), گرہ_اظہاریہ);
                    Ast_گرہ_اظہاریہ* ثابت sizep
                        = نیا AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_سائز};
                    sizep->dtype_سیٹ_Int();
                    sizep->did_چوڑائی(سچ);
                    sizep->protect(جھوٹ);
                    Ast_گرہ* ثابت newp
                        = نیا AstSub{nodep->fileline(), sizep,
                                     نیا AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    توڑو;
                }
                حالت VAttrType::DIM_INCREMENT: {
                    Ast_گرہ_اظہاریہ* ثابت newp
                        = نیا AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    توڑو;
                }
                حالت VAttrType::DIM_بٹ: {
                    اگر (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } ورنہ {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    توڑو;
                }
                طےشدہ: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } ورنہ {
                ثابت std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(سچ);
                ثابت uint32_t msbdim = dimpair.پہلا + dimpair.دوسرا;
                اگر (!nodep->dimp() || msbdim < 1) {
                    اگر (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_گرہ_اظہاریہ* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_عقب(), گرہ_اظہاریہ);
                        Ast_گرہ* ثابت newp = نیا Ast_سرکاؤ_L{
                            nodep->fileline(), نیا AstLenN{nodep->fileline(), fromp},
                            نیا AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    } ورنہ {
                        Ast_گرہ_DType* ثابت بنیاد_DTypep = dtypep->skipRefp();
                        UASSERT_OBJ(بنیاد_DTypep, nodep, "Unsized expression");
                        ثابت صحیح dim = 1;
                        AstConst* ثابت newp = dimension_قدر(nodep->fileline(), بنیاد_DTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    }
                } ورنہ اگر (VN_IS(nodep->dimp(), Const)) {
                    ثابت صحیح dim = VN_AS(nodep->dimp(), Const)->مقصد_SInt();
                    AstConst* ثابت newp
                        = dimension_قدر(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                } ورنہ {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ثابت tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_گرہ_اظہاریہ* ثابت dimp = nodep->dimp()->unlink_Fr_عقب();
                    Ast_گرہ_اظہاریہ* ثابت newp = نیا AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                }
            }
            توڑو;
        }
        حالت VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            توڑو;
        حالت VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_گرہ_DType* ثابت protoDtp = nodep->fromp()->dtypep();
            Ast_گرہ_DType* ثابت declDtp = [&]() {
                اگر (m_ftaskp->fvarp()) واپس m_ftaskp->fvarp()->dtypep();
                Ast_گرہ_DType* ثابت voidp = نیا AstVoidDType{m_ftaskp->fileline()};
                ڈالو_Deletep(voidp);  // Note voidp used past here
                واپس voidp;
            }();
            اگر (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warn_مزید()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warn_مزید()
                        << "... Declaration data type: " << declDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnContextPrimary() << '\n'
                        << declDtp->warnOther() << "... Location of out-of-block declaration\n"
                        << declDtp->warnContextSecondary());
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            توڑو;
        }
        حالت VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_گرہ_DType* ثابت dtypep = nodep->fromp()->dtypep();
            string نتیجہ = dtypep->prettyDTypeName(سچ);
            اگر (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                نتیجہ += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << نتیجہ << "' from " << dtypep);
            Ast_گرہ* ثابت newp = نیا AstConst{nodep->fileline(), AstConst::String{}, نتیجہ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            توڑو;
        }
        حالت VAttrType::TYPEID:
            اگر (Ast_گرہ_DType* dtypep = VN_CAST(nodep->fromp(), گرہ_DType)) {
                nodep->dtypep(dtypep);
            } ورنہ {
                nodep->dtypep(VN_AS(nodep->fromp(), گرہ_اظہاریہ)->dtypep());
            }
            توڑو;
        حالت VAttrType::VAR_بنیاد:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            توڑو;
        طےشدہ: {
            // Everything else resolved earlier
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Unsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            توڑو;
        }
        }
    }
    خالی دورہ(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(Ast_متن* nodep) override {
        // Only used in CStmts which don't care....
    }

    خالی دورہ(AstCellArrayRef* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        user_Iterate_And_اگلا(nodep->selp(), چوڑائی_VP{SELF, PRELIM}.p());
        user_Iterate_And_اگلا(nodep->selp(), چوڑائی_VP{SELF, FINAL}.p());
        nodep->dtype_سیٹ_Void();  // placeholder; this node shouldnt survive beyond linking
        nodep->did_چوڑائی(سچ);
    }

    خالی دورہ(AstCellRef* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;

        اگر (Ast_گرہ_اظہاریہ* ثابت cellExprp = VN_CAST(nodep->cellp(), گرہ_اظہاریہ)) {
            user_Iterate_And_اگلا(cellExprp, چوڑائی_VP{SELF, PRELIM}.p());
            user_Iterate_And_اگلا(cellExprp, چوڑائی_VP{SELF, FINAL}.p());
        }

        اگر (Ast_گرہ_اظہاریہ* ثابت exprp = VN_CAST(nodep->exprp(), گرہ_اظہاریہ)) {
            user_Iterate_And_اگلا(exprp, چوڑائی_VP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            user_Iterate_And_اگلا(exprp, چوڑائی_VP{SELF, FINAL}.p());
        } ورنہ {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->did_چوڑائی(سچ);
    }

    // DTYPES
    خالی دورہ(Ast_گرہ_Array_DType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed

        اگر (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ثابت basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            اگر (basicp->implicit()) {
                UASSERT_OBJ(basicp->چوڑائی() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->چوڑائی() << " bits");
                AstBasicDType* ثابت newp = نیا AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->چوڑائی_Force(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        اگر (VN_IS(nodep, PackArrayDType)) m_underPackedArray = سچ;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        user_Iterate_And_اگلا(nodep->rangep(), چوڑائی_VP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        اگر (AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->چوڑائی_From_Sub(nodep->subDTypep());
            اگر (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(سچ);
        } ورنہ {
            ثابت Ast_گرہ_DType* ثابت elemDTypep = nodep->subDTypep()->skipRefp();
            اگر (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ثابت صحیح چوڑائی = nodep->subDTypep()->چوڑائی() * nodep->rangep()->elementsConst();
            nodep->چوڑائی_Force(چوڑائی, چوڑائی);
            اگر (!VL_RESTORER_پچھلا(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ثابت Ast_گرہ_DType* بنیاد_DTypep = nodep->subDTypep();
                جب (!VN_IS(بنیاد_DTypep, RefDType) && بنیاد_DTypep->subDTypep()) {
                    بنیاد_DTypep = بنیاد_DTypep->subDTypep();
                }
                ثابت AstBasicDType* ثابت basicp = nodep->basicp();
                اگر (!VN_IS(بنیاد_DTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(سچ));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(AstAssocArrayDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->کلید_DTypep(iterateEditMoveDTypep(nodep, nodep->کلید_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_گرہ_DType* ثابت childp = nodep->بچہ_DTypep();
        childp->unlink_Fr_عقب();
        Ast_گرہ* ثابت elementsp = nodep->elementsp()->unlink_Fr_عقب();
        Ast_گرہ* newp;
        اگر (VN_IS(elementsp, Unbounded)) {
            newp = نیا Ast_قطار_DType{nodep->fileline(), VFlag_بچہ_DType{}, childp, خالی_اشاریہ};
            VL_DO_DANGLING(elementsp->delete_درخت(), elementsp);
        } ورنہ اگر (Ast_گرہ_DType* ثابت keyp = VN_CAST(elementsp, گرہ_DType)) {
            newp = نیا AstAssocArrayDType{nodep->fileline(), VFlag_بچہ_DType{}, childp, keyp};
        } ورنہ {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_سطر* ثابت elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3_Error_رمز::WIDTHEXPAND, سچ);
            // Must be expression that is constant, but we'll determine that later
            newp = نیا AstUnpackArrayDType{
                nodep->fileline(), VFlag_بچہ_DType{}, childp,
                نیا AstRange{nodep->fileline(), نیا AstConst(elementsp->fileline(), 0),
                             نیا AstSub{elementsNewFl, VN_AS(elementsp, گرہ_اظہاریہ),
                                        نیا AstConst(elementsNewFl, 1)},
                             سچ}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, خالی_اشاریہ), newp);
    }
    خالی دورہ(AstConstraintRefDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    خالی دورہ(AstDynArrayDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(Ast_قطار_DType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        user_Iterate_And_اگلا(nodep->boundp(), چوڑائی_VP{SELF, BOTH}.p());
        اگر (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlink_Fr_عقب()->delete_درخت();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(AstVoidDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(AstUnsizedArrayDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        اگر (nodep->بچہ_DTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(AstWildcardArrayDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(AstBasicDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        اگر (nodep->generic()) واپس;  // Already perfect
        اگر (nodep->rangep()) {
            user_Iterate_And_اگلا(nodep->rangep(), چوڑائی_VP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->چوڑائی_Force(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } ورنہ اگر (nodep->isRanged()) {
            nodep->چوڑائی_Force(nodep->nrange().elements(), nodep->nrange().elements());
        } ورنہ اگر (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->چوڑائی_Force(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(AstConstDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, خالی_اشاریہ);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->چوڑائی_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(AstRefDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_چوڑائی(سچ);
        اگر (nodep->typeofp()) {  // type(typeofp_expression)
            اگر (Ast_گرہ_DType* typeofDtp = VN_CAST(nodep->typeofp(), گرہ_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } ورنہ {
                // Type comes from expression's type, e.g. "type(variable)"
                user_Iterate_And_اگلا(nodep->typeofp(), چوڑائی_VP{SELF, BOTH}.p());
                Ast_گرہ* ثابت typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlink_Fr_عقب()->delete_درخت(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, خالی_اشاریہ);
        اگر (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), خالی_اشاریہ);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), خالی_اشاریہ);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        اگر (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ثابت منطقی اندر_Template_Module = !m_modep || m_modep->parameterizedTemplate();
            اگر (اندر_Template_Module) {
                nodep->doing_چوڑائی(جھوٹ);
                واپس;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->چوڑائی_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doing_چوڑائی(جھوٹ);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    خالی چوڑائی_DType_Name_Params(ثابت AstTypedef* nodep) {
        ثابت VDTypeNameScopes* ثابت scopesp = VDTypeNameScopes::currentp();
        اگر (!scopesp) واپس;  // Computing parameters, as by V3Param, so naming as found
        Ast_گرہ* scopep = خالی_اشاریہ;
        برائے (ثابت Ast_گرہ* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            اگر (ثابت std::سمتیہ<Ast_گرہ*>* ثابت paramsp = scopesp->paramsp(scopep)) {
                برائے (Ast_گرہ* ثابت paramp : *paramsp) {
                    اگر (!paramp->did_چوڑائی()) userIterate(paramp, خالی_اشاریہ);
                }
            }
        }
    }
    خالی دورہ(AstTypedef* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        اگر (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(خالی_اشاریہ);
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ثابت منطقی declares = nodep->بچہ_DTypep() != خالی_اشاریہ;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        اگر (declares) {
            Ast_گرہ_DType* ثابت dtypep = nodep->dtypep();
            اگر (Ast_گرہ_UOr_Struct_DType* ثابت sdtypep = VN_CAST(dtypep, گرہ_UOr_Struct_DType)) {
                چوڑائی_DType_Name_Params(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } ورنہ اگر (AstEnumDType* ثابت edtypep = VN_CAST(dtypep, EnumDType)) {
                چوڑائی_DType_Name_Params(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, خالی_اشاریہ);
    }
    خالی دورہ(AstParamTypeDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, خالی_اشاریہ);
        nodep->چوڑائی_From_Sub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        اگر (nodep->dtypep() && nodep->بچہ_DTypep()) { nodep->بچہ_DTypep(خالی_اشاریہ); }
    }
    خالی دورہ(AstRequireDType* nodep) override {
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
        اگر (Ast_گرہ_DType* ثابت dtp = VN_CAST(nodep->lhsp(), گرہ_DType)) {
            nodep->replaceWith(dtp->unlink_Fr_عقب());
        } ورنہ {
            اگر (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(نیا AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
        Ast_گرہ_DType* ثابت مقصد_Dtp = nodep->چوٹی()->dtypep()->skip_Ref_مقصد_Enump();
        Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_مقصد_Enump();
        File_سطر* ثابت fl = nodep->fileline();
        ثابت خودکار castable = Ast_گرہ::computeCastable(مقصد_Dtp, fromDtp, nodep->fromp());
        Ast_گرہ* newp;
        اگر (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            واپس;
        } ورنہ اگر (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ثابت enumDtp = VN_AS(مقصد_Dtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_گرہ_اظہاریہ* ثابت testp = enumTestValid(nodep->fromp(), enumDtp);
            File_سطر* ثابت fl_novalue = نیا File_سطر{fl};
            fl_novalue->warnOff(V3_Error_رمز::ENUMVALUE, سچ);
            newp = نیا AstCond{
                fl, testp,
                نیا Ast_اظہاریہ_Stmt{fl,
                                نیا Ast_تفویض{fl_novalue, nodep->چوٹی()->unlink_Fr_عقب(),
                                              nodep->fromp()->unlink_Fr_عقب()},
                                نیا AstConst{fl, AstConst::Signed32{}, 1}},
                نیا AstConst{fl, AstConst::Signed32{}, 0}};
        } ورنہ اگر ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(مقصد_Dtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << مقصد_Dtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warn_مزید() << "... Suggest static cast");
            newp = نیا Ast_اظہاریہ_Stmt{
                fl,
                نیا Ast_تفویض{fl, nodep->چوٹی()->unlink_Fr_عقب(),
                              نیا AstCast{fl, nodep->fromp()->unlink_Fr_عقب(), مقصد_Dtp}},
                نیا AstConst{fl, AstConst::Signed32{}, 1}};
        } ورنہ اگر (castable == VCastable::INCOMPATIBLE) {
            newp = نیا AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << مقصد_Dtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } ورنہ {
            newp = نیا AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << مقصد_Dtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warn_مزید()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    خالی دورہ(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        اگر (AstConst* ثابت constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlink_Fr_عقب();
            Ast_گرہ* ثابت newp
                = نیا Ast_Cast_سائز{nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ورنہ اگر (Ast_گرہ_DType* ثابت refp = VN_CAST(nodep->dtp(), گرہ_DType)) {
            refp->unlink_Fr_عقب();
            Ast_گرہ* ثابت newp = نیا AstCast{nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب(),
                                              VFlag_بچہ_DType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ورنہ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlink_Fr_عقب());
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    خالی دورہ(AstCast* nodep) override {
        اگر (nodep->did_چوڑائی()) واپس;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        اگر (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            اگر (AstSigned* ثابت fromp = VN_CAST(nodep->fromp(), Signed)) {
                اگر (VN_IS(fromp->lhsp(), گرہ_دھارا)) {
                    Ast_گرہ* ثابت lhsp = fromp->lhsp()->unlink_Fr_عقب();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_درخت(), fromp);
                }
            }
            user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, PRELIM, دھارا_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_گرہ_DType* ثابت مقصد_Dtp = nodep->dtypep()->skip_Ref_مقصد_Enump();
            Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_مقصد_Enump();
            ثابت خودکار castable = Ast_گرہ::computeCastable(مقصد_Dtp, fromDtp, nodep->fromp());
            منطقی bad = جھوٹ;
            اگر (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << مقصد_Dtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = سچ;
            } ورنہ اگر (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } ورنہ اگر (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << مقصد_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warn_مزید() << "... Suggest dynamic $cast");
                bad = سچ;
            } ورنہ اگر (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << مقصد_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = سچ;
            } ورنہ {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
            اگر (bad) {
            } ورنہ اگر (AstBasicDType* ثابت basicp = مقصد_Dtp->basicp()) {
                اگر (!basicp->isString() && fromDtp->isString()) {
                    newp = نیا AstNToI{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(), مقصد_Dtp};
                } ورنہ اگر (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_گرہ_DType* ثابت origDTypep = nodep->dtypep();
                    اگر (!VN_IS(fromDtp, دھارا_DType)) {
                        ثابت صحیح چوڑائی = مقصد_Dtp->چوڑائی();
                        castSized(nodep, nodep->fromp(), چوڑائی);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } ورنہ {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 جھوٹ);
                }
                اگر (newp) {
                } ورنہ اگر (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    اگر (nodep->fromp()->isSigned()) {
                        newp = نیا AstISToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب()};
                    } ورنہ {
                        newp = نیا AstIToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب()};
                    }
                } ورنہ اگر (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = نیا AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب()};
                    newp->dtypep(basicp);
                } ورنہ اگر (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = نیا AstSigned{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب()};
                } ورنہ اگر (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = نیا AstUnsigned{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب()};
                } ورنہ {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } ورنہ اگر (VN_IS(مقصد_Dtp, قطار_DType)) {
                اگر (VN_IS(fromDtp, BasicDType)) {
                    newp = نیا Ast_Cvt_Packed_مقصد_Array{nodep->fileline(),
                                                   nodep->fromp()->unlink_Fr_عقب(), مقصد_Dtp};
                } ورنہ اگر (VN_IS(fromDtp, قطار_DType) || VN_IS(fromDtp, دھارا_DType)) {
                    صحیح src_Element_بٹ = 1;
                    صحیح dst_Element_بٹ = 1;
                    اگر (Ast_گرہ_DType* ثابت elemDtp = fromDtp->subDTypep()) {
                        src_Element_بٹ = elemDtp->چوڑائی();
                    }
                    ثابت Ast_قطار_DType* ثابت dst_قطار_Dtp = VN_AS(مقصد_Dtp, قطار_DType);
                    اگر (Ast_گرہ_DType* ثابت elemDtp = dst_قطار_Dtp->subDTypep()) {
                        dst_Element_بٹ = elemDtp->چوڑائی();
                    }
                    newp = نیا Ast_Cvt_Array_مقصد_Array{nodep->fileline(),
                                                  nodep->fromp()->unlink_Fr_عقب(),
                                                  مقصد_Dtp,
                                                  جھوٹ,
                                                  1,
                                                  dst_Element_بٹ,
                                                  src_Element_بٹ};
                }
            } ورنہ اگر (VN_IS(مقصد_Dtp, ClassRefDType)) {
                // Can just remove cast
            } ورنہ {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << مقصد_Dtp->prettyDTypeNameQ());
            }
            اگر (!newp) newp = nodep->fromp()->unlink_Fr_عقب();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        اگر (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, جھوٹ, دھارا_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_گرہ_اظہاریہ* ثابت underp = nodep->fromp()->unlink_Fr_عقب();
            underp->dtypeFrom(nodep);
            underp->did_چوڑائی(سچ);
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstCastWrap{nodep->fileline(), underp};
            newp->did_چوڑائی(سچ);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    خالی دورہ(Ast_Cast_سائز* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            صحیح چوڑائی = nodep->rhsp()->مقصد_SInt();
            اگر (چوڑائی < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << چوڑائی);
                چوڑائی = 1;
            }
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, PRELIM, دھارا_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), چوڑائی);  // lhsp may change
        }
        اگر (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_گرہ* ثابت underp = nodep->lhsp()->unlink_Fr_عقب();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    خالی دورہ(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{nodep->dtypep(), BOTH, دھارا_USE_CAST}.p());
    }
    خالی castSized(Ast_گرہ* nodep, Ast_گرہ* underp, صحیح چوڑائی) {
        ثابت AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        اگر (!underDtp) underDtp = underp->dtypep()->basicp();
        اگر (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->ڈھونڈو_ثنائی_ہندسہ_DType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ثابت صحیح calc_چوڑائی = std::max(چوڑائی, underDtp->چوڑائی());
            Ast_گرہ_DType* ثابت calcDtp = nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                calc_چوڑائی, calc_چوڑائی, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         جھوٹ, دھارا_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_گرہ_DType* ثابت برآمد_Dtp = nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                چوڑائی, چوڑائی, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(برآمد_Dtp);
            // We ignore warnings as that is sort of the point of a cast
            چوڑائی_Check_Sized(nodep, "Cast expr", VN_AS(underp, گرہ_اظہاریہ), برآمد_Dtp, EXTEND_EXP,
                            جھوٹ);
            VL_DANGLING(underp);
        }
    }
    خالی دورہ(AstConstraintBefore* nodep) override {
        user_Iterate_And_اگلا(nodep->lhssp(), چوڑائی_VP{SELF, BOTH}.p());
        user_Iterate_And_اگلا(nodep->rhssp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(Ast_Constraint_اظہاریہ* nodep) override {
        user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstConstraintUnique* nodep) override {
        user_Iterate_And_اگلا(nodep->rangesp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(Ast_Dist_شے* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), چوڑائی_VP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    خالی دورہ(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        اگر (nodep->did_چوڑائی()) واپس;
        اگر (nodep->doing_چوڑائی()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ڈالو_Deletep(nodep->valuep()->unlink_Fr_عقب());
            nodep->valuep(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->did_چوڑائی(سچ);
            واپس;
        }
        nodep->doing_چوڑائی(سچ);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        اگر (!m_paramsOnly && nodep->is_Iface_Array_بانٹو()) m_بانٹو_Iface_Array_Varps.ڈالو_عقب(nodep);
        اگر (nodep->attrsp()) {
            nodep->attrsp()->foreach([یہ, nodep](Ast_Attr_کا* attrp) {
                اگر (attrp->attrType() == VAttrType::VAR_پورٹ_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    اگر (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), گرہ_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ڈالو_Deletep(attrp->unlink_Fr_عقب()), attrp);
                }
            });
        }
        اگر (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_گرہ_DType* dtp = nodep->dtypep();
            Ast_گرہ_DType* np = خالی_اشاریہ;
            جب (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                اگر (ثابت AstDynArrayDType* ثابت unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    اگر (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } ورنہ {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ثابت newp
                        = نیا AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    اگر (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } ورنہ {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->جوڑو_Typesp(newp);
                    np = newp;
                } ورنہ {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        اگر (AstWildcardArrayDType* ثابت wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        اگر (VN_IS(nodep->dtypep()->skip_Ref_مقصد_Constp(), ConstDType)) nodep->isConst(سچ);
        // Parameters if implicit untyped inherit from what they are assigned to
        ثابت AstBasicDType* ثابت bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        منطقی didchk = جھوٹ;
        ثابت منطقی implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        اگر (implicitParam) {
            اگر (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                user_Iterate_And_اگلا(nodep->valuep(), چوڑائی_VP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                اگر (nodep->valuep()->isDouble()) {
                    nodep->dtype_سیٹ_Double();
                    VL_DANGLING(bdtypep);
                } ورنہ اگر (nodep->valuep()->isString()) {
                    nodep->dtype_سیٹ_String();
                    VL_DANGLING(bdtypep);
                } ورنہ {
                    صحیح چوڑائی = 0;
                    Ast_گرہ_DType* ثابت قدر_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(قدر_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ثابت AstBasicDType* ثابت قدر_Bdtypep = قدر_DTypep->basicp();
                    منطقی issigned = جھوٹ;
                    اگر (bdtypep->isNosign()) {
                        اگر (قدر_Bdtypep && قدر_Bdtypep->isSigned()) issigned = سچ;
                    } ورنہ {
                        issigned = bdtypep->isSigned();
                    }
                    اگر (قدر_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        اگر (ثابت خودکار* ثابت constp = VN_CAST(nodep->valuep(), Const)) {
                            اگر (constp->عدد().isString()) {
                                چوڑائی = constp->عدد().مقصد_String().لمبائی_2() * 8;
                            }
                        }
                        اگر (چوڑائی < 8) چوڑائی = 8;
                    } ورنہ اگر (nodep->valuep()->dtypep()->چوڑائی_Sized()) {
                        چوڑائی = nodep->valuep()->چوڑائی();
                    } ورنہ {
                        اگر (nodep->valuep()->چوڑائی() > 32) {
                            nodep->valuep()->v3warn(
                                چوڑائی_2,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        چوڑائی = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ثابت منطقی is_قدر_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    اگر (چوڑائی == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_Range_DType(
                            VNumRange{0, 0}, nodep->valuep()->چوڑائی_Min(),
                            VSigning::fromBool(issigned), is_قدر_Fourstate));
                    } ورنہ {
                        nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(چوڑائی, nodep->valuep()->چوڑائی_Min(),
                                                         VSigning::fromBool(issigned),
                                                         is_قدر_Fourstate);
                    }
                    didchk = سچ;
                }
                iterate_Check_تفویض(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } ورنہ {
                // Or, if nothing assigned, they're integral
                nodep->dtype_سیٹ_Integer();
                VL_DANGLING(bdtypep);
            }
        } ورنہ اگر (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtype_سیٹ_Logic_Sized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        اگر (nodep->isNet()) {
            Ast_گرہ_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
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
            user_Iterate_And_اگلا(nodep->valuep(), چوڑائی_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_تفویض(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        user_Iterate_And_اگلا(nodep->delayp(), چوڑائی_VP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->did_چوڑائی(سچ);
        nodep->doing_چوڑائی(جھوٹ);
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        اگر (nodep->did_چوڑائی()) واپس;
        اگر (!nodep->varp()) {
            اگر (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, جھوٹ,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                واپس;
            } ورنہ {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        اگر (!nodep->varp()->did_چوڑائی()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), خالی_اشاریہ);
        }
        اگر (!m_paramsOnly && nodep->varp()->is_Iface_Array_بانٹو()) {
            replace_بانٹو_Iface_Array_Ref(nodep);
            واپس;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        اگر (VN_IS(nodep->backp(), گرہ_تفویض) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        اگر (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(سچ);
        اگر (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } ورنہ اگر (nodep->access().isWriteOrRW() && nodep->varp()->is_داخلہ()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), تفویض_Force) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ثابت منطقی hierRef = !m_curModVars.گنتی(nodep->varp());
            ثابت منطقی net_پورٹ
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::پورٹ;
            ثابت منطقی cont_تفویض = VN_IS(nodep->abovep(), تفویض_W);
            ثابت منطقی pinConn = VN_IS(nodep->abovep(), Pin);
            اگر (!(hierRef && net_پورٹ && (cont_تفویض || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } ورنہ اگر (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
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
        nodep->did_چوڑائی(سچ);
    }

    خالی دورہ(AstEnumDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_گرہ_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_گرہ_DType* ثابت badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        اگر (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->ڈھونڈو_Int_DType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->چوڑائی_From_Sub(nodep->subDTypep());
        // Assign widths
        user_Iterate_And_اگلا(nodep->itemsp(), چوڑائی_VP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_نمبر عدد(nodep, nodep->چوڑائی(), 0);
        ثابت V3_نمبر one{nodep, nodep->چوڑائی(), 1};
        منطقی wrapAround = جھوٹ;
        std::map<ثابت V3_نمبر, Ast_Enum_شے*> inits;
        برائے (Ast_Enum_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شے)) {
            اگر (itemp->valuep()) {
                اگر (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_درخت("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                اگر (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlink_Fr_عقب()->delete_درخت();
                    جاری;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = جھوٹ;  // Explicit value resets wrap-around tracking
            }
            اگر (!itemp->valuep()) {
                اگر (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                اگر (عدد.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(نیا AstConst{itemp->fileline(), عدد});
            }

            ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
            اگر (constp->عدد().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            عدد.op_تفویض(constp->عدد());
            // Look for duplicates
            ثابت خودکار pair = inits.emplace(عدد, itemp);
            اگر (!pair.دوسرا) {  // IEEE says illegal
                ثابت Ast_گرہ* ثابت otherp = pair.پہلا->دوسرا;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ثابت منطقی پچھلا_Neg = constp->عدد().isNegative();
            عدد.op_جوڑو(one, constp->عدد());
            اگر (basicp->isSigned()) {
                wrapAround = !پچھلا_Neg && عدد.isNegative();
            } ورنہ {
                wrapAround = عدد.isEqZero();
            }
        }
    }
    خالی دورہ(Ast_Enum_شے* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_گرہ_DType* ثابت vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        اگر (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            user_Iterate_And_اگلا(nodep->valuep(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            منطقی warnOn = سچ;
            Ast_گرہ_اظہاریہ* valuep = nodep->valuep();
            اگر (ثابت Ast_جوڑو* ثابت anodep = VN_CAST(valuep, جوڑو_2)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                اگر (anodep->fileline()->equal_پہلا_سطر_ستون(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                اگر (static_cast<صحیح>(constp->عدد().most_سیٹ_ثنائی_ہندسہ_P1()) > nodep->چوڑائی()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = جھوٹ;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(نیا AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlink_Fr_عقب(),
                                      nodep->dtypep()});
        }
    }
    خالی دورہ(Ast_Enum_شے_Ref* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        اگر (!nodep->itemp()->did_چوڑائی()) {
            // We need to do the whole enum en masse
            Ast_گرہ* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            برائے (; enump; enump = enump->backp()) {
                اگر (VN_IS(enump, EnumDType)) توڑو;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    خالی دورہ(Ast_گرہ_Cons* nodep) override {}  // Widthed when created
    خالی دورہ(Ast_گرہ_Cons_شے* nodep) override {}  // Widthed when created
    خالی دورہ(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
        برائے (Ast_گرہ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, چوڑائی_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_گرہ_DType* subDTypep = خالی_اشاریہ;
        nodep->dtype_سیٹ_ثنائی_ہندسہ();

        اگر (dtype && dtype->isString()) {
            nodep->dtype_سیٹ_String();
            subDTypep = nodep->ڈھونڈو_String_DType();
        } ورنہ اگر (dtype && dtype->isDouble()) {
            nodep->dtype_سیٹ_Double();
            subDTypep = nodep->ڈھونڈو_Double_DType();
        } ورنہ {
            // Take width as maximum across all items
            صحیح چوڑائی = nodep->exprp()->چوڑائی();
            صحیح mwidth = nodep->exprp()->چوڑائی_Min();
            منطقی isFourstate = nodep->exprp()->dtypep()->isFourstate();
            برائے (ثابت Ast_گرہ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                چوڑائی = std::max(چوڑائی, itemp->چوڑائی());
                mwidth = std::max(mwidth, itemp->چوڑائی_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                چوڑائی, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        برائے (Ast_گرہ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, Dist_شے)->rangep();
            اگر (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, چوڑائی_VP{subDTypep, FINAL}.p());
            } ورنہ {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        اگر (m_constraintp) {
            منطقی can_زیریں = سچ;
            برائے (Ast_Dist_شے* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), Dist_شے)) {
                اگر (VN_IS(ditemp->rangep(), InsideRange)) جاری;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ثابت Ast_گرہ_DType* ثابت شے_Dtp = ditemp->rangep()->dtypep()->skipRefp();
                اگر (!شے_Dtp->isIntegralOrPacked() || !ditemp->rangep()->is_خالص()) {
                    can_زیریں = جھوٹ;
                    توڑو;
                }
            }
            اگر (can_زیریں) واپس;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
        برائے (Ast_Dist_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Dist_شے)) {
            Ast_گرہ_اظہاریہ* inewp
                = inside_شے(nodep, nodep->exprp()->clone_درخت_خالص(جھوٹ), itemp->rangep());
            اگر (!inewp) جاری;
            Ast_گرہ_اظہاریہ* ثابت cmpp
                = نیا AstGt{itemp->fileline(), itemp->weightp()->unlink_Fr_عقب(),
                            نیا AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3_Error_رمز::UNSIGNED, سچ);
            cmpp->fileline()->modifyWarnOff(V3_Error_رمز::CMPCONST, سچ);
            inewp = نیا AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? نیا AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        اگر (!newp) newp = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(AstInside* nodep) override {
        user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
        برائے (Ast_گرہ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, چوڑائی_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_گرہ_DType* expDTypep = خالی_اشاریہ;

        اگر (dtype && dtype->isString()) {
            nodep->dtype_سیٹ_String();
            expDTypep = nodep->ڈھونڈو_String_DType();
        } ورنہ اگر (dtype && dtype->isDouble()) {
            nodep->dtype_سیٹ_Double();
            expDTypep = nodep->ڈھونڈو_Double_DType();
        } ورنہ {
            // Take width as maximum across all items
            صحیح چوڑائی = nodep->exprp()->چوڑائی();
            صحیح mwidth = nodep->exprp()->چوڑائی_Min();
            منطقی isFourstate = nodep->exprp()->dtypep()->isFourstate();
            برائے (ثابت Ast_گرہ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                چوڑائی = std::max(چوڑائی, itemp->چوڑائی());
                mwidth = std::max(mwidth, itemp->چوڑائی_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
            ثابت VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(چوڑائی, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        برائے (Ast_گرہ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            اگر (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, چوڑائی_VP{expDTypep, FINAL}.p());
            } ورنہ اگر (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_گرہ_اظہاریہ* exprp;
        Ast_اظہاریہ_Stmt* اظہاریہ_Stmtp = خالی_اشاریہ;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        اگر (!m_constraintp && !nodep->exprp()->is_خالص()) {
            File_سطر* ثابت fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_گرہ_DType* ثابت اظہاریہ_Dtp = nodep->exprp()->dtypep();
            ثابت صحیح w = اظہاریہ_Dtp->چوڑائی();
            Ast_گرہ_DType* ثابت عارضی_DTypep
                = اظہاریہ_Dtp->چوڑائی_Sized() ? اظہاریہ_Dtp
                                        : nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(w, w, اظہاریہ_Dtp->numeric(),
                                                                     اظہاریہ_Dtp->isFourstate());
            AstVar* ثابت varp
                = نیا AstVar{fl, VVarType::XTEMP, m_inside_عارضی_نام.get(nodep), عارضی_DTypep};
            exprp = نیا AstVarRef{fl, varp, VAccess::READ};
            اظہاریہ_Stmtp = نیا Ast_اظہاریہ_Stmt{fl,
                                        نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlink_Fr_عقب()},
                                        exprp};
            اگر (m_ftaskp) {
                varp->funcLocal(سچ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ورنہ {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(اظہاریہ_Stmtp);
        } ورنہ {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
        برائے (Ast_گرہ_اظہاریہ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), گرہ_اظہاریہ);  // Will be unlinking
            Ast_گرہ_اظہاریہ* inewp;
            اگر (اظہاریہ_Stmtp) {
                inewp = inside_شے(nodep, اظہاریہ_Stmtp, itemp);
                اظہاریہ_Stmtp = خالی_اشاریہ;
            } ورنہ {
                inewp = inside_شے(nodep, exprp->clone_درخت_خالص(جھوٹ), itemp);
            }
            اگر (!inewp) جاری;
            newp = newp ? نیا AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        اگر (اظہاریہ_Stmtp) VL_DO_DANGLING(اظہاریہ_Stmtp->delete_درخت(), اظہاریہ_Stmtp);
        اگر (!newp) newp = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    Ast_گرہ_اظہاریہ* inside_شے(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* itemp) {
        ثابت Ast_گرہ_DType* ثابت شے_Dtp = itemp->dtypep()->skipRefp();
        اگر (AstInsideRange* ثابت irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            واپس irangep->newAndFromInside(exprp, irangep->lhsp()->unlink_Fr_عقب(),
                                             irangep->rhsp()->unlink_Fr_عقب());
        } ورنہ اگر (VN_IS(شے_Dtp, UnpackArrayDType) || VN_IS(شے_Dtp, DynArrayDType)
                   || VN_IS(شے_Dtp, قطار_DType)) {
            // Unsupported in parameters
            Ast_گرہ_اظہاریہ* ثابت inewp = نیا AstCMethodHard{nodep->fileline(), itemp->unlink_Fr_عقب(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, شے_Dtp->subDTypep(), BOTH);
            inewp->dtype_سیٹ_ثنائی_ہندسہ();
            inewp->did_چوڑائی(سچ);
            واپس inewp;
        } ورنہ اگر (VN_IS(شے_Dtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_درخت(), exprp);
            واپس خالی_اشاریہ;
        }
        واپس AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlink_Fr_عقب());
    }
    خالی دورہ(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        اگر (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
            user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } ورنہ {
            اگر (m_vup->prelim()) {
                user_Iterate_And_اگلا(nodep->lhsp(), m_vup);
                user_Iterate_And_اگلا(nodep->rhsp(), m_vup);
            }
            اگر (m_vup->final()) {
                Ast_گرہ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ثابت صحیح exp_چوڑائی = expDTypep->چوڑائی();
                ثابت منطقی waiveLhs = exp_چوڑائی == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->چوڑائی_Min() >= nodep->lhsp()->چوڑائی();
                ثابت منطقی waiveRhs = exp_چوڑائی == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && exp_چوڑائی >= nodep->rhsp()->چوڑائی_Min();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    خالی دورہ(AstIfaceRefDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        اگر (nodep->isVirtual()) nodep->iface_بذریعہ_Cellp()->سیٹ_Has_Virtual_Ref();
        UINFO(4, "dtWidthed " << nodep);
    }
    خالی دورہ(Ast_گرہ_UOr_Struct_DType* nodep) override {
        اگر (nodep->doing_چوڑائی()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
            nodep->doing_چوڑائی(جھوٹ);
            واپس;
        }
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_چوڑائی(سچ);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        اگر (ثابت AstUnionDType* ثابت unionp = VN_CAST(nodep, UnionDType)) {
            اگر (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, خالی_اشاریہ);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(جھوٹ);
        // Error checks
        برائے (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گرہ_DType* ثابت dtp = itemp->subDTypep()->skipRefp();
            اگر (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            اگر ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ڈالو_Deletep(itemp->valuep()->unlink_Fr_عقب());
            }
        }
        ثابت منطقی isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ثابت منطقی اندر_Template_Module = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        اگر (VN_IS(nodep, UnionDType) || nodep->packed()) {
            صحیح lsb = 0;
            صحیح چوڑائی = 0;
            منطقی پہلا = سچ;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            برائے (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            برائے (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                اگر (itemp->isFourstate()) nodep->isFourstate(سچ);
                itemp->lsb(lsb);
                اگر (VN_IS(nodep, UnionDType)) {
                    ثابت صحیح شے_چوڑائی = itemp->چوڑائی();
                    // Skip union size check for template modules with unresolved parameters
                    اگر (!پہلا && isHardPackedUnion && شے_چوڑائی != چوڑائی && !اندر_Template_Module) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    چوڑائی = std::max(چوڑائی, شے_چوڑائی);
                } ورنہ {
                    lsb += itemp->چوڑائی();
                    چوڑائی += itemp->چوڑائی();
                }
                پہلا = جھوٹ;
            }
            nodep->چوڑائی_Force(چوڑائی, چوڑائی);  // Signing stays as-is, as parsed from declaration
        } ورنہ {
            nodep->چوڑائی_Force(1, 1);
        }
        nodep->doing_چوڑائی(جھوٹ);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    خالی دورہ(AstThisRef* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->بچہ_DTypep()));
        اگر (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    خالی دورہ(AstClassRefDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    خالی دورہ(AstClassOrPackageRef* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        userIterateChildren(nodep, خالی_اشاریہ);
    }
    خالی دورہ(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warn_مزید() << "... Suggest use a typedef");
        nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstClassExtends* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        اگر (VN_IS(nodep->بچہ_DTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->بچہ_DTypep()));
        }
    }
    خالی دورہ(AstMemberDType* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->چوڑائی_From_Sub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        اگر (nodep->valuep()) {
            user_Iterate_And_اگلا(nodep->valuep(), چوڑائی_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_تفویض(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    خالی دورہ(AstStructSel* nodep) override {
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        اگر (nodep->did_چوڑائی()) واپس;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = سچ;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_مقصد_Enump();
        UINFO(9, "     from dt " << fromDtp);
        ثابت AstMemberSel* ثابت fromSel = VN_CAST(nodep->fromp(), MemberSel);
        اگر (AstClocking* ثابت clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->پہلا_Abovep(), Clocking)
                                               : خالی_اشاریہ) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            اگر (AstVar* ثابت varp = memberSelClocking(nodep, clockingp)) {
                اگر (!varp->did_چوڑائی()) userIterate(varp, خالی_اشاریہ);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlink_Fr_عقب());
                VL_DO_DANGLING(fromp->delete_درخت(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                اگر (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::link_LValue_سیٹ(nodep, nodep->access());
                }
                اگر (AstIfaceRefDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->did_چوڑائی(سچ);  // Must not visit again: will confuse scopes
                واپس;
            }
        } ورنہ اگر (Ast_گرہ_UOr_Struct_DType* ثابت adtypep = VN_CAST(fromDtp, گرہ_UOr_Struct_DType)) {
            اگر (memberSelStruct(nodep, adtypep)) واپس;
        } ورنہ اگر (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            اگر (memberSelClass(nodep, adtypep)) واپس;
        } ورنہ اگر (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            اگر (Ast_گرہ* foundp = memberSelIface(nodep, adtypep)) {
                اگر (AstClocking* ثابت clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                اگر (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    اگر (!varp->did_چوڑائی()) userIterate(varp, خالی_اشاریہ);
                    AstIface* ثابت ifacep = adtypep->iface_بذریعہ_Cellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ثابت AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
                    اگر (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ثابت newp = نیا AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->did_چوڑائی(سچ);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                        واپس;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_چوڑائی(سچ);
                    واپس;
                }
                اگر (AstModport* ثابت modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ثابت newDtypep = نیا AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->جوڑو_Typesp(newDtypep);
                    Ast_گرہ_اظہاریہ* ثابت fromp = nodep->fromp()->unlink_Fr_عقب();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
                اگر (AstCell* ثابت cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    اگر (VN_IS(cellp->modp(), Iface)) {
                        ثابت string viftopName = cellp->name() + "__Viftop";
                        Ast_گرہ_Module* ثابت والد_Ifacep = adtypep->iface_بذریعہ_Cellp();
                        AstVar* viftopVarp = خالی_اشاریہ;
                        برائے (Ast_گرہ* itemp = والد_Ifacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            اگر (AstVar* ثابت vp = VN_CAST(itemp, Var)) {
                                اگر (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    توڑو;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        اگر (!viftopVarp->did_چوڑائی()) userIterate(viftopVarp, خالی_اشاریہ);
                        Ast_گرہ_DType* subDtypep = viftopVarp->dtypep();
                        اگر (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            اگر (AstIfaceRefDType* ثابت subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ثابت newDtypep = نیا AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(سچ);
                                v3Global.rootp()->typeTablep()->جوڑو_Typesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->did_چوڑائی(سچ);
                        واپس;
                    }
                }
                اگر (Ast_گرہ_FTask* ثابت ftaskp = VN_CAST(foundp, گرہ_FTask)) {
                    Ast_طریقہ_Call* ثابت newp = نیا Ast_طریقہ_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    واپس;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } ورنہ اگر (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, قطار_DType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_گرہ* ثابت newp = نیا Ast_طریقہ_Call{nodep->fileline(),
                                                    nodep->fromp()->unlink_Fr_عقب(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            واپس;
        } ورنہ {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    منطقی memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        اگر (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            Ast_طریقہ_Call* ثابت newp = نیا Ast_طریقہ_Call{
                nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            دورہ(newp);
            واپس سچ;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت پہلا_classp = adtypep->classp();
        UASSERT_OBJ(پہلا_classp, nodep, "Unlinked");
        برائے (AstClass* classp = پہلا_classp; classp;) {
            اگر (Ast_گرہ* ثابت foundp = m_memberMap.ڈھونڈو_Member(classp, nodep->name())) {
                اگر (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    اگر (!varp->did_چوڑائی()) userIterate(varp, خالی_اشاریہ);
                    اگر (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ثابت varRefp
                            = نیا AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                        واپس سچ;
                    }
                    اگر (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_چوڑائی(سچ);
                    اگر (nodep->fromp()->same_درخت(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::link_LValue_سیٹ(nodep);
                    واپس سچ;
                }
                اگر (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(نیا AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(), constrp});
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس سچ;
                }
                اگر (Ast_Enum_شے_Ref* ثابت adfoundp = VN_CAST(foundp, Enum_شے_Ref)) {
                    nodep->replaceWith(adfoundp->clone_درخت(جھوٹ));
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس سچ;
                }
                اگر (Ast_گرہ_FTask* ftaskp = VN_CAST(foundp, گرہ_FTask)) {
                    Ast_طریقہ_Call* newp = نیا Ast_طریقہ_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس سچ;
                }
                اگر (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->ڈھونڈو_Constraint_Ref_DType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    واپس سچ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : خالی_اشاریہ;
        }

        VSpellCheck speller;
        برائے (AstClass* classp = پہلا_classp; classp;) {
            برائے (Ast_گرہ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                اگر (VN_IS(itemp, Constraint) || VN_IS(itemp, Enum_شے_Ref) || VN_IS(itemp, Var)) {
                    speller.ڈالو_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : خالی_اشاریہ;
        }
        ثابت string suggest = speller.بہترین_Candidate_پیغام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << پہلا_classp->verilogKwd()
                      << " " << پہلا_classp->prettyNameQ() << "\n"
                      << (suggest.خالی_خانے() ? "" : nodep->fileline()->warn_مزید() + suggest));
        واپس جھوٹ;  // Caller handles error
    }
    منطقی modport_Allows_شے(Ast_گرہ* nodep, ثابت AstIfaceRefDType* adtypep, ثابت Ast_گرہ* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ثابت AstModport* ثابت modportp = adtypep->modportp();
        اگر (!modportp) واپس سچ;
        اگر (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) {
            اگر (varp->isParam()) واپس سچ;
        } ورنہ اگر (!VN_IS(itemp, گرہ_FTask) && !VN_IS(itemp, Clocking)) {
            واپس سچ;
        }
        خودکار pair = m_modport_نام.emplace(modportp, std::unordered_سیٹ<string>{});
        اگر (pair.دوسرا) {
            برائے (ثابت Ast_گرہ* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.پہلا->دوسرا.داخل_کرو(mpItemp->name());
            }
        }
        اگر (pair.پہلا->دوسرا.گنتی(itemp->name())) واپس سچ;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->iface_بذریعہ_Cellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        واپس جھوٹ;
    }
    Ast_گرہ* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_گرہ_Module* ثابت ifacep = adtypep->iface_بذریعہ_Cellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        برائے (Ast_گرہ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            اگر (itemp->name() == nodep->name()) {
                اگر (!modport_Allows_شے(nodep, adtypep, itemp)) {
                    واپس خالی_اشاریہ;  // Caller handles error
                }
                واپس itemp;
            }
            اگر (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ڈالو_Candidate(itemp->prettyName());
            }
        }
        ثابت string suggest = speller.بہترین_Candidate_پیغام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.خالی_خانے() ? "" : nodep->fileline()->warn_مزید() + suggest));
        واپس خالی_اشاریہ;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        برائے (Ast_گرہ* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            اگر (Ast_Clocking_شے* citemp = VN_CAST(itemp, Clocking_شے)) {
                اگر (citemp->varp()->name() == nodep->name()) واپس citemp->varp();
                speller.ڈالو_Candidate(citemp->varp()->prettyName());
            }
        }
        ثابت string suggest = speller.بہترین_Candidate_پیغام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.خالی_خانے() ? "" : nodep->fileline()->warn_مزید() + suggest));
        واپس خالی_اشاریہ;  // Caller handles error
    }
    منطقی memberSelStruct(AstMemberSel* nodep, Ast_گرہ_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        اگر (AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.ڈھونڈو_Member(adtypep, nodep->name()), MemberDType)) {
            اگر (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } ورنہ اگر (adtypep->packed()) {
                AstSel* ثابت newp = نیا AstSel{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                                memberp->lsb(), memberp->چوڑائی()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_مقصد_Enump());
                newp->did_چوڑائی(سچ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } ورنہ {
                AstStructSel* ثابت newp = نیا AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_مقصد_Enump());
                newp->did_چوڑائی(سچ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            واپس سچ;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        واپس جھوٹ;
    }

    خالی دورہ(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    خالی دورہ(Ast_طریقہ_Call* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        اگر (nodep->did_چوڑائی()) واپس;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), چوڑائی_VP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_مقصد_Enump();
        AstBasicDType* ثابت basicp = fromDtp ? fromDtp->basicp() : خالی_اشاریہ;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, چوڑائی_VP{SELF, BOTH}.p());
        اگر (nodep->name() == "rand_mode") {
            طریقہ_Call_Rand_منوال(nodep);
        } ورنہ اگر (nodep->name() == "constraint_mode") {
            طریقہ_Call_Constraint(nodep, خالی_اشاریہ);
        } ورنہ اگر (AstEnumDType* ثابت adtypep = VN_CAST(fromDtp, EnumDType)) {
            طریقہ_Call_Enum(nodep, adtypep);
        } ورنہ اگر (AstAssocArrayDType* ثابت adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            طریقہ_Call_Assoc(nodep, adtypep);
        } ورنہ اگر (AstWildcardArrayDType* ثابت adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            طریقہ_Call_Wildcard(nodep, adtypep);
        } ورنہ اگر (AstDynArrayDType* ثابت adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            طریقہ_Call_Dyn(nodep, adtypep);
        } ورنہ اگر (Ast_قطار_DType* ثابت adtypep = VN_CAST(fromDtp, قطار_DType)) {
            طریقہ_Call_قطار(nodep, adtypep);
        } ورنہ اگر (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            طریقہ_Call_Class(nodep, adtypep);
        } ورنہ اگر (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            طریقہ_Call_Iface_Ref(nodep, adtypep);
        } ورنہ اگر (AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            طریقہ_Call_Unpack(nodep, adtypep);
        } ورنہ اگر (AstConstraintRefDType* ثابت adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            طریقہ_Call_Constraint(nodep, adtypep);
        } ورنہ اگر (basicp && basicp->isEvent()) {
            طریقہ_Call_Event(nodep, basicp);
        } ورنہ اگر (basicp && basicp->isString()) {
            طریقہ_Call_String(nodep, basicp);
        } ورنہ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtype_سیٹ_Void();
        }
    }
    AstWith* طریقہ_With_Clause(Ast_گرہ_FTask_Ref* nodep, منطقی required, منطقی arbReturn,
                              Ast_گرہ_DType* returnDtp, Ast_گرہ_DType* اشاریہ_Dtp,
                              Ast_گرہ_DType* قدر_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        اگر (AstWith* ثابت withp = nodep->withp()) {
            withp->اشاریہ_Arg_Refp()->dtypep(اشاریہ_Dtp);
            withp->قدر_Arg_Refp()->dtypep(قدر_Dtp);
            userIterate(withp, چوڑائی_VP{returnDtp, BOTH}.p());
            withp->unlink_Fr_عقب();
            واپس withp;
        }
        اگر (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        واپس خالی_اشاریہ;
    }
    خالی طریقہ_Ok_Arguments(Ast_گرہ_FTask_Ref* nodep, صحیح minArg, صحیح maxArg) {
        صحیح narg = 0;
        اگر (AstWith* ثابت withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ڈالو_Deletep(withp->unlink_Fr_عقب()), withp);
        }
        برائے (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ثابت منطقی ok = (narg >= minArg) && (narg <= maxArg);
        اگر (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvt_مقصد_Str(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvt_مقصد_Str(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            برائے (; narg < minArg; ++narg) {
                nodep->جوڑو_Argsp(
                    نیا AstArg{nodep->fileline(), "",
                               نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}}});
            }
            برائے (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                جب (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlink_Fr_عقب();
                VL_DO_DANGLING(argp->delete_درخت(), argp);
            }
        }
    }

    Ast_گرہ_اظہاریہ* طریقہ_Arg(Ast_طریقہ_Call* nodep, صحیح arg) {
        AstArg* argp = nodep->argsp();
        برائے (صحیح narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        واپس argp->exprp();
    }

    خالی طریقہ_Call_Enum(Ast_طریقہ_Call* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        اگر (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = خالی_اشاریہ;
            طریقہ_Ok_Arguments(nodep, 0, 0);
            اگر (nodep->name() == "num") {
                صحیح اشیاء = 0;
                برائے (Ast_گرہ* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++اشیاء;
                newp = نیا AstConst(nodep->fileline(), AstConst::Signed32{}, اشیاء);
            } ورنہ اگر (nodep->name() == "first") {
                ثابت Ast_Enum_شے* itemp = adtypep->itemsp();
                اگر (!itemp) {
                    newp = نیا AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ورنہ {
                    newp = VN_AS(itemp->valuep()->clone_درخت(جھوٹ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } ورنہ اگر (nodep->name() == "last") {
                ثابت Ast_Enum_شے* itemp = adtypep->itemsp();
                جب (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), Enum_شے);
                اگر (!itemp) {
                    newp = نیا AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ورنہ {
                    newp = VN_AS(itemp->valuep()->clone_درخت(جھوٹ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } ورنہ اگر (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            اگر (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                طریقہ_Ok_Arguments(nodep, 0, 0);
            } ورنہ اگر (nodep->name() == "next") {
                attrType = VAttrType::ENUM_اگلا;
                طریقہ_Ok_Arguments(nodep, 0, 1);
            } ورنہ اگر (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_پچھلا;
                طریقہ_Ok_Arguments(nodep, 0, 1);
            } ورنہ {
                nodep->v3fatalSrc("Bad case");
            }

            اگر (nodep->name() != "name" && nodep->argsp()) {
                Ast_گرہ_اظہاریہ* stepp = طریقہ_Arg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = طریقہ_Arg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3_Error_رمز::WIDTHEXPAND, سچ);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                اگر (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_گرہ_اظہاریہ* ثابت newp = نیا AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ثابت uint32_t قدم_چوڑائی = VN_AS(stepp, Const)->مقصد_UInt();
                اگر (قدم_چوڑائی == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    Ast_طریقہ_Call* ثابت newp = نیا Ast_طریقہ_Call{
                        nodep->fileline(),
                        نیا Ast_طریقہ_Call{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    واپس;
                } ورنہ اگر (قدم_چوڑائی != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ڈالو_Deletep(nodep->argsp()->unlink_Fr_عقب());
                    Ast_طریقہ_Call* ثابت clonep = nodep->clone_درخت(جھوٹ);
                    VN_AS(stepp, Const)->عدد().سیٹ_Long(1);
                    AstConst* ثابت constp = نیا AstConst(nodep->fileline(), قدم_چوڑائی - 1);
                    AstArg* ثابت argp = نیا AstArg{nodep->fileline(), "", constp};
                    Ast_طریقہ_Call* ثابت newp
                        = نیا Ast_طریقہ_Call{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    واپس;
                }
            }
            Ast_گرہ_اظہاریہ* ثابت newp
                = enumSelect(nodep->fromp()->unlink_Fr_عقب(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    خالی طریقہ_Call_Wildcard(Ast_طریقہ_Call* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = خالی_اشاریہ;
        اگر (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            طریقہ_Ok_Arguments(nodep, 0, 0);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::ASSOC_سائز};  // So don't need num()
            newp->dtype_سیٹ_Int();
        } ورنہ اگر (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } ورنہ اگر (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", طریقہ_Arg(nodep, 0), SELF, BOTH);
            Ast_گرہ_اظہاریہ* ثابت اشاریہ_exprp = طریقہ_Call_Wildcard_اشاریہ_اظہاریہ(nodep, adtypep);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::ASSOC_EXISTS, اشاریہ_exprp->unlink_Fr_عقب()};
            newp->dtype_سیٹ_ثنائی_ہندسہ_Unsized(32, 1, VSigning::SIGNED);
        } ورنہ اگر (nodep->name() == "delete") {  // function void delete([input integer index])
            طریقہ_Ok_Arguments(nodep, 0, 1);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            اگر (!nodep->argsp()) {
                newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_سیٹ_Void();
            } ورنہ {
                iterateCheckSelf(nodep, "argument", طریقہ_Arg(nodep, 0), SELF, BOTH);
                Ast_گرہ_اظہاریہ* ثابت اشاریہ_exprp = طریقہ_Call_Wildcard_اشاریہ_اظہاریہ(nodep, adtypep);
                newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                          VCMethod::ASSOC_ERASE, اشاریہ_exprp->unlink_Fr_عقب()};
                newp->dtype_سیٹ_Void();
            }
        } ورنہ اگر (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ورنہ اگر (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = طریقہ_With_Clause(nodep, جھوٹ, جھوٹ, adtypep->subDTypep(),
                                   adtypep->ڈھونڈو_String_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->dtypep(قطار_DType_اشاریہ_دار_By(adtypep->subDTypep()));
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp
                = طریقہ_With_Clause(nodep, سچ, جھوٹ, nodep->ڈھونڈو_ثنائی_ہندسہ_DType(),
                                   adtypep->ڈھونڈو_String_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(قطار_DType_اشاریہ_دار_By(adtypep->subDTypep()));
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "map") {
            AstWith* ثابت withp
                = طریقہ_With_Clause(nodep, سچ, جھوٹ, adtypep->subDTypep(),
                                   adtypep->ڈھونڈو_String_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(قطار_DType_اشاریہ_دار_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        اگر (newp) {
            newp->protect(جھوٹ);
            newp->did_چوڑائی(سچ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    خالی طریقہ_Call_Assoc(Ast_طریقہ_Call* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = خالی_اشاریہ;
        اگر (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        اگر (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            طریقہ_Ok_Arguments(nodep, 0, 0);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::ASSOC_سائز};  // So don't need num()
            newp->dtype_سیٹ_Int();
        } ورنہ اگر (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", طریقہ_Arg(nodep, 0), adtypep->کلید_DTypep(), BOTH);
            Ast_گرہ_اظہاریہ* ثابت اشاریہ_exprp = طریقہ_Call_Assoc_اشاریہ_اظہاریہ(nodep, adtypep);
            طریقہ_Call_LValue_Recurse(nodep, اشاریہ_exprp, VAccess::READWRITE);
            newp
                = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                     VCMethod::array_طریقہ(nodep->name()),  // first/last/next/prev
                                     اشاریہ_exprp->unlink_Fr_عقب()};
            newp->dtype_سیٹ_Int();
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", طریقہ_Arg(nodep, 0), adtypep->کلید_DTypep(), BOTH);
            Ast_گرہ_اظہاریہ* ثابت اشاریہ_exprp = طریقہ_Call_Assoc_اشاریہ_اظہاریہ(nodep, adtypep);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::ASSOC_EXISTS, اشاریہ_exprp->unlink_Fr_عقب()};
            newp->dtype_سیٹ_ثنائی_ہندسہ_Unsized(32, 1, VSigning::SIGNED);
        } ورنہ اگر (nodep->name() == "delete") {  // function void delete([input integer index])
            طریقہ_Ok_Arguments(nodep, 0, 1);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            اگر (!nodep->argsp()) {
                newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_سیٹ_Void();
            } ورنہ {
                iterateCheckTyped(nodep, "argument", طریقہ_Arg(nodep, 0), adtypep->کلید_DTypep(),
                                  BOTH);
                Ast_گرہ_اظہاریہ* ثابت اشاریہ_exprp = طریقہ_Call_Assoc_اشاریہ_اظہاریہ(nodep, adtypep);
                newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                          VCMethod::ASSOC_ERASE, اشاریہ_exprp->unlink_Fr_عقب()};
                newp->dtype_سیٹ_Void();
            }
        } ورنہ اگر (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ورنہ اگر (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp = طریقہ_With_Clause(nodep, جھوٹ, جھوٹ, adtypep->subDTypep(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = طریقہ_With_Clause(
                nodep, جھوٹ, سچ, خالی_اشاریہ, nodep->ڈھونڈو_UInt32_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->withp(withp);
            اگر (nodep->name() == "unique_index") {
                newp->dtypep(قطار_DType_اشاریہ_دار_By(adtypep->کلید_DTypep()));
            } ورنہ {
                newp->dtypep(قطار_DType_اشاریہ_دار_By(adtypep->subDTypep()));
            }
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp = طریقہ_With_Clause(nodep, سچ, جھوٹ, nodep->ڈھونڈو_ثنائی_ہندسہ_DType(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(قطار_DType_اشاریہ_دار_By(adtypep->subDTypep()));
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp = طریقہ_With_Clause(nodep, سچ, جھوٹ, nodep->ڈھونڈو_ثنائی_ہندسہ_DType(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(قطار_DType_اشاریہ_دار_By(adtypep->کلید_DTypep()));
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "map") {
            AstWith* ثابت withp = طریقہ_With_Clause(nodep, سچ, جھوٹ, adtypep->subDTypep(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(قطار_DType_اشاریہ_دار_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        اگر (newp) {
            newp->protect(جھوٹ);
            newp->did_چوڑائی(سچ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    Ast_گرہ_اظہاریہ* طریقہ_Call_Assoc_اشاریہ_اظہاریہ(Ast_طریقہ_Call* nodep, AstAssocArrayDType* adtypep) {
        Ast_گرہ* ثابت اشاریہ_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", اشاریہ_exprp, CONTEXT_DET, FINAL, adtypep->کلید_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(اشاریہ_exprp);  // May have been edited
        واپس nodep->argsp()->exprp();
    }
    Ast_گرہ_اظہاریہ* طریقہ_Call_Wildcard_اشاریہ_اظہاریہ(Ast_طریقہ_Call* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_گرہ* ثابت اشاریہ_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", اشاریہ_exprp, CONTEXT_DET, FINAL, adtypep->ڈھونڈو_String_DType(),
                     EXTEND_EXP);
        VL_DANGLING(اشاریہ_exprp);  // May have been edited
        واپس nodep->argsp()->exprp();
    }
    خالی طریقہ_Call_LValue_Recurse(Ast_طریقہ_Call* nodep, Ast_گرہ* childp, ثابت VAccess& access) {
        اگر (AstCMethodHard* ثابت ichildp = VN_CAST(childp, CMethodHard)) {
            منطقی methAt = جھوٹ;
            منطقی meth_At_عقب = جھوٹ;
            منطقی methAtWrite = جھوٹ;
            انتخاب (ichildp->طریقہ()) {
            حالت VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = سچ;
                توڑو;
            حالت VCMethod::ARRAY_AT_عقب:  // FALLTHRU
                meth_At_عقب = سچ;
                توڑو;
            حالت VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            حالت VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            حالت VCMethod::DYN_AT_WRITE_APPEND_عقب:  //
                methAtWrite = سچ;
                توڑو;
            طےشدہ: توڑو;
            }
            اگر (methAt || meth_At_عقب || methAtWrite) {
                ثابت Ast_گرہ_DType* ثابت fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                اگر (VN_IS(fromDtypep, قطار_DType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    اگر (methAt) {
                        ichildp->طریقہ(VCMethod::ARRAY_AT_WRITE);
                    } ورنہ اگر (meth_At_عقب) {
                        ichildp->طریقہ(VCMethod::DYN_AT_WRITE_APPEND_عقب);
                    }
                }
                طریقہ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
                واپس;
            }
        }
        اگر (Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(childp, گرہ_Var_Ref)) {
            varrefp->access(access);
        } ورنہ اگر (ثابت AstMemberSel* ثابت ichildp = VN_CAST(childp, MemberSel)) {
            طریقہ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } ورنہ اگر (ثابت AstStructSel* ثابت ichildp = VN_CAST(childp, StructSel)) {
            طریقہ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } ورنہ اگر (ثابت Ast_گرہ_Sel* ثابت ichildp = VN_CAST(childp, گرہ_Sel)) {
            طریقہ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } ورنہ اگر (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } ورنہ {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* طریقہ_Call_Array(Ast_طریقہ_Call* nodep, Ast_گرہ_DType* adtypep) {
        AstCMethodHard* newp = خالی_اشاریہ;

        اگر (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = خالی_اشاریہ;
            اگر (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = طریقہ_With_Clause(nodep, جھوٹ, سچ, خالی_اشاریہ, nodep->ڈھونڈو_UInt32_DType(),
                                         adtypep->subDTypep());
            }
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->withp(withp);
            newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = طریقہ_With_Clause(
                nodep, جھوٹ, سچ, خالی_اشاریہ, nodep->ڈھونڈو_UInt32_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->withp(withp);
            اگر (nodep->name() == "unique_index") {
                newp->dtypep(newp->ڈھونڈو_قطار_اشاریہ_DType());
            } ورنہ {
                newp->dtypep(قطار_DType_اشاریہ_دار_By(adtypep->subDTypep()));
            }
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ثابت withp
                = طریقہ_With_Clause(nodep, سچ, جھوٹ, nodep->ڈھونڈو_ثنائی_ہندسہ_DType(),
                                   nodep->ڈھونڈو_UInt32_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(قطار_DType_اشاریہ_دار_By(adtypep->subDTypep()));
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp
                = طریقہ_With_Clause(nodep, سچ, جھوٹ, nodep->ڈھونڈو_ثنائی_ہندسہ_DType(),
                                   nodep->ڈھونڈو_UInt32_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->ڈھونڈو_قطار_اشاریہ_DType());
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ثابت withp
                = طریقہ_With_Clause(nodep, سچ, جھوٹ, adtypep->subDTypep(),
                                   nodep->ڈھونڈو_UInt32_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(قطار_DType_اشاریہ_دار_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        }
        واپس newp;
    }
    خالی طریقہ_Call_Dyn(Ast_طریقہ_Call* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = خالی_اشاریہ;
        اگر (nodep->name() == "at") {  // Created internally for []
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } ورنہ اگر (nodep->name() == "atWrite") {  // Created internally for []
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } ورنہ اگر (nodep->name() == "size") {
            طریقہ_Ok_Arguments(nodep, 0, 0);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::DYN_سائز};
            newp->dtype_سیٹ_Int();
        } ورنہ اگر (nodep->name() == "delete") {  // function void delete()
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::DYN_CLEAR};
            newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = طریقہ_With_Clause(nodep, جھوٹ, جھوٹ, adtypep->subDTypep(),
                                   nodep->ڈھونڈو_UInt32_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر ((newp = طریقہ_Call_Array(nodep, adtypep))) {
        } ورنہ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        اگر (newp) {
            newp->protect(جھوٹ);
            newp->did_چوڑائی(سچ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    خالی طریقہ_Call_قطار(Ast_طریقہ_Call* nodep, Ast_قطار_DType* adtypep) {
        AstCMethodHard* newp = خالی_اشاریہ;
        اگر (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ورنہ اگر (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ورنہ اگر (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            طریقہ_Ok_Arguments(nodep, 0, 0);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::DYN_سائز};
            newp->dtype_سیٹ_Int();
        } ورنہ اگر (nodep->name() == "delete") {  // function void delete([input integer index])
            طریقہ_Ok_Arguments(nodep, 0, 1);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            اگر (!nodep->argsp()) {
                newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                          VCMethod::DYN_CLEAR};
                newp->dtype_سیٹ_Void();
            } ورنہ {
                iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
                Ast_گرہ_اظہاریہ* ثابت اشاریہ_exprp = طریقہ_Call_قطار_اشاریہ_اظہاریہ(nodep);
                اگر (اشاریہ_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                              VCMethod::DYN_نکالو_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtype_سیٹ_Void();
                } ورنہ {
                    newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                              VCMethod::DYN_ERASE, اشاریہ_exprp->unlink_Fr_عقب()};
                    newp->dtype_سیٹ_Void();
                }
            }
        } ورنہ اگر (nodep->name() == "insert") {
            طریقہ_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", طریقہ_Arg(nodep, 0), BOTH);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_گرہ_اظہاریہ* ثابت اشاریہ_exprp = طریقہ_Call_قطار_اشاریہ_اظہاریہ(nodep);
            AstArg* ثابت argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            اگر (اشاریہ_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                          VCMethod::DYN_ڈالو_FRONT, argp->exprp()->unlink_Fr_عقب()};
                newp->dtype_سیٹ_Void();
            } ورنہ {
                newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                          VCMethod::DYN_داخل_کرو, اشاریہ_exprp->unlink_Fr_عقب()};
                newp->جوڑو_Pinsp(argp->exprp()->unlink_Fr_عقب());
                newp->dtype_سیٹ_Void();
            }
        } ورنہ اگر (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            طریقہ_Ok_Arguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ورنہ اگر (nodep->name() == "push_back" || nodep->name() == "push_front") {
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", طریقہ_Arg(nodep, 0), adtypep->subDTypep(), BOTH);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ثابت argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ(nodep->name()),
                                      argp->exprp()->unlink_Fr_عقب()};
            newp->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ثابت withp
                = طریقہ_With_Clause(nodep, جھوٹ, جھوٹ, adtypep->subDTypep(),
                                   nodep->ڈھونڈو_UInt32_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                      VCMethod::array_طریقہ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
        } ورنہ اگر ((newp = طریقہ_Call_Array(nodep, adtypep))) {
        } ورنہ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        اگر (newp) {
            newp->protect(جھوٹ);
            newp->did_چوڑائی(سچ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    Ast_گرہ_اظہاریہ* طریقہ_Call_قطار_اشاریہ_اظہاریہ(Ast_طریقہ_Call* nodep) {
        Ast_گرہ* ثابت اشاریہ_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", اشاریہ_exprp, BOTH);
        VL_DANGLING(اشاریہ_exprp);  // May have been edited
        واپس nodep->argsp()->exprp();
    }
    خالی طریقہ_Call_Warn_Timing(Ast_گرہ_FTask_Ref* ثابت nodep, ثابت std::string& className) {
        اگر (v3Global.opt.timing().is_سیٹ_False()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } ورنہ اگر (!v3Global.opt.timing().is_سیٹ_True()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    خالی طریقہ_Call_Iface_Ref(Ast_طریقہ_Call* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ثابت ifacep = adtypep->iface_بذریعہ_Cellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        اگر (Ast_گرہ_FTask* ثابت ftaskp
            = VN_CAST(m_memberMap.ڈھونڈو_Member(ifacep, nodep->name()), گرہ_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            اگر (!modport_Allows_شے(nodep, adtypep, ftaskp)) {
                nodep->dtype_سیٹ_Void();
                واپس;
            }
            اگر (adtypep->isVirtual()) {
                برائے (Ast_گرہ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    اگر (AstVar* ثابت mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, خالی_اشاریہ);
            اگر (ftaskp->isStatic()) {
                AstArg* ثابت argsp = nodep->argsp();
                اگر (argsp) argsp->unlink_Fr_عقب_With_اگلا();
                Ast_گرہ_FTask_Ref* newp = خالی_اشاریہ;
                اگر (VN_IS(ftaskp, Task)) {
                    newp = نیا AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } ورنہ {
                    newp = نیا AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            } ورنہ {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                اگر (VN_IS(ftaskp, Task)) nodep->dtype_سیٹ_Void();
                process_FTask_Ref_دلائل(nodep);
            }
            واپس;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    خالی handle_Randomize_دلائل(Ast_گرہ_FTask_Ref* ثابت nodep, AstClass* ثابت classp) {
        منطقی has_Non_Null_دلائل = جھوٹ;
        AstConst* nullp = خالی_اشاریہ;
        برائے (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = خالی_اشاریہ;
            Ast_گرہ_اظہاریہ* exprp = argp->exprp();
            اگر (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                اگر (constp->عدد().isNull()) {
                    nullp = constp;
                    جاری;
                }
            }
            has_Non_Null_دلائل = سچ;
            AstVar* fromVarp = خالی_اشاریہ;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            اگر (Ast_طریقہ_Call* طریقہ_Callp = VN_CAST(nodep, طریقہ_Call)) {
                Ast_گرہ_اظہاریہ* fromp = طریقہ_Callp->fromp();
                جب (AstMemberSel* ثابت memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ثابت varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            اگر (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_عقب()->delete_درخت(), argp);
                جاری;
            }
            جب (exprp) {
                اگر (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } ورنہ {
                    اگر (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } ورنہ {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlink_Fr_عقب()->delete_درخت(), argp);
                    }
                    exprp = خالی_اشاریہ;
                }
                // All variables in the dot hierarchy must be randomizable
                اگر (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            اگر (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } ورنہ اگر (classp->existsMember([&](ثابت AstClass*, ثابت AstVar* memberVarp) {
                           واپس memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } ورنہ {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_عقب()->delete_درخت(), argp);
            }
        }
        اگر (nullp && has_Non_Null_دلائل) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    خالی طریقہ_Call_Class(Ast_طریقہ_Call* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت پہلا_classp = adtypep->classp();
        AstWith* withp = خالی_اشاریہ;
        اگر (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = طریقہ_With_Clause(nodep, جھوٹ, جھوٹ, adtypep->ڈھونڈو_Void_DType(),
                                     adtypep->ڈھونڈو_ثنائی_ہندسہ_DType(), adtypep);
            برائے (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                اگر (argp->exprp()) userIterate(argp->exprp(), چوڑائی_VP{SELF, BOTH}.p());
            }
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, پہلا_classp);
            handle_Randomize_دلائل(nodep, پہلا_classp);
        } ورنہ اگر (nodep->name() == "srandom") {
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, پہلا_classp);
        }
        UASSERT_OBJ(پہلا_classp, nodep, "Unlinked");
        برائے (AstClass* classp = پہلا_classp; classp;) {
            اگر (nodep->fileline()->timingOn()) {
                اگر (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ثابت packagep = get_شے_Package(classp);
                    // Check if it's std
                    اگر (packagep && packagep->name() == "std") {
                        اگر (classp->name() == "process") {
                            طریقہ_Call_Warn_Timing(nodep, "process");
                        } ورنہ اگر (classp->name() == "semaphore" && nodep->name() == "get") {
                            طریقہ_Call_Warn_Timing(nodep, "semaphore");
                        } ورنہ اگر (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            طریقہ_Call_Warn_Timing(nodep, "mailbox");
                        }
                    }
                }
            }
            اگر (Ast_گرہ_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.ڈھونڈو_Member(classp, nodep->name()), گرہ_FTask)) {
                userIterate(ftaskp, خالی_اشاریہ);
                اگر (ftaskp->isStatic()) {
                    AstArg* ثابت argsp = nodep->argsp();
                    اگر (argsp) argsp->unlink_Fr_عقب_With_اگلا();
                    Ast_گرہ_FTask_Ref* newp = خالی_اشاریہ;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    اگر (!m_vup) {  // Called as task
                        newp = نیا AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } ورنہ {
                        newp = نیا AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                } ورنہ {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    اگر (VN_IS(ftaskp, Task)) {
                        اگر (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_اظہاریہ)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtype_سیٹ_Void();
                    }
                    اگر (withp) nodep->withp(withp);
                    process_FTask_Ref_دلائل(nodep);
                }
                واپس;
            } ورنہ اگر (nodep->name() == "get_randstate") {
                طریقہ_Ok_Arguments(nodep, 0, 0);
                پہلا_classp->بنیاد_Most_Classp()->needRNG(سچ);
                v3Global.useRandomizeMethods(سچ);
                AstCMethodHard* ثابت newp
                    = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                         VCMethod::RNG_GET_RANDSTATE, خالی_اشاریہ};
                newp->usePtr(سچ);
                newp->dtype_سیٹ_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            } ورنہ اگر (nodep->name() == "set_randstate") {
                طریقہ_Ok_Arguments(nodep, 1, 1);
                Ast_گرہ_اظہاریہ* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_گرہ_اظہاریہ* ثابت exprp = nodep->argsp()->exprp();
                پہلا_classp->بنیاد_Most_Classp()->needRNG(سچ);
                v3Global.useRandomizeMethods(سچ);
                AstCMethodHard* ثابت newp
                    = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                         VCMethod::RNG_سیٹ_RANDSTATE, exprp->unlink_Fr_عقب()};
                newp->usePtr(سچ);
                newp->dtype_سیٹ_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            } ورنہ اگر (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(سچ);
                nodep->dtypep(nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : خالی_اشاریہ;
        }
        {
            VSpellCheck speller;
            برائے (AstClass* classp = پہلا_classp; classp;) {
                برائے (Ast_گرہ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    اگر (VN_IS(itemp, گرہ_FTask)) speller.ڈالو_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : خالی_اشاریہ;
            }
            ثابت string suggest = speller.بہترین_Candidate_پیغام(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << پہلا_classp->verilogKwd() << " " << پہلا_classp->prettyNameQ()
                           << "\n"
                           << (suggest.خالی_خانے() ? "" : nodep->fileline()->warn_مزید() + suggest));
        }
        nodep->dtype_سیٹ_Int();  // Guess on error
    }
    خالی طریقہ_Call_Constraint(Ast_طریقہ_Call* nodep, AstConstraintRefDType*) {
        اگر (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            طریقہ_Ok_Arguments(nodep, 0, 1);
            اگر (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
                nodep->dtypep(nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::INT));
            } ورنہ {
                nodep->dtype_سیٹ_Void();
            }
            v3Global.useRandomizeMethods(سچ);
        } ورنہ {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    خالی طریقہ_Call_Rand_منوال(Ast_طریقہ_Call* nodep) {
        طریقہ_Ok_Arguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        اگر (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            nodep->dtype_سیٹ_Void();
        } ورنہ {
            nodep->dtypep(nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(سچ);
    }
    خالی طریقہ_Call_Unpack(Ast_طریقہ_Call* nodep, AstUnpackArrayDType* adtypep) {
        شمار : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_مجموع,
            ARRAY_حاصل_ضرب
        } طریقہ_Id;

        طریقہ_Id = UNKNOWN;
        اگر (nodep->name() == "or") {
            طریقہ_Id = ARRAY_OR;
        } ورنہ اگر (nodep->name() == "and") {
            طریقہ_Id = ARRAY_AND;
        } ورنہ اگر (nodep->name() == "xor") {
            طریقہ_Id = ARRAY_XOR;
        } ورنہ اگر (nodep->name() == "sum") {
            طریقہ_Id = ARRAY_مجموع;
        } ورنہ اگر (nodep->name() == "product") {
            طریقہ_Id = ARRAY_حاصل_ضرب;
        }

        اگر (طریقہ_Id) {
            AstWith* ثابت withp
                = طریقہ_With_Clause(nodep, جھوٹ, جھوٹ, adtypep->subDTypep(),
                                   nodep->ڈھونڈو_UInt32_DType(), adtypep->subDTypep());
            طریقہ_Ok_Arguments(nodep, 0, 0);
            اگر (withp) {
                طریقہ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ثابت newp
                    = نیا AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(),
                                         VCMethod::array_طریقہ("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                اگر (!nodep->پہلا_Abovep()) newp->dtype_سیٹ_Void();
                newp->protect(جھوٹ);
                newp->did_چوڑائی(سچ);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                واپس;
            } ورنہ {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_سطر* ثابت fl = nodep->fileline();
                Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
                برائے (صحیح i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_گرہ_اظہاریہ* ثابت arrayRef = nodep->fromp()->clone_درخت_خالص(جھوٹ);
                    Ast_گرہ_اظہاریہ* ثابت selector = نیا AstArraySel{fl, arrayRef, i};
                    اگر (!newp) {
                        newp = selector;
                    } ورنہ {
                        انتخاب (طریقہ_Id) {
                        حالت ARRAY_OR: newp = نیا AstOr{fl, newp, selector}; توڑو;
                        حالت ARRAY_AND: newp = نیا AstAnd{fl, newp, selector}; توڑو;
                        حالت ARRAY_XOR: newp = نیا AstXor{fl, newp, selector}; توڑو;
                        حالت ARRAY_مجموع: newp = نیا Ast_جوڑو{fl, newp, selector}; توڑو;
                        حالت ARRAY_حاصل_ضرب: newp = نیا AstMul{fl, newp, selector}; توڑو;
                        طےشدہ: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            }
        } ورنہ اگر (AstCMethodHard* newp = طریقہ_Call_Array(nodep, adtypep)) {
            newp->protect(جھوٹ);
            newp->did_چوڑائی(سچ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    خالی طریقہ_Call_Event(Ast_طریقہ_Call* nodep, AstBasicDType*) {
        // Method call on event
        اگر (nodep->name() == "triggered") {
            طریقہ_Ok_Arguments(nodep, 0, 0);
            AstCMethodHard* ثابت callp = نیا AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtype_سیٹ_ثنائی_ہندسہ();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } ورنہ {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    خالی طریقہ_Call_String(Ast_طریقہ_Call* nodep, AstBasicDType*) {
        // Method call on string
        اگر (nodep->name() == "len") {
            // Constant value
            طریقہ_Ok_Arguments(nodep, 0, 0);
            Ast_گرہ* ثابت newp = نیا AstLenN{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } ورنہ اگر (nodep->name() == "itoa") {
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } ورنہ اگر (nodep->name() == "hextoa") {
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } ورنہ اگر (nodep->name() == "octtoa") {
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } ورنہ اگر (nodep->name() == "bintoa") {
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } ورنہ اگر (nodep->name() == "realtoa") {
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } ورنہ اگر (nodep->name() == "tolower") {
            طریقہ_Ok_Arguments(nodep, 0, 0);
            Ast_گرہ* ثابت newp
                = نیا Ast_مقصد_زیریں_N{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ اگر (nodep->name() == "toupper") {
            طریقہ_Ok_Arguments(nodep, 0, 0);
            Ast_گرہ* ثابت newp
                = نیا Ast_مقصد_بالائی_N{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ اگر (nodep->name() == "compare" || nodep->name() == "icompare") {
            ثابت منطقی ignoreCase = nodep->name()[0] == 'i';
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            AstArg* ثابت argp = nodep->argsp();
            Ast_گرہ_اظہاریہ* ثابت lhs = nodep->fromp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت rhs = argp->exprp()->unlink_Fr_عقب();
            Ast_گرہ* ثابت newp = نیا AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ اگر (nodep->name() == "putc") {
            طریقہ_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", طریقہ_Arg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", طریقہ_Arg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_گرہ_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_عقب(), VarRef);
            Ast_گرہ_اظہاریہ* ثابت rhsp = arg0p->exprp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت thsp = arg1p->exprp()->unlink_Fr_عقب();
            AstVarRef* ثابت varrefp
                = نیا AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_گرہ* ثابت newp = نیا Ast_تفویض{
                nodep->fileline(), fromp, نیا AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ڈالو_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } ورنہ اگر (nodep->name() == "getc") {
            طریقہ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", طریقہ_Arg(nodep, 0), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->fromp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت rhsp = arg0p->exprp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ اگر (nodep->name() == "substr") {
            طریقہ_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", طریقہ_Arg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", طریقہ_Arg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->fromp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت rhsp = arg0p->exprp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت thsp = arg1p->exprp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ اگر (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            اگر (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } ورنہ اگر (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } ورنہ اگر (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } ورنہ اگر (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } ورنہ اگر (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } ورنہ {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            طریقہ_Ok_Arguments(nodep, 0, 0);
            Ast_گرہ* ثابت newp
                = نیا AstAtoN{nodep->fileline(), nodep->fromp()->unlink_Fr_عقب(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    Ast_قطار_DType* قطار_DType_اشاریہ_دار_By(Ast_گرہ_DType* اشاریہ_DTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        اگر (Ast_قطار_DType* ثابت queuep = m_قطار_DType_اشاریہ_دار[اشاریہ_DTypep]) {
            واپس queuep;
        } ورنہ {
            Ast_قطار_DType* ثابت newp
                = نیا Ast_قطار_DType{اشاریہ_DTypep->fileline(), اشاریہ_DTypep, خالی_اشاریہ};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(newp);
            m_قطار_DType_اشاریہ_دار[اشاریہ_DTypep] = newp;
            واپس newp;
        }
    }

    خالی دورہ(AstNew* nodep) override {
        اگر (nodep->did_چوڑائی()) واپس;
        AstClass* classp = خالی_اشاریہ;
        منطقی تفویض_2 = جھوٹ;
        اگر (VN_IS(nodep->backp(), تفویض)) {  // assignment case
            تفویض_2 = سچ;
            Ast_گرہ* warnp = خالی_اشاریہ;
            AstClassRefDType* refp = خالی_اشاریہ;

            اگر (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                اگر (AstClass* ثابت clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ثابت adtypep
                        = نیا AstClassRefDType{nodep->fileline(), clsp, خالی_اشاریہ};
                    v3Global.rootp()->typeTablep()->جوڑو_Typesp(adtypep);
                    refp = adtypep;
                }
            } ورنہ {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : خالی_اشاریہ;
            }
            اگر (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, گرہ_DType)
                           ? ("data type "s + VN_AS(warnp, گرہ_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                واپس;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            اگر (Ast_گرہ_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.ڈھونڈو_Member(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } ورنہ {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } ورنہ {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        process_FTask_Ref_دلائل(nodep);
        اگر (!تفویض_2) nodep->dtypeFrom(nodep->taskp());
    }
    خالی دورہ(AstNewCopy* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        AstClassRefDType* ثابت refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        اگر (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            واپس;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
        اگر (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    خالی دورہ(AstNewDynamic* nodep) override {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        AstDynArrayDType* ثابت adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        اگر (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            واپس;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        اگر (!VN_IS(nodep->backp(), گرہ_تفویض)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            واپس;
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
                consp->sizep(nodep->sizep()->unlink_Fr_عقب());
                nodep->replaceWith(consp->unlink_Fr_عقب());
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            }
        }
    }

    خالی دورہ(Ast_Tagged_اظہاریہ* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtype_سیٹ_ثنائی_ہندسہ();
        userIterateChildren(nodep, m_vup);
    }
    خالی دورہ(Ast_Tagged_نمونہ* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtype_سیٹ_ثنائی_ہندسہ();
        userIterateChildren(nodep, m_vup);
    }
    خالی دورہ(Ast_نمونہ_Var* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtype_سیٹ_ثنائی_ہندسہ();
    }
    خالی دورہ(Ast_نمونہ_ستارہ* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtype_سیٹ_ثنائی_ہندسہ();
    }
    خالی دورہ(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtype_سیٹ_ثنائی_ہندسہ();
        userIterateChildren(nodep, m_vup);
    }

    خالی دورہ(Ast_نمونہ* nodep) override {
        اگر (m_اندر_Param_Override && !nodep->بچہ_DTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_اندر_Param_Override);
            برائے (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_اندر_Param_Override = جھوٹ;
                user_Iterate_And_اگلا(patp->repp(), چوڑائی_VP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                اگر (!VN_IS(patp->keyp(), متن_2) && !VN_IS(patp->keyp(), گرہ_DType)) {
                    user_Iterate_And_اگلا(patp->keyp(), چوڑائی_VP{SELF, BOTH}.p());
                }
                برائے (Ast_گرہ *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_اندر_Param_Override = VN_IS(valuep, نمونہ);
                    userIterate(valuep, چوڑائی_VP{SELF, BOTH}.p());
                }
            }
            واپس;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_اندر_Param_Override);
        m_اندر_Param_Override = جھوٹ;
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        UINFO(9, "PATTERN " << nodep);
        اگر (nodep->بچہ_DTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        اگر (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_گرہ_DType* dtypep = nodep->dtypep();
        اگر (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            اگر (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                واپس;
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            اگر (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                واپس;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            برائے (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ثابت صحیح times = دورہ_Pat_Member_Rep(patp);
                برائے (صحیح i = 1; i < times; i++) {
                    AstPatMember* ثابت newp = patp->clone_درخت(جھوٹ);
                    patp->جوڑو_اگلا_یہاں(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            برائے (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                اگر (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlink_Fr_عقب(&relinkHandle);
                    جب (Ast_گرہ_اظہاریہ* ثابت movep = VN_AS(patp->lhssp()->nextp(), گرہ_اظہاریہ)) {
                        movep->unlink_Fr_عقب();  // Not unlinkFrBackWithNext, just one
                        Ast_گرہ* newkeyp = خالی_اشاریہ;
                        اگر (patp->keyp()) newkeyp = patp->keyp()->clone_درخت(سچ);
                        AstPatMember* ثابت newp
                            = نیا AstPatMember{patp->fileline(), movep, newkeyp, خالی_اشاریہ};
                        patp->جوڑو_اگلا(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = خالی_اشاریہ;
            برائے (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ثابت nextp = VN_AS(patp->nextp(), PatMember);
                اگر (patp->isDefault()) {
                    اگر (defaultp) {
                        VL_DO_DANGLING(ڈالو_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlink_Fr_عقب();
                }
                patp = nextp;
            }
            جب (ثابت AstConstDType* ثابت vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, چوڑائی_VP{SELF, BOTH}.p());

            اگر (خودکار* ثابت vdtypep = VN_CAST(dtypep, StructDType)) {
                نمونہ_Struct(nodep, vdtypep, defaultp);
            } ورنہ اگر (خودکار* ثابت vdtypep = VN_CAST(dtypep, گرہ_Array_DType)) {
                نمونہ_Array(nodep, vdtypep, defaultp);
            } ورنہ اگر (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ثابت consp = نیا AstConsAssoc{nodep->fileline()};
                نمونہ_Keyed<Ast_Cons_Assoc_شے>(nodep, consp, dtypep, defaultp);
            } ورنہ اگر (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ثابت consp = نیا AstConsWildcard{nodep->fileline()};
                نمونہ_Keyed<Ast_Cons_Wildcard_شے>(nodep, consp, dtypep, defaultp);
            } ورنہ اگر (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, قطار_DType)) {
                نمونہ_Dyn_Array_Or_قطار(nodep, dtypep, defaultp);
            } ورنہ اگر (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                نمونہ_Basic(nodep, dtypep, defaultp);
            } ورنہ {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            اگر (defaultp) VL_DO_DANGLING(ڈالو_Deletep(defaultp), defaultp);
        }
    }
    خالی نمونہ_Struct(Ast_نمونہ* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        استعمال PatMap = std::map<ثابت AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ثابت AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            جب (patp) {
                کرو {
                    اگر (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        اگر (ثابت Ast_متن* textp = VN_CAST(patp->keyp(), متن_2)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.ڈھونڈو_Member(vdtypep, textp->متن()),
                                           MemberDType);
                            اگر (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->متن() << "' not found as member");
                                توڑو;
                            } ورنہ {
                                ثابت std::pair<PatMap::iterator, منطقی> ret
                                    = patmap.emplace(memp, patp);
                                اگر (!ret.دوسرا) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), متن_2)->متن());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } ورنہ اگر (Ast_گرہ_DType* nodedtypep = VN_CAST(patp->keyp(), گرہ_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, چوڑائی_VP{SELF, BOTH}.p());
                            ثابت string dtype = nodedtypep->dtypep()->prettyDTypeName(سچ);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } ورنہ {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } ورنہ اگر (memp) {
                        // constant expr
                        ثابت std::pair<PatMap::iterator, منطقی> ret = patmap.emplace(memp, patp);
                        اگر (!ret.دوسرا) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), متن_2)->متن());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } ورنہ {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } جب (جھوٹ);

                //اگلا
                اگر (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
        اگر (vdtypep->packed()) {
            برائے (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت خودکار it = patmap.ڈھونڈو(memp);
                اگر (it == patmap.انتہا()) {  // default or default_type assignment
                    اگر (AstStructDType* ثابت memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalue_Concat_نمونہ_Struct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } ورنہ {
                        AstPatMember* ثابت patp
                            = default_Patp_نمونہ_Struct(nodep, memp, defaultp, dtypemap);
                        newp = قدر_Concat_نمونہ_Struct(patp, newp, memp, nodep);
                        اگر (patp) VL_DO_DANGLING(ڈالو_Deletep(patp), patp);
                    }
                } ورنہ {  // member assignment
                    newp = قدر_Concat_نمونہ_Struct(it->دوسرا, newp, memp, nodep);
                }
            }
        } ورنہ {  // Unpacked
            AstConsStruct* ثابت consp = نیا AstConsStruct{nodep->fileline(), vdtypep};
            برائے (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت خودکار it = patmap.ڈھونڈو(memp);
                AstPatMember* patp = خالی_اشاریہ;
                اگر (it == patmap.انتہا()) {  // Default or default_type assignment
                    patp = default_Patp_نمونہ_Struct(nodep, memp, defaultp, dtypemap);
                    ڈالو_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ڈالو_Deletep(patp);  // patp used below
                } ورنہ {
                    patp = it->دوسرا;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_گرہ_اظہاریہ* ثابت valuep = نمونہ_Member_قدر_Iterate(patp);
                consp->جوڑو_Itemsp(نیا Ast_Cons_Struct_شے{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        اگر (newp) {
            nodep->replaceWith(newp);
        } ورنہ {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_گرہ_اظہاریہ* nestedvalue_Concat_نمونہ_Struct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_گرہ_اظہاریہ* newp,
                                                 Ast_نمونہ* nodep, ثابت DTypeMap& dtypemap) {
        برائے (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            اگر (AstStructDType* ثابت memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalue_Concat_نمونہ_Struct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } ورنہ {
                AstPatMember* ثابت patp
                    = default_Patp_نمونہ_Struct(nodep, memp_nested, defaultp, dtypemap);
                newp = قدر_Concat_نمونہ_Struct(patp, newp, memp_nested, nodep);
                اگر (patp) VL_DO_DANGLING(ڈالو_Deletep(patp), patp);
            }
        }
        واپس newp;
    }

    AstPatMember* default_Patp_نمونہ_Struct(Ast_نمونہ* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ثابت DTypeMap& dtypemap) {
        ثابت string memp_DType = memp->virtRefDTypep()->prettyDTypeName(سچ);
        ثابت خودکار it = dtypemap.ڈھونڈو(memp_DType);
        اگر (it != dtypemap.انتہا()) {
            // default_value for data_type
            واپس it->دوسرا->clone_درخت(جھوٹ);
        }
        اگر (defaultp) {
            // default_value for any unmatched member yet
            واپس defaultp->clone_درخت(جھوٹ);
        }
        اگر (memp->valuep()) {
            واپس نیا AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_درخت(جھوٹ), گرہ_اظہاریہ),
                                    نیا Ast_متن{nodep->fileline(), memp->name()}, خالی_اشاریہ};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        واپس خالی_اشاریہ;
    }

    Ast_گرہ_اظہاریہ* قدر_Concat_نمونہ_Struct(AstPatMember* patp, Ast_گرہ_اظہاریہ* newp,
                                           AstMemberDType* memp, Ast_نمونہ* nodep) {
        اگر (patp) {
            patp->dtypep(memp);
            Ast_گرہ_اظہاریہ* ثابت valuep = نمونہ_Member_قدر_Iterate(patp);
            اگر (!newp) {
                newp = valuep;
            } ورنہ {
                AstConcat* ثابت concatp = نیا AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3_Error_رمز::WIDTHCONCAT, سچ);
                valuep->fileline()->modifyWarnOff(V3_Error_رمز::WIDTHCONCAT, سچ);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        واپس newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_گرہ_DType* elemDTypep,
                                       ثابت DTypeMap& dtypemap = DTypeMap{}) {
        Ast_گرہ_اظہاریہ* ثابت valuep = defaultp->lhssp()->clone_درخت(جھوٹ);
        Ast_گرہ_DType* ثابت elemDTypeSkipRefp = elemDTypep->skipRefp();
        ثابت AstStructDType* ثابت structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ثابت منطقی unpackedStruct = structp && !structp->packed();

        اگر (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
            واپس defaultp->clone_درخت(جھوٹ);
        }
        اگر (VN_IS(valuep, نمونہ)) {
            VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
            واپس defaultp->clone_درخت(جھوٹ);
        }
        اگر (!valuep->dtypep()) userIterate(valuep, چوڑائی_VP{SELF, BOTH}.p());
        منطقی پورا_Element = unpackedStruct;
        اگر (Ast_گرہ_DType* ثابت قدر_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            پورا_Element = unpackedStruct ? !قدر_DTypep->skipRefp()->isIntegralOrPacked()
                                          : قدر_DTypep->similarDType(elemDTypeSkipRefp);
        }
        اگر (پورا_Element) {
            VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
            واپس defaultp->clone_درخت(جھوٹ);
        }

        AstPatMember* ثابت nestedDefaultp
            = نیا AstPatMember{defaultp->fileline(), valuep, خالی_اشاریہ, خالی_اشاریہ};
        nestedDefaultp->isDefault(سچ);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        برائے (ثابت خودکار& اندراج_2 : dtypemap) nestedDefaultp->جوڑو_اگلا(اندراج_2.دوسرا->clone_درخت(جھوٹ));
        Ast_نمونہ* ثابت recursivePatternp = نیا Ast_نمونہ{defaultp->fileline(), nestedDefaultp};
        واپس نیا AstPatMember{defaultp->fileline(), recursivePatternp, خالی_اشاریہ, خالی_اشاریہ};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_گرہ_اظہاریہ* نمونہ_Default_Valuep(AstPatMember* defaultp, Ast_گرہ_DType* elemDTypep) {
        اگر (!defaultp) واپس خالی_اشاریہ;
        AstPatMember* ثابت elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_گرہ_اظہاریہ* ثابت valuep = نمونہ_Member_قدر_Iterate(elemDefaultp);
        VL_DO_DANGLING(ڈالو_Deletep(elemDefaultp), elemDefaultp);
        واپس valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    خالی نمونہ_Concat_Array(Ast_نمونہ* nodep, AstUnpackArrayDType* arrayDtp) {
        ثابت VNumRange range = arrayDtp->declRange();
        ثابت صحیح elements = range.elements();
        AstConsArray* ثابت newp = نیا AstConsArray{nodep->fileline(), arrayDtp};
        صحیح entn = 0;  // Number of elements given so far, from the left
        منطقی error = جھوٹ;  // Reported an error, so the value is not complete
        برائے (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_گرہ_اظہاریہ* ثابت valuep = نمونہ_Member_قدر_Iterate(patp);
            ثابت uint64_t اشاریہ = range.ascending() ? entn : elements - 1 - entn;
            ثابت AstUnpackArrayDType* ثابت spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            اگر (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_اقدار(اشاریہ, valuep);
                جاری;
            }
            ثابت VCastable castable = Ast_گرہ::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            اگر (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = سچ;
            }
            newp->valuep(اشاریہ, valuep);
            ++entn;
        }
        اگر (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = سچ;
        }
        اگر (error) {
            VL_DO_DANGLING(ڈالو_Deletep(newp), newp);
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
        } ورنہ {
            nodep->replaceWith(newp);
        }
    }
    خالی نمونہ_Array(Ast_نمونہ* nodep, Ast_گرہ_Array_DType* arrayDtp, AstPatMember* defaultp) {
        اگر (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            نمونہ_Concat_Array(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            واپس;
        }
        ثابت VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = pat_سمتیہ_Map(nodep, range);
        UINFO(9, "ent " << range.بائیں() << " to " << range.دائیں());
        Ast_گرہ* newp = خالی_اشاریہ;
        منطقی missed = جھوٹ;  // Some element missed, so not a complete value
        برائے (صحیح entn = 0, ent = range.بائیں(); entn < range.elements();
             ++entn, ent += range.بایاں_مقصد_دایاں_Inc()) {
            Ast_گرہ_اظہاریہ* valuep = خالی_اشاریہ;
            ثابت خودکار it = patmap.ڈھونڈو(ent);
            اگر (it == patmap.انتہا()) {
                اگر (defaultp) {
                    valuep = نمونہ_Default_Valuep(defaultp, arrayDtp->subDTypep());
                } ورنہ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = سچ;
                }
            } ورنہ {
                AstPatMember* ثابت patp = it->دوسرا;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = نمونہ_Member_قدر_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ثابت AstUnpackArrayDType* ثابت memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                اگر (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warn_مزید()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
                    valuep = نیا AstConst{patp->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}};
                }
            }

            اگر (valuep) {
                اگر (VN_IS(arrayDtp, UnpackArrayDType)) {
                    اگر (!newp) newp = نیا AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } ورنہ {  // Packed. Convert to concat for now.
                    اگر (!newp) {
                        newp = valuep;
                    } ورنہ {
                        AstConcat* ثابت concatp
                            = نیا AstConcat{valuep->fileline(), VN_AS(newp, گرہ_اظہاریہ), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        اگر (!patmap.خالی_خانے()) nodep->v3error("Assignment pattern with too many elements");
        اگر (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ڈالو_Deletep(newp), newp);
            newp = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}};
        }
        اگر (newp) {
            nodep->replaceWith(newp);
        } ورنہ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* نمونہ_Const_Keyp(AstPatMember* patp) {
        اگر (VN_IS(patp->keyp(), گرہ_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            واپس خالی_اشاریہ;
        }
        اگر (patp->کلید_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->کلید_Exprp());  // May change
        AstConst* ثابت constp = VN_CAST(patp->کلید_Exprp(), Const);
        اگر (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        واپس constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* نمونہ_Scalar_Keyp(AstPatMember* patp, Ast_گرہ_DType* کلید_DTypep) {
        اگر (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            واپس خالی_اشاریہ;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        اگر (کلید_DTypep->skipRefp()->isIntegralOrPacked() && patp->کلید_Exprp()) {
            userIterate(patp->کلید_Exprp(), چوڑائی_VP{کلید_DTypep, PRELIM}.p());
            ثابت صحیح چوڑائی = std::max(patp->کلید_Exprp()->چوڑائی(), کلید_DTypep->چوڑائی());
            Ast_گرہ_DType* ثابت contextDTypep
                = کلید_DTypep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(چوڑائی, چوڑائی, کلید_DTypep->numeric(), سچ);
            iterateCheck(patp, "key", patp->کلید_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, جھوٹ);
        }
        AstConst* ثابت constp = نمونہ_Const_Keyp(patp);
        اگر (!constp) واپس خالی_اشاریہ;
        اگر (constp->عدد().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            واپس خالی_اشاریہ;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, کلید_DTypep, EXTEND_EXP, جھوٹ);
        Ast_گرہ_اظہاریہ* ثابت convp = patp->کلید_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_گرہ_اظہاریہ* ثابت keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        واپس VN_AS(keyp->unlink_Fr_عقب(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_گرہ_اظہاریہ* نمونہ_Keyp(AstConsAssoc*, AstPatMember* patp, Ast_گرہ_DType* arrayDtp) {
        Ast_گرہ_DType* ثابت کلید_DTypep = VN_AS(arrayDtp, AssocArrayDType)->کلید_DTypep();
        اگر (!کلید_DTypep->skipRefp()->isAggregateType() || !patp->کلید_Exprp()) {
            واپس نمونہ_Scalar_Keyp(patp, کلید_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->کلید_Exprp(), کلید_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->کلید_Exprp());
        Ast_گرہ_اظہاریہ* ثابت foldedp = patp->کلید_Exprp();
        اگر (!foldedp->is_Constant_درخت()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            واپس خالی_اشاریہ;
        }
        اگر (foldedp->exists([](ثابت AstConst* constp) { واپس constp->عدد().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            واپس خالی_اشاریہ;
        }
        واپس foldedp->unlink_Fr_عقب();
    }
    AstConst* نمونہ_Keyp(AstConsWildcard*, AstPatMember* patp, Ast_گرہ_DType* arrayDtp) {
        واپس نمونہ_Scalar_Keyp(patp, arrayDtp->ڈھونڈو_String_DType());
    }
    AstConst* نمونہ_Keyp(AstConsDynArray*, AstPatMember* patp, Ast_گرہ_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        واپس نمونہ_Scalar_Keyp(patp, arrayDtp->ڈھونڈو_Basic_DType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    سانچہ <قسم_نام T_شے, قسم_نام T_Cons>
    خالی ترتیب_دو_Cons_اشیاء(T_Cons* nodep) {
        std::سمتیہ<T_شے*> itemps;
        جب (T_شے* ثابت itemp = nodep->itemsp()) {
            itemp->unlink_Fr_عقب();
            itemps.ڈالو_عقب(itemp);
        }
        ثابت خودکار کلید_کا = [](ثابت T_شے* itemp) -> ثابت V3_نمبر& {
            ثابت Ast_گرہ* ثابت keyp = itemp->keyp();
            واپس VN_AS(keyp, Const)->عدد();
        };
        std::stable_ترتیب_دو(itemps.begin(), itemps.انتہا(), [&](ثابت T_شے* ap, ثابت T_شے* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            واپس کلید_کا(ap).isLtXZ(کلید_کا(bp));
        });
        ثابت T_شے* firstp = خالی_اشاریہ;  // First item with the current key
        برائے (T_شے* ثابت itemp : itemps) {
            اگر (firstp && کلید_کا(firstp).isCaseEq(کلید_کا(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } ورنہ {
                firstp = itemp;
            }
            nodep->جوڑو_Itemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    سانچہ <قسم_نام T_شے, قسم_نام T_Cons>
    خالی نمونہ_Keyed(Ast_نمونہ* nodep, T_Cons* consp, Ast_گرہ_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(نمونہ_Default_Valuep(defaultp, arrayDtp->subDTypep()));
        برائے (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ثابت خودکار keyp = نمونہ_Keyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_گرہ_اظہاریہ* ثابت valuep = نمونہ_Member_قدر_Iterate(patp);
            اگر (keyp) {
                consp->جوڑو_Itemsp(نیا T_شے{patp->fileline(), keyp, valuep});
            } ورنہ {
                VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ثابت AstAssocArrayDType* ثابت assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        اگر (!assocDtp || !assocDtp->کلید_DTypep()->skipRefp()->isAggregateType()) {
            ترتیب_دو_Cons_اشیاء<T_شے>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_گرہ_اظہاریہ* new_Cons_Dyn_Array_Or_قطار(File_سطر* fileline, ثابت Ast_گرہ_DType* dtypep) {
        اگر (VN_IS(dtypep, DynArrayDType)) واپس نیا AstConsDynArray{fileline};
        اگر (VN_IS(dtypep, قطار_DType)) واپس نیا Ast_Cons_قطار{fileline};
        واپس خالی_اشاریہ;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    سانچہ <قسم_نام T_شے, قسم_نام T_Cons>
    خالی جوڑو_Cons_شے(T_Cons* consp, Ast_گرہ_اظہاریہ* valuep) {
        Ast_گرہ_DType* ثابت arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ثابت منطقی isElement
            = Ast_گرہ::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), خالی_اشاریہ)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ثابت Ast_قطار_DType* ثابت sub_قطار_Dtp = VN_CAST(valuep->dtypep()->skipRefp(), قطار_DType);
        اگر (!isElement && !(sub_قطار_Dtp && sub_قطار_Dtp->boundp())) {
            T_Cons* ثابت subp = Ast_گرہ::cast<T_Cons>(valuep);
            اگر (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                اگر (subp->itemsp()) consp->جوڑو_Itemsp(subp->itemsp()->unlink_Fr_عقب_With_اگلا());
                VL_DO_DANGLING(ڈالو_Deletep(subp), subp);
                واپس;
            }
            اگر (Ast_Cons_قطار* ثابت queuep = VN_CAST(valuep, Cons_قطار)) {
                جوڑو_Cons_اشیاء(consp, queuep);
                واپس;
            }
            اگر (AstConsDynArray* ثابت dynp = VN_CAST(valuep, ConsDynArray)) {
                جوڑو_Cons_اشیاء(consp, dynp);
                واپس;
            }
        }
        T_شے* ثابت itemp = نیا T_شے{valuep->fileline(), valuep};
        consp->جوڑو_Itemsp(itemp);
        // Width the value now, as constructors are widthed when created
        اگر (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } ورنہ {
            user_Iterate_And_اگلا(valuep, چوڑائی_VP{arrayDtp, FINAL}.p());
        }
    }
    خالی جوڑو_Cons_شے(Ast_گرہ_اظہاریہ* consp, Ast_گرہ_اظہاریہ* valuep) {
        اگر (AstConsDynArray* ثابت dynp = VN_CAST(consp, ConsDynArray)) {
            جوڑو_Cons_شے<Ast_Cons_Dyn_Array_شے>(dynp, valuep);
        } ورنہ {
            جوڑو_Cons_شے<Ast_Cons_قطار_شے>(VN_AS(consp, Cons_قطار), valuep);
        }
    }
    سانچہ <قسم_نام T_Sub>
    خالی جوڑو_Cons_اشیاء(Ast_گرہ_اظہاریہ* consp, T_Sub* subp) {
        جب (خودکار* ثابت itemp = subp->itemsp()) {
            Ast_گرہ_اظہاریہ* ثابت valuep = itemp->valuep()->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(itemp->unlink_Fr_عقب()), itemp);
            جوڑو_Cons_شے(consp, valuep);
        }
        VL_DO_DANGLING(ڈالو_Deletep(subp), subp);
    }
    خالی نمونہ_Dyn_Array_Or_قطار(Ast_نمونہ* nodep, Ast_گرہ_DType* arrayp, AstPatMember* defaultp) {
        Ast_گرہ_اظہاریہ* ثابت newp = new_Cons_Dyn_Array_Or_قطار(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ثابت AstPatMember* ثابت firstp = VN_AS(nodep->itemsp(), PatMember);
        ثابت منطقی keyed = defaultp || (firstp && firstp->keyp());
        اگر (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            نمونہ_Keyed<Ast_Cons_Dyn_Array_شے>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            واپس;
        }
        ثابت حرف* ثابت kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        اگر (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        برائے (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            اگر (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            جوڑو_Cons_شے(newp, نمونہ_Member_قدر_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    خالی نمونہ_Basic(Ast_نمونہ* nodep, Ast_گرہ_DType* vdtypep, AstPatMember* defaultp) {
        ثابت AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ثابت VNumRange range = bdtypep->declRange();
        PatVecMap patmap = pat_سمتیہ_Map(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
        برائے (ثابت صحیح ent : range.seq_Hi_مقصد_Lo()) {
            Ast_گرہ_اظہاریہ* valuep = خالی_اشاریہ;
            ثابت خودکار it = patmap.ڈھونڈو(ent);
            اگر (it == patmap.انتہا()) {
                اگر (defaultp) {
                    valuep = نمونہ_Default_Valuep(defaultp, nodep->ڈھونڈو_ثنائی_ہندسہ_DType());
                } ورنہ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } ورنہ {
                AstPatMember* ثابت patp = it->دوسرا;
                patmap.erase(it);
                patp->dtypep(nodep->ڈھونڈو_ثنائی_ہندسہ_DType());
                valuep = نمونہ_Member_قدر_Iterate(patp);
            }
            اگر (valuep) {  // Packed. Convert to concat for now.
                اگر (!newp) {
                    newp = valuep;
                } ورنہ {
                    AstConcat* ثابت concatp = نیا AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        اگر (!patmap.خالی_خانے()) nodep->v3error("Assignment pattern with too many elements");
        اگر (newp) {
            nodep->replaceWith(newp);
        } ورنہ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_گرہ_اظہاریہ* نمونہ_Member_قدر_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, چوڑائی_VP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_گرہ_اظہاریہ* valuep = patp->lhssp()->unlink_Fr_عقب();
        اگر (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            اگر (AstConst* ثابت newp = V3_چوڑائی_Commit::new_If_Const_Commit_سائز(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        واپس valuep;
    }

    ساکن خالی checkEventAssignment(ثابت Ast_گرہ_تفویض* ثابت asgnp) {
        string unsupEvtAsgn;
        اگر (!استعمال_کرتا_Dynamic_Scheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        اگر (VN_IS(asgnp->rhsp(), CReset)) واپس;
        اگر (asgnp->rhsp()->dtypep()->isEvent() && !استعمال_کرتا_Dynamic_Scheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.خالی_خانے() ? "from" : " and from");
        }
        اگر (!unsupEvtAsgn.خالی_خانے()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warn_مزید()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warn_مزید()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    ساکن منطقی استعمال_کرتا_Dynamic_Scheduler(Ast_گرہ* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        جب (سچ) {
            AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef);
            اگر (vrefp) واپس استعمال_کرتا_Dynamic_Scheduler(vrefp);
            اگر (VN_IS(nodep, MemberSel)) {
                واپس سچ;
            } ورنہ اگر (Ast_گرہ_Sel* selp = VN_CAST(nodep, گرہ_Sel)) {
                nodep = selp->fromp();
            } ورنہ {
                واپس جھوٹ;
            }
        }
    }

    ساکن منطقی استعمال_کرتا_Dynamic_Scheduler(AstVarRef* vrefp) {
        واپس VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    خالی دورہ(AstPatMember* nodep) override {
        Ast_گرہ_DType* ثابت vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, چوڑائی_VP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), تفویض_3, FINAL, vdtypep, EXTEND_LHS);
    }
    صحیح دورہ_Pat_Member_Rep(AstPatMember* nodep) {
        uint32_t times = 1;
        اگر (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ثابت AstConst* ثابت constp = VN_CAST(nodep->repp(), Const);
            اگر (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } ورنہ {
                times = constp->مقصد_UInt();
            }
            اگر (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlink_Fr_عقب_With_اگلا()
                ->delete_درخت();  // Done with replicate before cloning
        }
        واپس times;
    }

    خالی دورہ(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_گرہ* propp = nodep->propp();
            جب (AstLogNot* ثابت notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ثابت untilp = VN_CAST(propp, Until);
            اگر (untilp && VN_IS(nodep->backp(), گرہ_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().is_سیٹ_False() || !v3Global.opt.timing().is_سیٹ_True())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(نیا AstConst{untilp->fileline(), AstConst::ثنائی_ہندسہ_False{}});
                VL_DO_DANGLING(ڈالو_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            user_Iterate_And_اگلا(nodep->sensesp(), خالی_اشاریہ);
            اگر (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }

    منطقی پہلا_New_Statement_Ok_Recurse(Ast_گرہ* nodep) {
        اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            اگر (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) واپس سچ;
        }
        اگر (Ast_تفویض* ثابت aitemp = VN_CAST(nodep, تفویض)) {
            اگر (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) واپس سچ;
        }
        اگر (AstInitialStaticStmt* ثابت aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            برائے (Ast_گرہ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (!پہلا_New_Statement_Ok_Recurse(stmtp)) واپس جھوٹ;
            }
            واپس سچ;
        }
        اگر (AstInitialAutomaticStmt* ثابت aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            برائے (Ast_گرہ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (!پہلا_New_Statement_Ok_Recurse(stmtp)) واپس جھوٹ;
            }
            واپس سچ;
        }
        واپس جھوٹ;
    }

    //--------------------
    // Top levels

    سانچہ <قسم_نام Case_شے>
    خالی handleCaseType(Ast_گرہ* casep, Ast_گرہ_اظہاریہ* exprp, Case_شے* itemsp) {
        Ast_Attr_کا* ثابت exprap = VN_CAST(exprp, Attr_کا);
        اگر (!exprap) واپس;
        اگر (exprap->attrType() != VAttrType::TYPEID) واپس;

        ثابت Ast_گرہ_DType* ثابت اظہاریہ_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << اظہاریہ_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_سطر* ثابت newfl = casep->fileline();
        newfl->warnOff(V3_Error_رمز::CASEINCOMPLETE, سچ);  // Side effect of transform
        newfl->warnOff(V3_Error_رمز::CASEOVERLAP, سچ);  // Side effect of transform
        casep->fileline(newfl);
        برائے (Case_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<Case_شے>(itemp->nextp())) {
            اگر (itemp->isDefault()) جاری;
            منطقی کامیابی = جھوٹ;
            برائے (Ast_گرہ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ثابت Ast_Attr_کا* ثابت condAttrp = VN_CAST(condp, Attr_کا);
                اگر (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } ورنہ {
                    Ast_گرہ_DType* ثابت condDtp = condAttrp->dtypep();
                    اگر (Ast_گرہ::computeCastable(اظہاریہ_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        کامیابی = سچ;
                        توڑو;
                    }
                }
            }
            ڈالو_Deletep(itemp->condsp()->unlink_Fr_عقب_With_اگلا());
            // Item condition becomes constant 1 if hits else 0
            itemp->جوڑو_Condsp(نیا AstConst{newfl, AstConst::ثنائی_ہندسہ_True{}, کامیابی});
        }
        exprap->replaceWith(نیا AstConst{newfl, AstConst::ثنائی_ہندسہ_True{}});
        VL_DO_DANGLING(ڈالو_Deletep(exprap), exprap);
    }

    سانچہ <قسم_نام Case_شے>
    خالی handleCase(Ast_گرہ* casep, Ast_گرہ_اظہاریہ* exprp, Case_شے* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_گرہ_DType* subDTypep = exprp->dtypep();
        برائے (Case_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<Case_شے>(itemp->nextp())) {
            برائے (Ast_گرہ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                اگر (condp->dtypep() == subDTypep) جاری;

                اگر (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->ڈھونڈو_Double_DType();
                } ورنہ اگر (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->ڈھونڈو_String_DType();
                } ورنہ {
                    ثابت صحیح چوڑائی = std::max(subDTypep->چوڑائی(), condp->چوڑائی());
                    ثابت صحیح mwidth = std::max(subDTypep->چوڑائی_Min(), condp->چوڑائی_Min());
                    ثابت منطقی issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                        چوڑائی, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        برائے (Case_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<Case_شے>(itemp->nextp())) {
            برائے (Ast_گرہ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    خالی دورہ(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
        برائے (Ast_Gen_Case_شے *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), Gen_Case_شے);
            برائے (Ast_گرہ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, چوڑائی_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    خالی دورہ(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_اگلا(nodep->initsp(), خالی_اشاریہ);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        user_Iterate_And_اگلا(nodep->incsp(), خالی_اشاریہ);
    }
    خالی دورہ(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    خالی دورہ(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
        برائے (Ast_Case_شے *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_شے);  // Prelim may cause the node to get replaced
            user_Iterate_And_اگلا(itemp->stmtsp(), خالی_اشاریہ);
            برائے (Ast_گرہ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, چوڑائی_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    خالی دورہ(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_گرہ_DType* ثابت شے_DTypep = nodep->ڈھونڈو_UInt32_DType();
        برائے (Ast_Case_شے *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_شے);  // Prelim may cause the node to get replaced
            user_Iterate_And_اگلا(itemp->stmtsp(), خالی_اشاریہ);
            برائے (Ast_گرہ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, شے_DTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    خالی دورہ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        user_Iterate_And_اگلا(nodep->stmtsp(), خالی_اشاریہ);
    }
    خالی دورہ(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    خالی دورہ(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    خالی دورہ(Ast_گرہ_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        user_Iterate_And_اگلا(nodep->thensp(), خالی_اشاریہ);
        user_Iterate_And_اگلا(nodep->elsesp(), خالی_اشاریہ);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        user_Iterate_And_اگلا(nodep->stmtsp(), خالی_اشاریہ);
        // expected result is same as parent's expected result
        user_Iterate_And_اگلا(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    خالی دورہ(Ast_گرہ_Foreach* nodep) override {
        اگر (nodep->did_چوڑائی()) واپس;
        nodep->did_چوڑائی(سچ);
        ثابت Ast_Foreach_سرخی* ثابت headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        user_Iterate_And_اگلا(headerp->fromp(), چوڑائی_VP{SELF, BOTH}.p());
        Ast_گرہ_اظہاریہ* ثابت fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_گرہ_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        برائے (Ast_گرہ *argsp = headerp->elementsp(), *اگلا_argsp; argsp; argsp = اگلا_argsp) {
            اگلا_argsp = argsp->nextp();
            ثابت منطقی خالی_خانے = VN_IS(argsp, خالی_خانے_2);
            AstVar* ثابت varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || خالی_خانے, argsp, "Missing foreach loop variable");
            اگر (varp) varp->مستعمل_Loop_Idx(سچ);
            اگر (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
                واپس;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            اگر (VN_IS(fromDtp, گرہ_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, قطار_DType)) {
                // Nothing special here
            } ورنہ اگر (AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                اگر (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
                    واپس;
                }
            } ورنہ اگر (ثابت AstAssocArrayDType* ثابت adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->کلید_DTypep());
            } ورنہ {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
                واپس;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_گرہ* ثابت bodyp = nodep->bodyp();
        user_Iterate_And_اگلا(bodyp, خالی_اشاریہ);
        اگر (AstForeach* ثابت loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convert_مقصد_While(loopp), loopp, nodep);
            واپس;
        }
    }

    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
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
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->چوڑائی_Sized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            اگر (ثابت AstIfaceRefDType* ثابت irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                اگر (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                    واپس;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            دھارا_تفویض_زیریں(nodep);
            Ast_گرہ_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            اگر (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ثابت Ast_گرہ_DType* ثابت lhsDtp = lhsDTypep->skipRefp();
                ثابت Ast_گرہ_DType* ثابت rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
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
                checkUnpackedArrayAssignmentCompatible<Ast_گرہ_Var_Ref, Ast_گرہ_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), گرہ_Var_Ref), VN_CAST(nodep->rhsp(), گرہ_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                اگر (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), گرہ_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterate_Check_تفویض(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        اگر (VN_IS(nodep, تفویض_Force)) checkForceReleaseLhs(nodep, nodep->lhsp());
        اگر (Ast_گرہ* ثابت controlp = nodep->timingControlp()) {
            اگر (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlink_Fr_عقب_With_اگلا()->delete_درخت(), controlp);
            } ورنہ اگر (nodep->fileline()->timingOn() && v3Global.opt.timing().is_سیٹ_True()) {
                iterateNull(controlp);
            } ورنہ {
                اگر (nodep->fileline()->timingOn()) {
                    اگر (v3Global.opt.timing().is_سیٹ_False()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } ورنہ {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlink_Fr_عقب_With_اگلا()->delete_درخت(), controlp);
            }
        }
        اگر (VN_IS(nodep->rhsp(), خالی_خانے_قطار)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ثابت Ast_گرہ_DType* ثابت lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            اگر (!VN_IS(lhsDtp, قطار_DType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
            Ast_طریقہ_Call* ثابت newp
                = نیا Ast_طریقہ_Call{nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب(), "delete"};
            newp->dtype_سیٹ_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            // Need to now convert it
            دورہ(newp);
            واپس;
        }
        اگر (ثابت AstNewDynamic* ثابت dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            اگر (!dynp->rhsp()) {
                newp = نیا AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlink_Fr_عقب()};
            } ورنہ {
                newp = نیا AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlink_Fr_عقب()};
                newp->جوڑو_Pinsp(dynp->rhsp()->unlink_Fr_عقب());
            }
            newp->did_چوڑائی(سچ);
            newp->protect(جھوٹ);
            newp->dtype_سیٹ_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        اگر (AstCMethodHard* ثابت slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod تفویض_طریقہ;
            اگر (slicep->طریقہ() == VCMethod::DYN_قاش) {
                تفویض_طریقہ = VCMethod::DYN_قاش_تفویض;
            } ورنہ اگر (slicep->طریقہ() == VCMethod::DYN_قاش_عقب_عقب) {
                تفویض_طریقہ = VCMethod::DYN_قاش_تفویض_عقب_عقب;
            } ورنہ اگر (slicep->طریقہ() == VCMethod::DYN_قاش_FRONT_عقب) {
                تفویض_طریقہ = VCMethod::DYN_قاش_تفویض_FRONT_عقب;
            } ورنہ {
                تفویض_طریقہ = VCMethod::_ENUM_MAX;  // not a slice
            }
            اگر (تفویض_طریقہ != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_گرہ_اظہاریہ* ثابت fromp = slicep->fromp()->unlink_Fr_عقب();
                // Collect existing slice index pins (lsb, msb)
                Ast_گرہ_اظہاریہ* ثابت lsbp = slicep->pinsp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* ثابت msbp = slicep->pinsp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
                AstCMethodHard* ثابت newp
                    = نیا AstCMethodHard{nodep->fileline(), fromp, تفویض_طریقہ};
                newp->جوڑو_Pinsp(lsbp);
                newp->جوڑو_Pinsp(msbp);
                newp->جوڑو_Pinsp(rhsp);
                newp->did_چوڑائی(سچ);
                newp->protect(جھوٹ);
                newp->dtype_سیٹ_Void();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            }
        }

        اگر (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.سیٹ_Assigns_Events();
        }
    }

    خالی دورہ(AstRSRule* nodep) override {
        اگر (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        user_Iterate_And_اگلا(nodep->prodlistsp(), خالی_اشاریہ);
        user_Iterate_And_اگلا(nodep->وزن_Stmtsp(), خالی_اشاریہ);
    }

    خالی دورہ(AstRelease* nodep) override {
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->چوڑائی_Sized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    خالی دورہ(AstDeassign* nodep) override {
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->چوڑائی_Sized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    ساکن منطقی isFormatNonNumericArg(ثابت Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        واپس dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, قطار_DType)  //
               || (VN_IS(dtypep, گرہ_UOr_Struct_DType)
                   && !VN_AS(dtypep, گرہ_UOr_Struct_DType)->packed());
    }

    خالی checkFormatNumericArg(Ast_گرہ* argp, حرف ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        اگر (std::tolower(ch) == 'd') واپس;
        // In contrast, %x of a class reference, causes errors on several simulators.
        اگر (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warn_مزید() << "... Suggest use '%s'");
    }

    خالی دورہ(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    خالی دورہ(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_اگلا(nodep->fmtp(), چوڑائی_VP{SELF, BOTH}.p());
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(Ast_مقصد_String_N* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtype_سیٹ_String();
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    خالی دورہ(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        اگر (m_vup && !m_vup->prelim()) واپس;  // Can be called as statement or function
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;
        //
        UINFO(9, "  Display in " << nodep->متن());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
        اگر (!nodep->اظہاریہ_Format_2()) {  // Non-constant format, can't check format string types
            دورہ_sformatf_format(nodep);
        }

        Ast_گرہ_اظہاریہ* oldExprsp = nodep->exprsp();
        اگر (nodep->اظہاریہ_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), گرہ_اظہاریہ);
        }
        اگر (oldExprsp) oldExprsp->unlink_Fr_عقب_With_اگلا();
        جب (Ast_گرہ_اظہاریہ* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), گرہ_اظہاریہ);
            اگر (oldExprsp) oldExprsp->unlink_Fr_عقب_With_اگلا();
            اگر (VN_IS(argp, SFormatArg)) {
                nodep->جوڑو_Exprsp(argp);
                جاری;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_گرہ_DType* ثابت dtypep = argp->dtypep()->skipRefp();
            اگر (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } ورنہ اگر (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } ورنہ اگر (isFormatNonNumericArg(dtypep)) {
                اگر (AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    اگر (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(سچ);
                    }
                } ورنہ {
                    // Class handles inside containers print type names, not their members.
                    Ast_گرہ_DType* گرہ_Dtypep = dtypep;
                    جب (گرہ_Dtypep) {
                        گرہ_Dtypep = گرہ_Dtypep->skipRefp();
                        اگر (Ast_گرہ_UOr_Struct_DType* ثابت uOrStructDTypep
                            = VN_CAST(گرہ_Dtypep, گرہ_UOr_Struct_DType)) {
                            uOrStructDTypep->سیٹ_Emit_مقصد_String();
                            v3Global.hasPrintedObjects(سچ);
                        }
                        گرہ_Dtypep = گرہ_Dtypep->subDTypep();
                    }
                }
                Ast_گرہ_اظہاریہ* ثابت newp = نیا Ast_مقصد_String_N{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } ورنہ اگر (nodep->اظہاریہ_Format_2()) {
                اگر (AstEnumDType* ثابت enumDtp = formatEnumDType(argp)) {
                    nodep->جوڑو_Exprsp(newFormatEnumArg(argp, enumDtp));
                    جاری;
                }
            }
            اگر (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            اگر (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->جوڑو_Exprsp(argp);
            } ورنہ {
                nodep->جوڑو_Exprsp(نیا AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->متن());
    }
    خالی دورہ(AstCReset* nodep) override {
        assert_At_اظہاریہ(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    خالی دورہ(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    خالی دورہ(AstConstraintRef* nodep) override { userIterateChildren(nodep, خالی_اشاریہ); }
    خالی دورہ(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        اگر (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
        اگر (!m_paramsOnly) {
            nodep->foreach([یہ](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    نیا AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string متن = VString::dequotePercent(nodep->fmtp()->متن());
            اگر (متن.خالی_خانے()) متن = "Elaboration system task message (IEEE 1800-2023 20.11)";
            انتخاب (nodep->displayType()) {
            حالت VDisplayType::DT_INFO: nodep->v3warn(USERINFO, متن); توڑو;
            حالت VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, متن); توڑو;
            حالت VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, متن); توڑو;
            حالت VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, متن); توڑو;
            طےشدہ: UASSERT_OBJ(جھوٹ, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        }
    }
    خالی دورہ(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        اگر (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    خالی دورہ(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    خالی دورہ(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    خالی دورہ(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    خالی دورہ(AstFError* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            user_Iterate_And_اگلا(nodep->strp(), چوڑائی_VP{SELF, BOTH}.p());
        }
    }
    خالی دورہ(AstFEof* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    خالی دورہ(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        اگر (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    خالی دورہ(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    خالی دورہ(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    خالی دورہ(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->انحراف(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    خالی دورہ(AstFGetC* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    خالی دورہ(AstFGetS* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_اگلا(nodep->strgp(), چوڑائی_VP{SELF, BOTH}.p());
        }
    }
    خالی دورہ(AstFUngetC* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    خالی دورہ(AstFRead* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->memp(), چوڑائی_VP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            اگر (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            اگر (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    خالی دورہ(AstFScanF* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_اگلا(nodep->exprsp(), چوڑائی_VP{SELF, BOTH}.p());
        }
    }
    خالی دورہ(AstSScanF* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->fromp(), چوڑائی_VP{SELF, BOTH}.p());
            user_Iterate_And_اگلا(nodep->exprsp(), چوڑائی_VP{SELF, BOTH}.p());
        }
    }
    خالی دورہ(Ast_ڈھیر_Trace_F* nodep) override { nodep->dtype_سیٹ_String(); }
    خالی دورہ(AstSysIgnore* nodep) override {
        user_Iterate_And_اگلا(nodep->exprsp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstSystemF* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(Ast_گرہ_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        user_Iterate_And_اگلا(nodep->memp(), چوڑائی_VP{SELF, BOTH}.p());
        ثابت Ast_گرہ_DType* subp = خالی_اشاریہ;
        اگر (ثابت AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            اگر (!adtypep->کلید_DTypep()->skipRefp()->basicp()
                || !adtypep->کلید_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } ورنہ اگر (ثابت AstUnpackArrayDType* ثابت adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } ورنہ اگر (ثابت AstDynArrayDType* ثابت adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } ورنہ {
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
        user_Iterate_And_اگلا(nodep->lsbp(), چوڑائی_VP{SELF, BOTH}.p());
        user_Iterate_And_اگلا(nodep->msbp(), چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(Ast_Test_Plus_دلائل* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    خالی دورہ(Ast_قدر_Plus_دلائل* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            user_Iterate_And_اگلا(nodep->outp(), چوڑائی_VP{SELF, BOTH}.p());
        }
    }
    خالی دورہ(Ast_وقت_Format* nodep) override {
        assertAtStatement(nodep);
        اگر (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        اگر (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        اگر (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        اگر (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    خالی دورہ(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_اگلا(nodep->passsp(), خالی_اشاریہ);
        user_Iterate_And_اگلا(nodep->failsp(), خالی_اشاریہ);
    }
    خالی دورہ(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
    }
    خالی دورہ(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_اگلا(nodep->passsp(), خالی_اشاریہ);
        user_Iterate_And_اگلا(nodep->failsp(), خالی_اشاریہ);
    }
    خالی دورہ(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_اگلا(nodep->passsp(), خالی_اشاریہ);
    }
    خالی دورہ(AstRestrict* nodep) override {
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
        Ast_گرہ_DType* ثابت modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_گرہ_DType* ثابت conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        اگر (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) واپس سچ;
        // Types don't match, but all dimensions done, still need the usual checks
        اگر (!rangep) واپس جھوٹ;

        ثابت AstRange* ثابت subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        صحیح subElems = 1;
        برائے (ثابت AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ثابت صحیح elements = rangep->elementsConst();
        ثابت صحیح from_بایاں = idx / subElems;

        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ثابت uint32_t conDims = conDTypep->dimensions(جھوٹ).دوسرا;
        ثابت uint32_t modDims = modDTypep->dimensions(جھوٹ).دوسرا;

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
                user_Iterate_And_اگلا(nodep->exprp(),
                                   چوڑائی_VP{conDTypep, FINAL, دھارا_USE_تفویض}.p());
                واپس سچ;
            }
            // ArraySel index is counted from lo
            ثابت صحیح ثنائی_ہندسہ_2 = arrp->declRange().ascending() ? from_بایاں : elements - 1 - from_بایاں;
            // Connection is self-determined
            user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{conDTypep, FINAL, دھارا_USE_تفویض}.p());
            Ast_گرہ_اظہاریہ* ثابت fromp = VN_AS(nodep->exprp(), گرہ_اظہاریہ)->unlink_Fr_عقب();
            اگر (Ast_گرہ_اظہاریہ* ثابت elemp = newIfaceArrayElement(fromp, ثنائی_ہندسہ_2)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ڈالو_Deletep(fromp), fromp);
            } ورنہ {
                AstArraySel* ثابت selp = نیا AstArraySel{fromp->fileline(), fromp, ثنائی_ہندسہ_2};
                selp->did_چوڑائی(سچ);
                nodep->exprp(selp);
            }
            واپس pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        اگر (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) واپس جھوٹ;

        // Packed connection
        ثابت صحیح modwidth = modDTypep->چوڑائی();
        ثابت صحیح conwidth = conDTypep->چوڑائی();

        // Connection has the width of the port, connect it to every element
        اگر (conwidth == modwidth) واپس جھوٹ;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        اگر (conwidth == modwidth * elements * subElems) {
            اگر (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->بایاں_Const() << ":"
                                                     << rangep->دایاں_Const() << "]");
            }
            ثابت صحیح selwidth = modwidth * subElems;
            // Connection is self-determined
            user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{conDTypep, FINAL, دھارا_USE_تفویض}.p());
            Ast_گرہ_اظہاریہ* ثابت fromp = VN_AS(nodep->exprp(), گرہ_اظہاریہ)->unlink_Fr_عقب();
            AstSel* ثابت selp = نیا AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_بایاں), selwidth};
            selp->did_چوڑائی(سچ);
            nodep->exprp(selp);
            واپس pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{conDTypep, FINAL, دھارا_USE_تفویض}.p());
        واپس سچ;
    }

    خالی دورہ(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        اگر (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ثابت concatp = VN_CAST(nodep->exprp(), Replicate);
            ثابت AstConst* ثابت countp = concatp ? VN_CAST(concatp->countp(), Const) : خالی_اشاریہ;
            ثابت Ast_گرہ_DType* ثابت dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            اگر (countp && countp->مقصد_UInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                Ast_نمونہ* ثابت patternp = نیا Ast_نمونہ{concatp->fileline(), خالی_اشاریہ};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ڈالو_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_اندر_Param_Override);
            m_اندر_Param_Override = is_Override_نمونہ(nodep->exprp());
            userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
        } ورنہ اگر (!m_paramsOnly) {
            اگر (!nodep->modVarp()->did_چوڑائی()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), خالی_اشاریہ);
            }
            اگر (!nodep->exprp()) {  // No-connect
                اگر (nodep->modVarp()->is_Iface_Array_بانٹو()) expandIfaceArrayPin(nodep);
                واپس;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ثابت VDirection pinDirection = nodep->modVarp()->direction();
            اگر (VN_IS(nodep->exprp(), گرہ_دھارا) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_گرہ_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_گرہ_DType* conDTypep = nodep->exprp()->dtypep();
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
            Ast_گرہ_DType* subDTypep = modDTypep;
            ثابت صحیح modwidth = modDTypep->چوڑائی();
            صحیح conwidth = conDTypep->چوڑائی();
            اگر (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                user_Iterate_And_اگلا(nodep->exprp(),
                                   چوڑائی_VP{subDTypep, FINAL, دھارا_USE_تفویض}.p());
            } ورنہ {
                اگر (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    اگر (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        اگر (nodep->modVarp()->is_Iface_Array_بانٹو()) expandIfaceArrayPin(nodep);
                        واپس;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->چوڑائی();
                }
                اگر (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } ورنہ اگر (nodep->modVarp()->isTristate()) {
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
                } ورنہ اگر (nodep->modVarp()->direction().isWritable()
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
                ثابت Ast_گرہ_Array_DType* ثابت اظہاریہ_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ثابت Ast_گرہ_Array_DType* ثابت modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                اگر (اظہاریہ_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpacked_Array_سائز(conDTypep) != unpacked_Array_سائز(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpacked_Array_سائز(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpacked_Array_سائز(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ورنہ اگر ((اظہاریہ_Arrayp && !modArrayp) || (!اظہاریہ_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (اظہاریہ_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ورنہ {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_گرہ_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), گرہ_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterate_Check_تفویض(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            اگر (nodep->modVarp()->is_Iface_Array_بانٹو()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    خالی دورہ(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        اگر (!m_paramsOnly) {
            اگر (VN_IS(nodep->modp(), Not_ملا_Module)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_راستہ_Looked_پیغام(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            اگر (nodep->rangep()) user_Iterate_And_اگلا(nodep->rangep(), چوڑائی_VP{SELF, BOTH}.p());
            user_Iterate_And_اگلا(nodep->pinsp(), خالی_اشاریہ);
            // If element of an instance array, pins are now connected, no longer need the ranges
            اگر (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ڈالو_Deletep(nodep->rangep()->unlink_Fr_عقب_With_اگلا());
            }
        }
        user_Iterate_And_اگلا(nodep->paramsp(), خالی_اشاریہ);
    }
    خالی دورہ(AstGatePin* nodep) override {
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->rangep(), چوڑائی_VP{SELF, BOTH}.p());
            user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ثابت Ast_گرہ_DType* ثابت pinDTypep = nodep->exprp()->dtypep();
            ثابت صحیح عدد_Insts = nodep->rangep()->elementsConst();
            ثابت صحیح modwidth = عدد_Insts;
            ثابت صحیح pinwidth = pinDTypep->چوڑائی();
            اگر (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_گرہ_DType* ثابت subDTypep = nodep->ڈھونڈو_Logic_DType(1, 1, pinDTypep->numeric());
                user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{subDTypep, FINAL}.p());
                Ast_گرہ* ثابت newp
                    = نیا AstReplicate{nodep->fileline(), nodep->exprp()->unlink_Fr_عقب(),
                                       static_cast<uint32_t>(عدد_Insts)};
                nodep->replaceWith(newp);
            } ورنہ {
                اگر (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                user_Iterate_And_اگلا(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlink_Fr_عقب());
            }
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        اگر (nodep->did_چوڑائی()) واپس;
        اگر (nodep->doing_چوڑائی()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(سچ);
            nodep->did_چوڑائی(سچ);
            واپس;
        }
        اگر (nodep->class_طریقہ() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } ورنہ اگر (nodep->class_طریقہ() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        اگر (nodep->class_طریقہ() && nodep->name() == "new") {
            اگر (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            اگر (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_گرہ* firstp = خالی_اشاریہ;
            برائے (Ast_گرہ* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                اگر (Ast_Stmt_اظہاریہ* ثابت sep = VN_CAST(itemp, Stmt_اظہاریہ)) {
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
                            توڑو;
                        }
                    }
                    جاری;
                }
                اگر (پہلا_New_Statement_Ok_Recurse(itemp)) جاری;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doing_چوڑائی(سچ);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        اگر (nodep->fvarp()) userIterate(nodep->fvarp(), خالی_اشاریہ);
        اگر (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtype_سیٹ_Void();
        } ورنہ اگر (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } ورنہ اگر (VN_IS(nodep, Property)) {
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
        چوڑائی_VP* vup = خالی_اشاریہ;
        اگر (VN_IS(nodep, Property)) vup = چوڑائی_VP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->did_چوڑائی(سچ);
        nodep->doing_چوڑائی(جھوٹ);
        اگر (nodep->dpiImport() && !nodep->dpi_Open_والد() && markHasOpenArray(nodep)) {
            nodep->dpi_Open_والد_Inc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        اگر (nodep->fvarp() && !nodep->dpiImport() && !nodep->خالص_Virtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->exists_And_اگلا([&](ثابت AstVarRef* varrefp) {
                   واپس varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::سمتیہ<AstVar*> ports;
        std::سمتیہ<Ast_Attr_کا*> protos;
        Ast_Attr_کا* protop = خالی_اشاریہ;  // Base prototype
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_عقب(portp); }
            } ورنہ اگر (Ast_Attr_کا* ثابت attrp = VN_CAST(stmtp, Attr_کا)) {
                اگر (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_عقب(attrp); }
                اگر (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        اگر (protop) {
            برائے (سائز_t i = 0; i < std::max(ports.سائز(), protos.سائز()); ++i) {
                اگر (i >= ports.سائز() || i >= protos.سائز()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    توڑو;
                } ورنہ {
                    AstVar* ثابت portp = ports[i];
                    Ast_Attr_کا* ثابت rprotop = protos[i];
                    Ast_گرہ_DType* ثابت declDtp = portp->dtypep();
                    Ast_گرہ_DType* ثابت protoDtp = rprotop->fromp()->dtypep();
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
                    } ورنہ اگر (!similarDTypeRecurse(protoDtp, declDtp)) {
                        protoDtp->v3warn(
                            PROTOTYPEMIS,
                            "In prototype for "
                                << nodep->prettyNameQ() << ", argument " << portp->prettyNameQ()
                                << " data-type does not match out-of-block"
                                   " declaration's data-type (IEEE 1800-2023 8.24)\n"
                                << protoDtp->warn_مزید() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warn_مزید() << "... Declaration data type: "
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
    خالی دورہ(AstConstraint* nodep) override {
        اگر (nodep->did_چوڑائی()) واپس;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, خالی_اشاریہ);
        nodep->did_چوڑائی(سچ);
    }
    خالی دورہ(AstProperty* nodep) override {
        اگر (nodep->did_چوڑائی()) واپس;
        اگر (nodep->doing_چوڑائی()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(سچ);
            nodep->did_چوڑائی(سچ);
            واپس;
        }
        nodep->doing_چوڑائی(سچ);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtype_سیٹ_ثنائی_ہندسہ();
        برائے (Ast_گرہ* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            اگر (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, خالی_اشاریہ);
            } ورنہ اگر (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } ورنہ اگر (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, خالی_اشاریہ);
            } ورنہ {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->did_چوڑائی(سچ);
        nodep->doing_چوڑائی(جھوٹ);
    }
    خالی دورہ(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    خالی دورہ(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        اگر (nodep->did_چوڑائی()) واپس;
        nodep->dtype_سیٹ_ثنائی_ہندسہ();
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (VN_IS(stmtp, Var)) {
                userIterate(stmtp, خالی_اشاریہ);
            } ورنہ اگر (VN_IS(stmtp, گرہ_اظہاریہ)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } ورنہ {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->did_چوڑائی(سچ);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* get_شے_Package(Ast_گرہ* pkgItemp) {
        جب (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        واپس VN_CAST(pkgItemp->backp(), Package);
    }
    خالی دورہ(AstFuncRef* nodep) override {
        دورہ(static_cast<Ast_گرہ_FTask_Ref*>(nodep));
        اگر (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            اگر (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_اظہاریہ))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtype_سیٹ_Void();
        } ورنہ {
            nodep->dtypeFrom(nodep->taskp());
        }
        اگر (nodep->fileline()->timingOn()) {
            Ast_گرہ_Module* ثابت classp = nodep->classOrPackagep();
            اگر (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ثابت packagep = get_شے_Package(classp);
                اگر (packagep && packagep->name() == "std") {
                    طریقہ_Call_Warn_Timing(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    ساکن منطقی are_Same_سائز(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ثابت std::سمتیہ<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ثابت std::سمتیہ<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        اگر (dims0.سائز() != dims1.سائز()) واپس جھوٹ;
        برائے (سائز_t i = 0; i < dims0.سائز(); ++i) {
            اگر (dims0[i]->elementsConst() != dims1[i]->elementsConst()) واپس جھوٹ;
        }
        واپس سچ;
    }
    // Makes sure that port and pin have same size and same datatype
    خالی check_Unpacked_Array_دلائل(AstVar* portp, Ast_گرہ* pinp) {
        اگر (AstUnpackArrayDType* ثابت پورٹ_Dtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            اگر (AstUnpackArrayDType* ثابت pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                اگر (!are_Same_سائز(پورٹ_Dtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << پورٹ_Dtypep->prettyDTypeNameQ() << ")");
                }
                ثابت AstBasicDType* ثابت پورٹ_Basicp = پورٹ_Dtypep->basicp();
                ثابت AstBasicDType* ثابت pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ثابت منطقی sameElemDType
                    = پورٹ_Basicp && pinBasicp
                          ? پورٹ_Basicp->چوڑائی() == pinBasicp->چوڑائی()
                                && پورٹ_Basicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(پورٹ_Dtypep->elemDTypep());
                اگر (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, پورٹ_Dtypep, "Argument", "Parameter"));
                }
            } ورنہ {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    خالی process_FTask_Ref_دلائل(Ast_گرہ_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        اگر (nodep->did_چوڑائی()) واپس;
        userIterate(nodep->taskp(), خالی_اشاریہ);
        //
        // And do the arguments to the task/function too
        کرو {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ثابت V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            اگر (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            برائے (ثابت خودکار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.پہلا;
                AstArg* ثابت argp = tconnect.دوسرا;
                Ast_گرہ_اظہاریہ* pinp = argp->exprp();
                اگر (!pinp) جاری;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                اگر (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlink_Fr_عقب_With_اگلا(&handle);  // Format + additional args, if any
                    Ast_گرہ_اظہاریہ* argsp = خالی_اشاریہ;
                    جب (AstArg* ثابت nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->جوڑو_اگلا(nextargp->exprp()->unlink_Fr_عقب_With_اگلا());
                        nextargp->unlink_Fr_عقب()->delete_درخت();  // Remove the call's Arg wrapper
                    }
                    string format;
                    اگر (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->عدد().مقصد_String();
                    } ورنہ {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ڈالو_Deletep(argp), argp);
                    AstSFormatF* ثابت newp
                        = نیا AstSFormatF{nodep->fileline(), format, جھوٹ, argsp};
                    اگر (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(نیا AstScopeName{newp->fileline(), سچ});
                    }
                    handle.relink(نیا AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    جاؤ reloop;  // so exit early; next loop will correct it
                }  //
                ورنہ اگر (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlink_Fr_عقب(&handle);  // No next, that's the next pin
                    Ast_گرہ_اظہاریہ* ثابت newp = نیا AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                اگر (portp->isWritable()) {
                    V3LinkLValue::link_LValue_سیٹ(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, چوڑائی_VP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } جب (جھوٹ);
        // Stage 2
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            برائے (ثابت خودکار& tconnect : tconnects) {
                AstVar* ثابت portp = tconnect.پہلا;
                ثابت AstArg* ثابت argp = tconnect.دوسرا;
                Ast_گرہ_اظہاریہ* ثابت pinp = argp->exprp();
                اگر (!pinp) جاری;  // Argument error we'll find later
                // Change data types based on above accept completion
                اگر (nodep->taskp()->dpiImport()) check_Unpacked_Array_دلائل(portp, pinp);
                اگر (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            برائے (ثابت خودکار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.پہلا;
                ثابت AstArg* ثابت argp = tconnect.دوسرا;
                Ast_گرہ_اظہاریہ* ثابت pinp = argp->exprp();
                اگر (!pinp) جاری;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, چوڑائی_VP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        اگر (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            برائے (ثابت خودکار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.پہلا;
                ثابت AstArg* ثابت argp = tconnect.دوسرا;
                Ast_گرہ_اظہاریہ* pinp = argp->exprp();
                اگر (!pinp) جاری;  // Argument error we'll find later
                Ast_گرہ_DType* ثابت پورٹ_DTypep = portp->dtypep()->skip_Ref_مقصد_Enump();
                اگر (VN_IS(pinp, خالی_خانے_قطار)) {
                    Ast_گرہ_اظہاریہ* newp = new_Cons_Dyn_Array_Or_قطار(pinp->fileline(), پورٹ_DTypep);
                    اگر (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = نیا AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(پورٹ_DTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ثابت Ast_گرہ_DType* ثابت pinDTypep = pinp->dtypep()->skip_Ref_مقصد_Enump();
                ثابت AstIfaceRefDType* ثابت پورٹ_Ifacep
                    = VN_CAST(پورٹ_DTypep->elemDTypep(سچ), IfaceRefDType);
                ثابت VCastable ifaceCastable
                    = پورٹ_Ifacep ? Ast_گرہ::computeCastable(پورٹ_DTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ سچ)
                                 : VCastable{VCastable::UNSUPPORTED};
                ثابت منطقی matchingRefDTypes = پورٹ_Ifacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(پورٹ_DTypep, pinDTypep);
                ثابت منطقی pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->عدد().isNull();
                اگر ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    اگر (پورٹ_Ifacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } ورنہ {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << پورٹ_DTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } ورنہ اگر (پورٹ_Ifacep && پورٹ_Ifacep->isVirtual() && portp->is_داخلہ() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } ورنہ اگر (portp->isWritable() && pinp->چوڑائی() != portp->چوڑائی()) {
                    pinp->v3widthWarn(portp->چوڑائی(), pinp->چوڑائی(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->چوڑائی()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->چوڑائی() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlink_Fr_عقب(&relinkHandle);
                    Ast_گرہ_اظہاریہ* ثابت newp = نیا AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                اگر (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(پورٹ_DTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && پورٹ_DTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << پورٹ_DTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    جاری;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                اگر (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } ورنہ اگر (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    check_Class_تفویض(nodep, "Function Argument", pinp, پورٹ_DTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, چوڑائی_VP{پورٹ_DTypep, FINAL, دھارا_USE_تفویض}.p());
                } ورنہ {
                    iterate_Check_تفویض(nodep, "Function Argument", pinp, FINAL, پورٹ_DTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    خالی handle_Std_Randomize_دلائل(Ast_گرہ_FTask_Ref* ثابت nodep) {
        AstConst* nullp = خالی_اشاریہ;
        برائے (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_گرہ_اظہاریہ* ثابت exprp = argp->exprp();
            اگر (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                اگر (constp->عدد().isNull()) {
                    nullp = constp;
                    جاری;
                }
            }
            اگر (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                جاری;
            }
            اگر (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                جاری;
            }
            اگر (ثابت AstCMethodHard* ثابت methodp = VN_CAST(exprp, CMethodHard)) {
                اگر (methodp->طریقہ() == VCMethod::ARRAY_AT
                    || methodp->طریقہ() == VCMethod::ARRAY_AT_WRITE) {
                    جاری;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        اگر (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        اگر (nodep->did_چوڑائی()) واپس;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = خالی_اشاریہ;
        اگر (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(سچ);
            nodep->dtypep(nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::INT));
            واپس;  // Handled in V3Randomize
        } ورنہ اگر (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ثابت classp = VN_CAST(nodep->classOrPackagep(), Class);
            اگر (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(سچ);
                Ast_گرہ_DType* ثابت adtypep = nodep->ڈھونڈو_ثنائی_ہندسہ_DType();
                withp = طریقہ_With_Clause(nodep, جھوٹ, جھوٹ, adtypep->ڈھونڈو_Void_DType(),
                                         adtypep->ڈھونڈو_ثنائی_ہندسہ_DType(), adtypep);
                برائے (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_اگلا(argp->exprp(), چوڑائی_VP{SELF, BOTH}.p());
                }
                handle_Std_Randomize_دلائل(nodep);  // Provided args should be in current scope
                process_FTask_Ref_دلائل(nodep);
                اگر (withp) nodep->withp(withp);
                nodep->did_چوڑائی(سچ);
                واپس;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            اگر (nodep->name() == "randomize") {
                AstClassRefDType* ثابت adtypep
                    = نیا AstClassRefDType{nodep->fileline(), classp, خالی_اشاریہ};
                v3Global.rootp()->typeTablep()->جوڑو_Typesp(adtypep);
                withp = طریقہ_With_Clause(nodep, جھوٹ, جھوٹ, adtypep->ڈھونڈو_Void_DType(),
                                         adtypep->ڈھونڈو_ثنائی_ہندسہ_DType(), adtypep);
                برائے (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_اگلا(argp->exprp(), چوڑائی_VP{SELF, BOTH}.p());
                }
                handle_Randomize_دلائل(nodep, classp);
            } ورنہ اگر (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } ورنہ اگر (nodep->name() == "get_randstate") {
                طریقہ_Ok_Arguments(nodep, 0, 0);
                classp->بنیاد_Most_Classp()->needRNG(سچ);
                v3Global.useRandomizeMethods(سچ);
                AstCExpr* ثابت newp
                    = نیا AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtype_سیٹ_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            } ورنہ اگر (nodep->name() == "set_randstate") {
                طریقہ_Ok_Arguments(nodep, 1, 1);
                Ast_گرہ_اظہاریہ* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_گرہ_اظہاریہ* ثابت exprp = nodep->argsp()->exprp();
                classp->بنیاد_Most_Classp()->needRNG(سچ);
                v3Global.useRandomizeMethods(سچ);
                AstCExpr* ثابت newp
                    = نیا AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->جوڑو(exprp->unlink_Fr_عقب());
                newp->جوڑو(")");
                newp->dtype_سیٹ_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            } ورنہ {
                UASSERT_OBJ(جھوٹ, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        اگر (nodep->did_چوڑائی()) {
            اگر (withp) nodep->withp(withp);
            واپس;
        }
        اگر ((nodep->taskp()->class_طریقہ() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            منطقی allow = جھوٹ;
            اگر (m_ftaskp && m_ftaskp->class_طریقہ() && !m_ftaskp->isStatic()) {
                اگر (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } ورنہ اگر (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                اگر (!allow) {
                    ثابت AstClass* callerClassp = m_containingClassFinder.ڈھونڈو(m_ftaskp);
                    اگر (!callerClassp) {
                        callerClassp = m_containingClassFinder.ڈھونڈو(m_ftaskp->classOrPackagep());
                    }
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    اگر (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.ڈھونڈو(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            اگر (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ثابت AstClass* ثابت cgClassp = m_containingClassFinder.ڈھونڈو(nodep);
                اگر (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    اگر (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.ڈھونڈو(nodep->taskp());
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
            nodep->scopeNamep(نیا AstScopeName{nodep->fileline(), جھوٹ});
        }
        // And do the arguments to the task/function too
        process_FTask_Ref_دلائل(nodep);
        اگر (withp) nodep->withp(withp);
        nodep->did_چوڑائی(سچ);
        // See steps that follow in visit(AstFuncRef*)
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, خالی_اشاریہ);
    }
    خالی دورہ(Ast_Sen_شے* nodep) override {
        اگر (nodep->is_Combo_ستارہ()) واپس;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        اگر (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            خودکار* ثابت concatOrReplp = VN_CAST(nodep->sensp(), گرہ_Biop);
            خودکار* ثابت rhsp = concatOrReplp->rhsp()->unlink_Fr_عقب();
            اگر (nodep->کنارہ_Type() == VEdgeType::ET_بدلا) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                خودکار* ثابت lhsp = concatOrReplp->lhsp()->unlink_Fr_عقب();
                nodep->جوڑو_اگلا_یہاں(نیا Ast_Sen_شے{lhsp->fileline(), nodep->کنارہ_Type(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(نیا Ast_Sen_شے{rhsp->fileline(), nodep->کنارہ_Type(), rhsp});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ {
            userIterateChildren(nodep, چوڑائی_VP{SELF, BOTH}.p());
            اگر (nodep->کنارہ_Type().an_کنارہ()) {
                Ast_گرہ_DType* ثابت sensDtp = nodep->sensp()->dtypep()->skipRefp();
                اگر (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } ورنہ اگر (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    خالی دورہ(Ast_Clocking_شے* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ثابت refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, چوڑائی_VP{SELF, PRELIM}.p());
    }
    خالی دورہ(AstWait* nodep) override {
        اگر (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }
        اگر (nodep->fileline()->timingOn()) {
            اگر (v3Global.opt.timing().is_سیٹ_True()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                اگر (Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(nodep->condp(), گرہ_Var_Ref)) {
                    اگر (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warn_مزید() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                واپس;
            } ورنہ اگر (v3Global.opt.timing().is_سیٹ_False()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } ورنہ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        اگر (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_عقب_With_اگلا();
            nodep->replaceWith(stmtsp);
        } ورنہ {
            nodep->unlink_Fr_عقب();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_گرہ_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->اشاریہ_Arg_Refp(), خالی_اشاریہ);
        userIterateChildren(nodep->قدر_Arg_Refp(), خالی_اشاریہ);
        اگر (!nodep->exprp()) {
            nodep->dtype_سیٹ_Void();
        } ورنہ {
            اگر (!nodep->exprp()->hasDType()) {
                user_Iterate_And_اگلا(nodep->exprp(), خالی_اشاریہ);
            } ورنہ {
                اگر (vdtypep) {
                    user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{nodep->dtypep(), PRELIM}.p());
                } ورنہ {  // 'sort with' allows arbitrary type
                    user_Iterate_And_اگلا(nodep->exprp(), چوڑائی_VP{SELF, PRELIM}.p());
                }
            }

            اگر (!nodep->exprp()->hasDType()) {
                nodep->dtype_سیٹ_Void();
            } ورنہ {
                nodep->dtypeFrom(nodep->exprp());
                iterate_Check_تفویض(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    خالی دورہ(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        اگر (nodep->اشاریہ()) {
            nodep->dtypeFrom(m_withp->اشاریہ_Arg_Refp());
        } ورنہ {
            nodep->dtypeFrom(m_withp->قدر_Arg_Refp());
        }
    }
    خالی دورہ(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, خالی_اشاریہ);
    }

    //--------------------
    // Default
    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {
        اگر (!nodep->did_چوڑائی()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, خالی_اشاریہ);
    }
    خالی دورہ(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << سطر_ختم);
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        اگر (Ast_گرہ_DType* ثابت vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } ورنہ اگر (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->چوڑائی() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->چوڑائی() << " bits -> " << nodep->چوڑائی()
                                    << " bits" << سطر_ختم);

        // Final processing
        user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, FINAL}.p());
        nodep->did_چوڑائی(سچ);
    }
    خالی دورہ_Class(AstClass* nodep) {
        اگر (nodep->did_چوڑائی_And_سیٹ()) واپس;

        // If the class is std::process
        اگر (nodep->name() == "process") {
            AstPackage* ثابت packagep = get_شے_Package(nodep);
            اگر (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                اگر (AstVar* ثابت varp
                    = VN_CAST(m_memberMap.ڈھونڈو_Member(nodep, "m_process"), Var)) {
                    Ast_گرہ_DType* ثابت dtypep = varp->get_بچہ_DTypep();
                    اگر (!varp->dtypep()) {
                        VL_DO_DANGLING(ڈالو_Deletep(dtypep->unlink_Fr_عقب()), dtypep);
                    }
                    AstBasicDType* ثابت newdtypep = نیا AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->جوڑو_Typesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                اگر (Ast_گرہ_FTask* ثابت ftaskp
                    = VN_CAST(m_memberMap.ڈھونڈو_Member(nodep, "self"), گرہ_FTask)) {
                    ftaskp->سیٹ_Need_Process();
                    v3Global.سیٹ_استعمال_کرتا_Timing();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        user_Iterate_And_اگلا(nodep->extendsp(), خالی_اشاریہ);
        userIterateChildren(nodep, خالی_اشاریہ);  // First size all members
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enum_عارضی_نام);
        VL_RESTORER_COPY(m_inside_عارضی_نام);
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            دورہ_Class(classp);
        } ورنہ {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([یہ](ثابت AstVar* varp) { m_curModVars.داخل_کرو(varp); });
            userIterateChildren(nodep, خالی_اشاریہ);
        }
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, خالی_اشاریہ);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    خالی دورہ_Or_Lu64(Ast_گرہ_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_سیٹ_Double();
            Ast_گرہ_DType* ثابت subDTypep = nodep->ڈھونڈو_ثنائی_ہندسہ_DType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    خالی دورہ(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_سیٹ_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, PRELIM}.p());
            اگر (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    نیا AstISToRD{nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب()});
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            }
        }
    }
    خالی دورہ(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_سیٹ_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, PRELIM}.p());
        }
    }
    خالی دورہ_Os32_Lr(Ast_گرہ_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سیٹ_Integer2_State();
        }
    }
    خالی دورہ_Ou64_Lr(Ast_گرہ_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سیٹ_UInt64();
        }
    }

    خالی دورہ_log_not(AstLogNot* nodep) {
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
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            اگر (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ثابت newp
                    = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            }
        }
    }
    خالی دورہ_log_and_or(Ast_گرہ_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(
                Ast_گرہ_اظہاریہ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    خالی دورہ_Abort_Prop(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ_red_and_or(Ast_گرہ_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    خالی دورہ_red_unknown(Ast_گرہ_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    منطقی isEquivalentDType(ثابت Ast_گرہ_DType* lhs, ثابت Ast_گرہ_DType* rhs) {
        // a) If two types match, they are equivalent.
        اگر (!lhs || !rhs) واپس جھوٹ;
        lhs = lhs->skip_Ref_مقصد_Enump();
        rhs = rhs->skip_Ref_مقصد_Enump();
        اگر (lhs == rhs) واپس سچ;
        // If both are basic types, check if they are the same type
        اگر (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ثابت خودکار* lb = VN_CAST(lhs, BasicDType);
            ثابت خودکار* rb = VN_CAST(rhs, BasicDType);
            اگر (lb->isString() != rb->isString()) واپس جھوٹ;
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
                اگر (lsz >= 0 && rsz >= 0 && lsz != rsz) واپس جھوٹ;
                واپس isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            واپس جھوٹ;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ثابت منطقی lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ثابت منطقی rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ثابت منطقی lhs_Is_قطار = VN_IS(lhs, قطار_DType);
        ثابت منطقی rhs_Is_قطار = VN_IS(rhs, قطار_DType);
        ثابت منطقی lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ثابت منطقی rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        اگر (lhsIsDynArray || rhsIsDynArray || lhs_Is_قطار || rhs_Is_قطار || lhsIsAssocArray
            || rhsIsAssocArray) {
            اگر (ثابت AstDynArrayDType* ثابت lhsp = VN_CAST(lhs, DynArrayDType)) {
                اگر (ثابت AstDynArrayDType* ثابت rhsp = VN_CAST(rhs, DynArrayDType)) {
                    واپس isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            اگر (ثابت Ast_قطار_DType* ثابت lhsp = VN_CAST(lhs, قطار_DType)) {
                اگر (ثابت Ast_قطار_DType* ثابت rhsp = VN_CAST(rhs, قطار_DType)) {
                    واپس isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            اگر (ثابت AstAssocArrayDType* ثابت lhsp = VN_CAST(lhs, AssocArrayDType)) {
                اگر (ثابت AstAssocArrayDType* ثابت rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    واپس isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->کلید_DTypep(), rhsp->کلید_DTypep());
                }
            }

            واپس جھوٹ;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        اگر (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            اگر (lhs->چوڑائی() != rhs->چوڑائی()) واپس جھوٹ;
            اگر (lhs->isFourstate() != rhs->isFourstate()) واپس جھوٹ;
            اگر (lhs->isSigned() != rhs->isSigned()) واپس جھوٹ;
            واپس سچ;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        اگر (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) واپس جھوٹ;
        واپس !(VN_IS(lhs, گرہ_UOr_Struct_DType) || VN_IS(rhs, گرہ_UOr_Struct_DType));
    }

    ساکن منطقی isAggregateType(ثابت Ast_گرہ* nodep) {
        اگر (!nodep) واپس جھوٹ;
        ثابت Ast_گرہ_DType* dtypep = nodep->dtypep();
        اگر (!dtypep) واپس جھوٹ;
        dtypep = dtypep->skipRefp();
        اگر (!dtypep) واپس جھوٹ;
        واپس dtypep->isAggregateType();
    }

    خالی دورہ_cmp_eq_gt(Ast_گرہ_Biop* nodep, ثابت ComparisonType cmpType) {
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
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ثابت منطقی lhsIsPat = VN_IS(nodep->lhsp(), نمونہ);
            ثابت منطقی rhsIsPat = VN_IS(nodep->rhsp(), نمونہ);
            اگر (lhsIsPat && !rhsIsPat) {
                user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } ورنہ اگر (rhsIsPat && !lhsIsPat) {
                user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } ورنہ {
                user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            }

            ثابت منطقی isAggrLhs = isAggregateType(nodep->lhsp());
            ثابت منطقی isAggrRhs = isAggregateType(nodep->rhsp());

            اگر ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::معمول_EQ || cmpType == ComparisonType::CASE)) {
                ثابت Ast_گرہ_DType* ثابت lhsDType = nodep->lhsp()->dtypep();
                ثابت Ast_گرہ_DType* ثابت rhsDType = nodep->rhsp()->dtypep();

                اگر (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_گرہ* ثابت newp
                        = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convert_Unpacked_ترتیب(nodep->rhsp(), nodep->lhsp()->dtypep());
            } ورنہ اگر (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                اگر (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_گرہ* ثابت newp
                        = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
                اگر (Ast_گرہ_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ورنہ اگر (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                اگر (Ast_گرہ_Biop* ثابت newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ورنہ {
                ثابت منطقی signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                اگر (Ast_گرہ_Biop* ثابت newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ثابت صحیح چوڑائی = std::max(nodep->lhsp()->چوڑائی(), nodep->rhsp()->چوڑائی());
                ثابت صحیح ewidth = std::max(nodep->lhsp()->چوڑائی_Min(), nodep->rhsp()->چوڑائی_Min());
                منطقی warnOn = سچ;
                اگر (!signedFl && چوڑائی == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    اگر ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->چوڑائی() >= nodep->rhsp()->چوڑائی_Min())) {
                        warnOn = جھوٹ;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    اگر ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->چوڑائی_Min() >= nodep->rhsp()->چوڑائی())) {
                        warnOn = جھوٹ;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(چوڑائی, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(چوڑائی, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::معمول_EQ
                                               || cmpType == ComparisonType::معمول_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    خالی دورہ_cmp_real(Ast_گرہ_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ_cmp_string(Ast_گرہ_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سیٹ_ثنائی_ہندسہ();
        }
    }
    خالی دورہ_cmp_type(Ast_گرہ_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, BOTH}.p());
            user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{SELF, BOTH}.p());
            ثابت Ast_Attr_کا* ثابت lhsap = VN_AS(nodep->lhsp(), Attr_کا);
            ثابت Ast_Attr_کا* ثابت rhsap = VN_AS(nodep->rhsp(), Attr_کا);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ثابت Ast_گرہ_DType* ثابت lhsDtp = lhsap->dtypep();
            ثابت Ast_گرہ_DType* ثابت rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ثابت منطقی invert = VN_IS(nodep, NeqT);
            ثابت منطقی identical
                = Ast_گرہ::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ثابت منطقی eq = invert ^ identical;
            Ast_گرہ* ثابت newp = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_True{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }

    خالی دورہ_negate_not(Ast_گرہ_Uniop* nodep, منطقی real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            اگر (!real_ok) checkCvtUS(nodep->lhsp(), جھوٹ);
        }
        اگر (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            اگر (Ast_گرہ_Uniop* ثابت newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtype_سیٹ_Double();
                واپس;
            }
        } ورنہ {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        اگر (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        اگر (m_vup->final()) {
            Ast_گرہ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_گرہ_DType* ثابت subDTypep = expDTypep;
            // Some warning suppressions
            منطقی lhsWarn = سچ;
            اگر (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                اگر (subDTypep->چوڑائی_Min() == (nodep->lhsp()->چوڑائی_Min() + 1)) lhsWarn = جھوٹ;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    خالی دورہ_signed_unsigned(Ast_گرہ_Uniop* nodep, VSigning rs_برآمد) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), سچ);
            ثابت صحیح چوڑائی = nodep->lhsp()->چوڑائی();
            Ast_گرہ_DType* ثابت expDTypep = nodep->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                چوڑائی, چوڑائی, rs_برآمد, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_گرہ_DType* ثابت subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    خالی دورہ_سرکاؤ_2(Ast_گرہ_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_سرکاؤ_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ثابت Ast_گرہ_Biop* ثابت newp = iterate_سرکاؤ_final(nodep);
        VL_DANGLING(nodep);
        (خالی)newp;  // Ununused
    }
    خالی iterate_سرکاؤ_prelim(Ast_گرہ_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), جھوٹ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            اگر (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtype_سیٹ_Logic_Unsized(nodep->چوڑائی(), nodep->چوڑائی_Min(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_گرہ_Biop* iterate_سرکاؤ_final(Ast_گرہ_Biop* nodep) {
        // Nodep maybe edited
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->final()) {
            Ast_گرہ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرہ_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            اگر (VN_IS(nodep, سرکاؤ_RS)) {
                اگر (Ast_گرہ_Biop* ثابت newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            منطقی warnOn = سچ;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            اگر (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), گرہ_تفویض)) warnOn = جھوٹ;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const);
            اگر (shiftp && !shiftp->عدد().isFourState() && shiftp->چوڑائی() <= 32) {
                ثابت int64_t سرکاؤ_Val = shiftp->عدد().مقصد_SQuad();
                اگر (VN_IS(nodep, سرکاؤ_L)) {
                    اگر (سرکاؤ_Val > 0 && nodep->چوڑائی() == nodep->lhsp()->چوڑائی() + سرکاؤ_Val)
                        warnOn = جھوٹ;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            اگر (nodep->rhsp()->چوڑائی() > 32) {
                اگر (shiftp && shiftp->عدد().most_سیٹ_ثنائی_ہندسہ_P1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_نمبر عدد{shiftp, 32, shiftp->عدد()};
                    Ast_گرہ* ثابت shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(نیا AstConst{shiftrhsp->fileline(), عدد});
                    VL_DO_DANGLING(shiftrhsp->delete_درخت(), shiftrhsp);
                }
            }
        }
        واپس nodep;  // May edit
    }

    خالی دورہ_boolexpr_and_or(Ast_گرہ_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), جھوٹ);
            checkCvtUS(nodep->rhsp(), جھوٹ);
            ثابت صحیح چوڑائی = std::max(nodep->lhsp()->چوڑائی(), nodep->rhsp()->چوڑائی());
            ثابت صحیح mwidth = std::max(nodep->lhsp()->چوڑائی_Min(), nodep->rhsp()->چوڑائی_Min());
            ثابت منطقی expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(
                چوڑائی, mwidth, VSigning::fromBool(expSigned),
                Ast_گرہ_اظہاریہ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        اگر (m_vup->final()) {
            Ast_گرہ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرہ_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    خالی دورہ_جوڑو_sub_replace(Ast_گرہ_Biop* nodep, منطقی real_ok, ثابت منطقی modOrDiv) {
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
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_اگلا(nodep->lhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_اگلا(nodep->rhsp(), چوڑائی_VP{CONTEXT_DET, PRELIM}.p());
            اگر (!real_ok) {
                checkCvtUS(nodep->lhsp(), جھوٹ);
                checkCvtUS(nodep->rhsp(), جھوٹ);
            }
            اگر (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                اگر (Ast_گرہ_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtype_سیٹ_Double();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                واپس;
            } ورنہ اگر (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, جوڑو_2)
                            ? (nodep->warn_مزید()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            } ورنہ {
                ثابت صحیح چوڑائی = std::max(nodep->lhsp()->چوڑائی(), nodep->rhsp()->چوڑائی());
                ثابت صحیح mwidth = std::max(nodep->lhsp()->چوڑائی_Min(), nodep->rhsp()->چوڑائی_Min());
                ثابت منطقی expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(
                    چوڑائی, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_گرہ_اظہاریہ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        اگر (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_گرہ_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گرہ_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            اگر (Ast_گرہ_Biop* ثابت newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            منطقی lhsWarn = سچ;
            منطقی rhsWarn = سچ;
            اگر (VN_IS(nodep, جوڑو_2) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                اگر (subDTypep->چوڑائی_Min() == (nodep->lhsp()->چوڑائی_Min() + 1)) lhsWarn = جھوٹ;
                اگر (subDTypep->چوڑائی_Min() == (nodep->rhsp()->چوڑائی_Min() + 1)) rhsWarn = جھوٹ;
                اگر (VN_IS(nodep, جوڑو_2) && nodep->lhsp()->چوڑائی() == 1
                    && nodep->rhsp()->چوڑائی() != 1)
                    lhsWarn = جھوٹ;  // do_increment + ...
                اگر (nodep->rhsp()->چوڑائی() == 1 && nodep->lhsp()->چوڑائی() != 1)
                    rhsWarn = جھوٹ;  // ... + do_increment
            } ورنہ اگر (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                اگر (subDTypep->چوڑائی_Min() >= (nodep->lhsp()->چوڑائی_Min())) lhsWarn = جھوٹ;
                اگر (subDTypep->چوڑائی_Min() >= (nodep->rhsp()->چوڑائی_Min())) rhsWarn = جھوٹ;
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
    خالی دورہ_real_جوڑو_sub(Ast_گرہ_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سیٹ_Double();
        }
    }
    خالی دورہ_real_neg_ceil(Ast_گرہ_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_اظہاریہ(nodep);
        اگر (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سیٹ_Double();
        }
    }

    خالی دورہ_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->اظہاریہ_Format_2(), nodep, "Assumes constant format");
        منطقی اندر_Pct = جھوٹ;
        Ast_گرہ_اظہاریہ* argp = nodep->exprsp();
        string newFormat;
        برائے (حرف ch : nodep->متن()) {
            اگر (!اندر_Pct && ch == '%') {
                اندر_Pct = سچ;
                newFormat += ch;
            } ورنہ اگر (اندر_Pct && (std::ہندسہ_ہے(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } ورنہ اگر (!اندر_Pct) {  // Normal text
                newFormat += ch;
            } ورنہ {
                اندر_Pct = جھوٹ;
                Ast_گرہ_اظہاریہ* ثابت nextp = argp ? VN_AS(argp->nextp(), گرہ_اظہاریہ) : خالی_اشاریہ;
                AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_گرہ_اظہاریہ* subargp = fargp ? fargp->exprp() : argp;
                ثابت Ast_گرہ_DType* ثابت dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : خالی_اشاریہ;
                ch = std::tolower(ch);
                انتخاب (ch) {
                حالت '?':  // Unspecified by user, guess
                    اگر (dtypep->isDouble()) {
                        ch = 'g';
                    } ورنہ اگر (dtypep->isString()) {
                        ch = 's';
                    } ورنہ اگر (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } ورنہ {
                        ch = 'p';
                    }
                    argp = nextp;
                    توڑو;
                حالت '%': توڑو;  // %% - just output a %
                حالت 'm': توڑو;  // %m - auto insert "name"
                حالت 'l': توڑو;  // %m - auto insert "library"
                حالت 'd':
                    اگر (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        اگر (subargp->isDouble()) spliceCvtS(subargp, سچ, 64);
                        argp = nextp;
                    }
                    توڑو;
                حالت 'b':  // FALLTHRU
                حالت 'o':  // FALLTHRU
                حالت 'x':
                    اگر (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    توڑو;
                حالت 't':  // Decimal/float time
                    اگر (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    توڑو;
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
                    توڑو;
                حالت 'p':  // FALLTHRU
                حالت 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    اگر (subargp && !fargp) {
                        اگر (AstEnumDType* ثابت enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlink_Fr_عقب(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    توڑو;
                طےشدہ:  // Most operators, just move to next argument
                    argp = nextp;
                    توڑو;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->متن(newFormat);
    }

    ساکن AstEnumDType* formatEnumDType(Ast_گرہ_اظہاریہ* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
        اگر (!enumDtp) {
            اگر (ثابت AstVarRef* ثابت varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
            }
        }
        واپس enumDtp;
    }

    Ast_اظہاریہ_Stmt* newEnumCapture(Ast_گرہ_اظہاریہ* valuep, AstEnumDType* dtypep) {
        File_سطر* ثابت flp = valuep->fileline();
        AstVar* ثابت varp = نیا AstVar{flp, VVarType::XTEMP, m_enum_عارضی_نام.get(valuep), dtypep};
        varp->noSubst(سچ);
        اگر (m_ftaskp) {
            varp->funcLocal(سچ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } ورنہ {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->جوڑو_Stmtsp(varp);
        }
        iterate(varp);
        Ast_اظہاریہ_Stmt* ثابت exprp = نیا Ast_اظہاریہ_Stmt{
            flp, نیا Ast_تفویض{flp, نیا AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            نیا AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        واپس exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_گرہ_اظہاریہ* valuep, AstEnumDType* dtypep) {
        اگر (valuep->is_خالص()) valuep = V3Const::constifyEdit(valuep);
        Ast_گرہ_اظہاریہ* keyp;
        اگر (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_درخت(جھوٹ);
        } ورنہ {
            Ast_اظہاریہ_Stmt* ثابت capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_درخت_خالص(جھوٹ);
        }
        ثابت VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ثابت argp = نیا AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        واپس argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    منطقی چوڑائی_Bad(Ast_گرہ* nodep, Ast_گرہ_DType* expDTypep) {
        ثابت صحیح exp_چوڑائی = expDTypep->چوڑائی();
        صحیح exp_چوڑائی_Min = expDTypep->چوڑائی_Min();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        اگر (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) واپس جھوٹ;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->چوڑائی() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(exp_چوڑائی != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        اگر (exp_چوڑائی_Min == 0) exp_چوڑائی_Min = exp_چوڑائی;
        اگر (nodep->dtypep()->چوڑائی() == exp_چوڑائی) واپس جھوٹ;
        اگر (nodep->dtypep()->چوڑائی_Sized() && nodep->چوڑائی() != exp_چوڑائی_Min) واپس سچ;
        اگر (!nodep->dtypep()->چوڑائی_Sized() && nodep->چوڑائی_Min() > exp_چوڑائی_Min) واپس سچ;
        واپس جھوٹ;
    }

    خالی fix_چوڑائی_Extend(Ast_گرہ_اظہاریہ* nodep, Ast_گرہ_DType* expDTypep, Extend_قاعدہ extend_قاعدہ) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<صحیح>(extend_قاعدہ) << ") old: " << nodep);
        اگر (extend_قاعدہ == EXTEND_OFF) واپس;
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        ثابت صحیح exp_چوڑائی = expDTypep->چوڑائی();
        اگر (constp && !constp->عدد().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_نمبر عدد{nodep, exp_چوڑائی, constp->عدد()};
            عدد.isSigned(جھوٹ);
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstConst{nodep->fileline(), عدد};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ورنہ اگر (exp_چوڑائی < nodep->چوڑائی()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlink_Fr_عقب(&linker);
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstSel{nodep->fileline(), nodep, 0, exp_چوڑائی};
            newp->did_چوڑائی(سچ);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } ورنہ {
            // Extend
            VNRelinker linker;
            nodep->unlink_Fr_عقب(&linker);
            منطقی doSigned = جھوٹ;
            انتخاب (extend_قاعدہ) {
            حالت EXTEND_ZERO: doSigned = جھوٹ; توڑو;
            حالت EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); توڑو;
            حالت EXTEND_LHS: doSigned = nodep->isSigned(); توڑو;
            طےشدہ: nodep->v3fatalSrc("bad case");
            }
            Ast_گرہ_اظہاریہ* ثابت newp
                = (doSigned ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_گرہ_اظہاریہ*>(نیا AstExtend{nodep->fileline(), nodep}));
            newp->did_چوڑائی(سچ);
            linker.relink(newp);
            nodep = newp;
        }
        اگر (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_گرہ_اظہاریہ* ثابت newp = spliceCvtD(nodep);
            nodep = newp;
        }
        اگر (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtype_Chg_چوڑائی_Signed(exp_چوڑائی, expDTypep->چوڑائی_Min(), expDTypep->numeric());
        } ورنہ {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    خالی fix_چوڑائی_Reduce(Ast_گرہ_اظہاریہ* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ثابت صحیح exp_چوڑائی = 1;
        ثابت صحیح expSigned = جھوٹ;
        اگر (nodep->چوڑائی() == exp_چوڑائی) واپس;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        اگر (constp) {
            V3_نمبر عدد(nodep, exp_چوڑائی);
            عدد.opRedOr(constp->عدد());
            عدد.isSigned(expSigned);
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstConst{nodep->fileline(), عدد};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_درخت(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ورنہ {
            VNRelinker linker;
            nodep->unlink_Fr_عقب(&linker);
            Ast_گرہ_اظہاریہ* newp;
            Ast_گرہ_DType* ثابت dtypeSkipp = nodep->dtypep()->skipRefp();
            اگر (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = نیا AstNeq{nodep->fileline(),
                                  نیا AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } ورنہ {
                // But, AstRedOr can be faster on wide
                newp = نیا AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtype_Chg_چوڑائی_Signed(exp_چوڑائی, exp_چوڑائی, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    منطقی fixAutoExtend(Ast_گرہ_اظہاریہ*& nodepr, صحیح exp_چوڑائی) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        اگر (AstConst* ثابت constp = VN_CAST(nodepr, Const)) {
            اگر (constp->عدد().autoExtend() && !constp->عدد().sized() && constp->چوڑائی() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_نمبر عدد(constp, exp_چوڑائی);
                عدد.opRepl(constp->عدد(), exp_چوڑائی);  // {width{'1}}
                Ast_گرہ_اظہاریہ* ثابت newp = نیا AstConst{constp->fileline(), عدد};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_درخت(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                واپس سچ;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            ورنہ اگر (!constp->عدد().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && exp_چوڑائی > 32 && constp->عدد().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << exp_چوڑائی
                                                  << " bits: " << constp->prettyName());
                V3_نمبر عدد(constp, exp_چوڑائی);
                عدد.opExtendXZ(constp->عدد(), constp->چوڑائی());
                Ast_گرہ_اظہاریہ* ثابت newp = نیا AstConst{constp->fileline(), عدد};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_درخت(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                واپس سچ;
            }
        }
        واپس جھوٹ;  // No change
    }
    منطقی is_بنیاد_Class_Recurse(ثابت AstClass* ثابت cls1p, ثابت AstClass* ثابت cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        اگر (cls1p == cls2p) واپس سچ;
        برائے (ثابت AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            اگر (is_بنیاد_Class_Recurse(cls1p, cextp->classp())) واپس سچ;
        }
        واپس جھوٹ;
    }
    // Lines to end a message on nodep with the data types of its operands
    ساکن string warnDTypes(ثابت Ast_گرہ* nodep, ثابت Ast_گرہ_DType* lhsDtp,
                             ثابت Ast_گرہ_DType* rhsDtp, ثابت string& lhsName = "Left-hand",
                             ثابت string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        اگر (lhsState == rhsState) lhsState = rhsState = "";
        واپس "\n" + nodep->warn_مزید() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warn_مزید()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    ساکن منطقی unpacked_Array_Sizes_مطابقت(ثابت Ast_گرہ_DType* lhsDtp, ثابت Ast_گرہ_DType* rhsDtp) {
        جب (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ثابت AstUnpackArrayDType* ثابت lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ثابت AstUnpackArrayDType* ثابت rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            اگر (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                واپس جھوٹ;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        واپس سچ;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    سانچہ <قسم_نام T, قسم_نام N>
    خالی checkUnpackedArrayAssignmentCompatible(ثابت Ast_گرہ* nodep, ثابت T* ثابت lhsRefp,
                                                ثابت N* ثابت rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::قدر || std::is_same<T, Ast_گرہ_Var_Ref>::قدر)
                && (std::is_same<N, AstVar>::قدر || std::is_same<N, Ast_گرہ_Var_Ref>::قدر),
            "Unsupported types provided.");
        اگر (!lhsRefp || !rhsRefp) واپس;
        std::string lhsName;
        std::string rhsName;
        اگر (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } ورنہ {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ثابت Ast_گرہ_DType* ثابت lhsDtp = lhsRefp->dtypep()->skipRefp();
        ثابت Ast_گرہ_DType* ثابت rhsDtp = rhsRefp->dtypep()->skipRefp();
        ثابت منطقی isLhsAggregate = lhsDtp->isAggregateType();
        ثابت منطقی isRhsAggregate = rhsDtp->isAggregateType();
        اگر (!isLhsAggregate && !isRhsAggregate) واپس;
        اگر (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            واپس;
        } ورنہ اگر (VN_IS(lhsDtp, قطار_DType) && VN_IS(rhsDtp, خالی_خانے_قطار_DType)) {
            واپس;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(جھوٹ),
                                      rhsDim = rhsDtp->dimensions(جھوٹ);
        // Check if unpacked array dimensions are matching
        اگر (lhsDim.دوسرا != rhsDim.دوسرا) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            واپس;
        }

        ثابت Ast_گرہ_DType* lhsDtpIterp = lhsDtp;
        ثابت Ast_گرہ_DType* rhsDtpIterp = rhsDtp;
        برائے (uint32_t dim = 0; dim < rhsDim.دوسرا; dim++) {
            // Associative arrays are compatible only with each other
            اگر (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                واپس;
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
    خالی check_Class_تفویض(ثابت Ast_گرہ* nodep, ثابت حرف* side, Ast_گرہ* rhsp,
                          Ast_گرہ_DType* ثابت lhsDTypep, منطقی isOutputArg = جھوٹ) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ثابت Ast_گرہ_DType* ثابت lhsRawDTypep = lhsDTypep->skipRefp();
        ثابت Ast_گرہ_DType* ثابت rhsRawDTypep = rhsp->dtypep()->skipRefp();
        اگر (ثابت AstClassRefDType* ثابت lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            اگر (ثابت AstClassRefDType* ثابت rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ثابت AstClass* ثابت dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ثابت AstClass* ثابت srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                اگر (is_بنیاد_Class_Recurse(dstp, srcp)) واپس;
            } ورنہ اگر (rhsp->isNull()) {
                واپس;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } ورنہ اگر (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        اگر (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlink_Fr_عقب(&relinker);
            relinker.relink(
                نیا Ast_Cvt_Unpack_مقصد_قطار{rhsp->fileline(), VN_AS(rhsp, گرہ_اظہاریہ), lhsDTypep});
        }
    }
    ساکن منطقی similarDTypeRecurse(ثابت Ast_گرہ_DType* ثابت node1p,
                                    ثابت Ast_گرہ_DType* ثابت node2p) {
        واپس node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    خالی iterateCheckFileDesc(Ast_گرہ* parentp, Ast_گرہ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, چوڑائی_VP{SELF, PRELIM}.p());
        Ast_گرہ_DType* ثابت expDTypep = underp->ڈھونڈو_UInt32_DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (خالی)underp;  // cppcheck
    }
    خالی iterateCheckReal(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ڈھونڈو_Double_DType(), stage);
    }
    خالی iterateCheckSigned8(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ڈھونڈو_Signed8_DType(), stage);
    }
    خالی iterateCheckSigned32(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ڈھونڈو_Int_DType(), stage);
    }
    خالی iterateCheckUInt32(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ڈھونڈو_UInt32_DType(), stage);
    }
    خالی iterateCheckDelay(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, چوڑائی_VP{SELF, PRELIM}.p());
        }
        اگر (stage & FINAL) {
            Ast_گرہ_DType* expDTypep;
            اگر (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->ڈھونڈو_Double_DType();
            } ورنہ {
                File_سطر* ثابت newFl = نیا File_سطر{underp->fileline()};
                newFl->warnOff(V3_Error_رمز::WIDTHEXPAND, سچ);
                underp->fileline(newFl);
                expDTypep = parentp->ڈھونڈو_ثنائی_ہندسہ_DType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, جھوٹ);
        }
        (خالی)underp;  // cppcheck
    }
    خالی iterateCheckString(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->ڈھونڈو_String_DType(), stage);
    }
    خالی iterateCheckTyped(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp,
                           Ast_گرہ_DType* expDTypep, Stage stage) {
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, چوڑائی_VP{expDTypep, PRELIM}.p());
        }
        اگر (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (خالی)underp;  // cppcheck
    }
    خالی iterateCheckTypedSelfPrelim(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp,
                                     Ast_گرہ_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, چوڑائی_VP{SELF, PRELIM}.p());
        }
        اگر (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (خالی)underp;  // cppcheck
    }
    خالی iterateCheckIntegralSelf(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, سچ);
    }
    خالی iterateCheckSelf(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Determ determ,
                          Stage stage, منطقی integralOnly = جھوٹ) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, چوڑائی_VP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, گرہ_اظہاریہ) ? checkCvtUS(VN_AS(underp, گرہ_اظہاریہ), integralOnly) : underp;
        Ast_گرہ_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (خالی)underp;  // cppcheck
    }
    خالی iterateCheckSizedSelf(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Determ determ,
                               Stage stage, منطقی deferUntyped = جھوٹ) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        اگر (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, چوڑائی_VP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, گرہ_اظہاریہ) ? checkCvtUS(VN_AS(underp, گرہ_اظہاریہ), جھوٹ) : underp;
        Ast_گرہ_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_گرہ_DType* ثابت checkDtp = expDTypep->skip_Ref_مقصد_Enump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        اگر (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (خالی)underp;  // cppcheck
    }
    خالی iterate_Check_تفویض(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* rhsp, Stage stage,
                            Ast_گرہ_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        اگر (AstConst* constp = VN_CAST(rhsp, Const)) {
            اگر (ثابت AstUnpackArrayDType* ثابت arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                اگر (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    اگر (basicp->چوڑائی() == 8 && constp->عدد().isFromString()) {
                        AstConsArray* newp = نیا AstConsArray{
                            constp->fileline(), lhsDTypep,
                            نیا AstConst{constp->fileline(), AstConst::Widthed_قدر{}, 8, 0}};
                        برائے (ثابت صحیح aindex : arrayp->declRange().seq_Lo_مقصد_Hi()) {
                            صحیح cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_نمبر selected{constp, 8};
                            selected.opSel(constp->عدد(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            اگر (!selected.isFourState()) {
                                اگر (ثابت uint32_t c = selected.مقصد_UInt()) {
                                    newp->valuep(aindex,
                                                 نیا AstConst{constp->fileline(),
                                                              AstConst::Widthed_قدر{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ڈالو_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        check_Class_تفویض(parentp, side, rhsp, lhsDTypep);
        ثابت منطقی lhs_دھارا = (VN_IS(parentp, گرہ_تفویض)
                                && VN_IS(VN_AS(parentp, گرہ_تفویض)->lhsp(), گرہ_دھارا));
        rhsp = iterateCheck(parentp, side, rhsp, تفویض_3, FINAL, lhsDTypep,
                            lhs_دھارا ? EXTEND_OFF : EXTEND_LHS, سچ, دھارا_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (خالی)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_گرہ_اظہاریہ* convert_Unpacked_ترتیب(Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_DType* dtypep) {
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        اگر (!arrayp || !arrayp->is_الٹ_کا(exprp->dtypep())) واپس exprp;
        VNRelinker relinker;
        exprp->unlink_Fr_عقب(&relinker);
        Ast_گرہ_اظہاریہ* ثابت newp = نیا Ast_Cvt_Unpack_مقصد_Unpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        واپس newp;
    }

    ساکن منطقی isLogicalOperator(ثابت Ast_گرہ* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        واپس VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    خالی iterateCheckBool(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, چوڑائی_VP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_گرہ_DType* ثابت underVDTypep = underp ? underp->dtypep()->skipRefp() : خالی_اشاریہ;
        اگر (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_عقب(&linker);
            Ast_گرہ* ثابت newp
                = نیا AstNeqD{parentp->fileline(), VN_AS(underp, گرہ_اظہاریہ),
                              نیا AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } ورنہ اگر (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_عقب(&linker);
            Ast_گرہ* ثابت newp
                = نیا AstNeqN{parentp->fileline(), VN_AS(underp, گرہ_اظہاریہ),
                              نیا AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } ورنہ اگر (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fix_چوڑائی_Reduce(VN_AS(underp, گرہ_اظہاریہ)), underp);  //بدلا
        } ورنہ اگر (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(نیا AstConst{parentp->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
            VL_DO_DANGLING(ڈالو_Deletep(underp), underp);
        } ورنہ {
            ثابت منطقی bad = چوڑائی_Bad(underp, parentp->ڈھونڈو_ثنائی_ہندسہ_DType());
            اگر (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->چوڑائی(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->چوڑائی()
                                             << (underp->چوڑائی() != underp->چوڑائی_Min()
                                                     ? " or " + cvt_مقصد_Str(underp->چوڑائی_Min())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            اگر (Ast_اظہاریہ_Stmt* ثابت اظہاریہ_Stmtp = VN_CAST(underp, اظہاریہ_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fix_چوڑائی_Reduce(اظہاریہ_Stmtp->resultp());
                اظہاریہ_Stmtp->dtypeFrom(اظہاریہ_Stmtp->resultp());
            } ورنہ اگر (underp->چوڑائی() != 1) {
                VL_DO_DANGLING(fix_چوڑائی_Reduce(VN_AS(underp, گرہ_اظہاریہ)), underp);  //بدلا
            }
        }
    }

    ساکن خالی checkEnum(Ast_گرہ* nodep, Ast_گرہ* parentp, ثابت Ast_گرہ_DType* ہدف_DTypep,
                          ثابت Ast_گرہ_DType* ماخذ_DTypep, ثابت Ast_گرہ_اظہاریہ* ماخذ_Exprp) {
        اگر (ثابت AstEnumDType* ثابت ہدف_Enump
            = VN_CAST(ہدف_DTypep->skip_Ref_مقصد_Enump(), EnumDType)) {
            ثابت VCastable castable
                = Ast_گرہ::computeCastable(ہدف_Enump, ماخذ_DTypep, ماخذ_Exprp);
            اگر (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << ہدف_DTypep->prettyDTypeNameQ() << " from "
                                             << ماخذ_DTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warn_مزید()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_گرہ* iterateCheck(Ast_گرہ* parentp, ثابت حرف* side, Ast_گرہ* underp, Determ determ,
                          Stage stage, Ast_گرہ_DType* expDTypep, Extend_قاعدہ extend_قاعدہ,
                          منطقی warnOn = سچ, دھارا_Use دھارا_Use_2 = دھارا_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ثابت دھارا_Use بچہ_دھارا_Use
            = دھارا_Use_2 != دھارا_USE_NONE ? دھارا_Use_2 : دھارا_Use_For_Determ(determ);
        اگر (rhsAccess.isReadOnly()) {
            اگر (Ast_گرہ_اظہاریہ* ثابت exprp = VN_CAST(underp, گرہ_اظہاریہ)) {
                // A value only read is converted to a reversed expected type.
                underp = convert_Unpacked_ترتیب(exprp, expDTypep);
            }
        }
        اگر (VN_IS(underp, گرہ_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } ورنہ اگر (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   چوڑائی_VP{expDTypep, FINAL, بچہ_دھارا_Use}.p());
        } ورنہ اگر (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   چوڑائی_VP{expDTypep, FINAL, بچہ_دھارا_Use}.p());
        } ورنہ اگر (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_گرہ* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, گرہ_اظہاریہ));
            underp = userIterateSubtreeReturnEdits(oldp, چوڑائی_VP{SELF, FINAL, بچہ_دھارا_Use}.p());
        } ورنہ اگر (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_گرہ* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, گرہ_اظہاریہ), سچ, expDTypep->چوڑائی());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, چوڑائی_VP{SELF, FINAL, بچہ_دھارا_Use}.p());
        } ورنہ اگر (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_گرہ* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, گرہ_اظہاریہ));
            underp = userIterateSubtreeReturnEdits(oldp, چوڑائی_VP{SELF, FINAL, بچہ_دھارا_Use}.p());
        } ورنہ {
            ثابت Ast_گرہ_DType* ثابت underDtp = underp->dtypep()->skipRefp();
            اگر (determ != تفویض_3 && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_گرہ* ثابت newp
                    = نیا AstConst{underp->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(underp), underp);
                underp = newp;
            }
            ثابت Ast_گرہ_DType* ثابت expDtp = expDTypep->skipRefp();
            اگر (determ == تفویض_3 && VN_IS(expDtp, گرہ_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            اگر (determ == تفویض_3 && !isIfaceArrayDType(expDtp)
                && !unpacked_Array_Sizes_مطابقت(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ثابت AstBasicDType* ثابت expBasicp = expDTypep->basicp();
            ثابت AstBasicDType* ثابت underBasicp = underp->dtypep()->basicp();
            اگر (expBasicp && underBasicp) {
                اگر (!m_enumItemp && !parentp->fileline()->warnIsOff(V3_Error_رمز::ENUMVALUE)
                    && warnOn) {
                    اگر (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, گرہ_اظہاریہ));
                    }
                    اگر (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, خالی_اشاریہ);
                    }
                }
                Ast_گرہ_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                اگر (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, چوڑائی_VP{SELF, FINAL, بچہ_دھارا_Use}.p());
                } ورنہ اگر (determ == تفویض_3) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    اگر (underp->isSigned() != subDTypep->isSigned()
                        || underp->چوڑائی() != subDTypep->چوڑائی()) {
                        subDTypep = parentp->ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(
                            std::max(subDTypep->چوڑائی(), underp->چوڑائی()),
                            std::max(subDTypep->چوڑائی_Min(), underp->چوڑائی_Min()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, چوڑائی_VP{subDTypep, FINAL, بچہ_دھارا_Use}.p());
                } ورنہ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, چوڑائی_VP{subDTypep, FINAL, بچہ_دھارا_Use}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                چوڑائی_Check_Sized(parentp, side, VN_AS(underp, گرہ_اظہاریہ), expDTypep, extend_قاعدہ,
                                warnOn);
            } ورنہ اگر (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } ورنہ اگر (ثابت AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ثابت AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                اگر (VN_IS(underp, Const) && VN_AS(underp, Const)->عدد().isNull()) {
                    // '= null' is ok
                } ورنہ اگر (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_بذریعہ_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } ورنہ اگر (expIfaceRefp->iface_بذریعہ_Cellp() != underIfaceRefp->iface_بذریعہ_Cellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_بذریعہ_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->iface_بذریعہ_Cellp()->prettyNameQ() << ").");
                } ورنہ اگر (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } ورنہ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, چوڑائی_VP{expDTypep, FINAL, بچہ_دھارا_Use}.p());
                }
            } ورنہ {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, چوڑائی_VP{expDTypep, FINAL, بچہ_دھارا_Use}.p());
            }
        }
        واپس underp;
    }

    خالی
    چوڑائی_Check_Sized(Ast_گرہ* parentp, ثابت حرف* side,
                    Ast_گرہ_اظہاریہ* underp,  // Node to be checked or have typecast added in front of
                    Ast_گرہ_DType* expDTypep, Extend_قاعدہ extend_قاعدہ, منطقی warnOn = سچ) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ثابت AstBasicDType* ثابت expBasicp = expDTypep->basicp();
        ثابت AstBasicDType* ثابت underBasicp = underp->dtypep()->basicp();
        اگر (expDTypep == underp->dtypep()) {
            واپس;  // Same type must match
        } ورنہ اگر (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#اگر VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#اگر_ختم
            واپس;
        } ورنہ {
            ثابت صحیح exp_چوڑائی = expDTypep->چوڑائی();
            صحیح exp_چوڑائی_Min = expDTypep->چوڑائی_Min();
            اگر (exp_چوڑائی_Min == 0) exp_چوڑائی_Min = exp_چوڑائی;
            ثابت منطقی bad = چوڑائی_Bad(underp, expDTypep);
            اگر ((bad || underp->چوڑائی() != exp_چوڑائی) && fixAutoExtend(underp /*ref*/, exp_چوڑائی)) {
                underp = خالی_اشاریہ;  // Changes underp
                واپس;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            اگر (VN_IS(parentp->backp(), Cast_سائز)) warnOn = جھوٹ;
            اگر (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = جھوٹ;
            اگر (VN_IS(underp, Const) && VN_AS(underp, Const)->عدد().isFromString()
                && exp_چوڑائی > underp->چوڑائی()
                && (((exp_چوڑائی - underp->چوڑائی()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = جھوٹ;
            }
            اگر ((VN_IS(parentp, جوڑو_2) && underp->چوڑائی() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->چوڑائی() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = جھوٹ;
            }
            اگر (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    exp_چوڑائی, underp->چوڑائی(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << exp_چوڑائی
                        << (exp_چوڑائی != exp_چوڑائی_Min ? " or " + cvt_مقصد_Str(exp_چوڑائی_Min) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->چوڑائی()
                        << (underp->چوڑائی() != underp->چوڑائی_Min()
                                ? " or " + cvt_مقصد_Str(underp->چوڑائی_Min())
                                : "")
                        << " bits.");
            }
            اگر (bad || underp->چوڑائی() != exp_چوڑائی) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ثابت Ast_گرہ_تفویض* assignp = VN_CAST(parentp, گرہ_تفویض);
                ثابت AstPin* pinp = VN_CAST(parentp, Pin);
                اگر (assignp && VN_IS(assignp->lhsp(), گرہ_دھارا)) {
                } ورنہ اگر (pinp && pinp->modVarp()->direction() != VDirection::داخلہ) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } ورنہ اگر (assignp && VN_IS(underp, گرہ_دھارا) && exp_چوڑائی > underp->چوڑائی()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ثابت صحیح سرکاؤ_2 = exp_چوڑائی - underp->چوڑائی();
                    underp->unlink_Fr_عقب(&linker);
                    AstExtend* ثابت widenedp = نیا AstExtend{underp->fileline(), underp};
                    widenedp->did_چوڑائی(سچ);
                    // Shift left so zeros fill on the right
                    Ast_گرہ_اظہاریہ* ثابت shiftedp = نیا Ast_سرکاؤ_L{
                        underp->fileline(), widenedp,
                        نیا AstConst{underp->fileline(), static_cast<uint32_t>(سرکاؤ_2)}, exp_چوڑائی};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } ورنہ {
                    VL_DO_DANGLING(fix_چوڑائی_Extend(underp, expDTypep, extend_قاعدہ), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_گرہ_اظہاریہ* checkCvtUS(Ast_گرہ_اظہاریہ* nodep, منطقی fatal) {
        اگر (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            اگر (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } ورنہ {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, جھوٹ, 32);
        }
        واپس nodep;
    }

    Ast_گرہ_اظہاریہ* spliceCvtD(Ast_گرہ_اظہاریہ* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        اگر (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_عقب(&linker);
            Ast_گرہ_اظہاریہ* newp;
            اگر (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = نیا AstISToRD{nodep->fileline(), nodep};
            } ورنہ {
                newp = نیا AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            واپس newp;
        } ورنہ {
            واپس nodep;
        }
    }
    Ast_گرہ_اظہاریہ* spliceCvtS(Ast_گرہ_اظہاریہ* nodep, منطقی warnOn, صحیح چوڑائی) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        اگر (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_عقب(&linker);
            اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                اگر (constp->isDouble() && V3_نمبر::epsilonIntegral(constp->عدد().مقصد_Double())) {
                    warnOn = جھوٹ;
                }
            }
            اگر (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtype_سیٹ_ثنائی_ہندسہ_Sized(چوڑائی, VSigning::SIGNED);
            واپس newp;
        } ورنہ {
            واپس nodep;
        }
    }
    Ast_گرہ_اظہاریہ* spliceCvtString(Ast_گرہ_اظہاریہ* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        اگر (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_عقب(&linker);
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            واپس newp;
        } ورنہ {
            واپس nodep;
        }
    }
    Ast_گرہ_Biop* replaceWithUOrSVersion(Ast_گرہ_Biop* nodep, منطقی signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        اگر (signedFlavorNeeded == nodep->signedFlavor()) واپس خالی_اشاریہ;
        اگر (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        انتخاب (nodep->type()) {
        حالت VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); واپس خالی_اشاریہ;
        حالت VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); واپس خالی_اشاریہ;
        حالت VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); واپس خالی_اشاریہ;
        حالت VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); واپس خالی_اشاریہ;
        حالت VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); واپس خالی_اشاریہ;
        حالت VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); واپس خالی_اشاریہ;
        حالت VNType::جوڑو_2: nodep->dtypeChgSigned(signedFlavorNeeded); واپس خالی_اشاریہ;
        حالت VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); واپس خالی_اشاریہ;
        حالت VNType::سرکاؤ_L: nodep->dtypeChgSigned(signedFlavorNeeded); واپس خالی_اشاریہ;
        طےشدہ: توڑو;
        }
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
        Ast_گرہ_Biop* newp = خالی_اشاریہ;
        انتخاب (nodep->type()) {
        حالت VNType::Gt: newp = نیا AstGtS{fl, lhsp, rhsp}; توڑو;
        حالت VNType::GtS: newp = نیا AstGt{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Gte: newp = نیا AstGteS{fl, lhsp, rhsp}; توڑو;
        حالت VNType::GteS: newp = نیا AstGte{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Lt: newp = نیا AstLtS{fl, lhsp, rhsp}; توڑو;
        حالت VNType::LtS: newp = نیا AstLt{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Lte: newp = نیا AstLteS{fl, lhsp, rhsp}; توڑو;
        حالت VNType::LteS: newp = نیا AstLte{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Div: newp = نیا AstDivS{fl, lhsp, rhsp}; توڑو;
        حالت VNType::DivS: newp = نیا AstDiv{fl, lhsp, rhsp}; توڑو;
        حالت VNType::ModDiv: newp = نیا AstModDivS{fl, lhsp, rhsp}; توڑو;
        حالت VNType::ModDivS: newp = نیا AstModDiv{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Mul: newp = نیا AstMulS{fl, lhsp, rhsp}; توڑو;
        حالت VNType::MulS: newp = نیا AstMul{fl, lhsp, rhsp}; توڑو;
        حالت VNType::سرکاؤ_R: newp = نیا Ast_سرکاؤ_RS{fl, lhsp, rhsp}; توڑو;
        حالت VNType::سرکاؤ_RS: newp = نیا Ast_سرکاؤ_R{fl, lhsp, rhsp}; توڑو;
        طےشدہ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            توڑو;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        واپس newp;
    }
    Ast_گرہ_Biop* replaceWithDVersion(Ast_گرہ_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        اگر (nodep->doubleFlavor()) واپس خالی_اشاریہ;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
        Ast_گرہ_Biop* newp = خالی_اشاریہ;
        // No width change on output;...                // All below have bool or double outputs
        انتخاب (nodep->type()) {
        حالت VNType::جوڑو_2: newp = نیا Ast_جوڑو_D{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Sub: newp = نیا AstSubD{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Pow: newp = نیا AstPowD{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Eq:
        حالت VNType::EqCase: newp = نیا AstEqD{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Neq:
        حالت VNType::NeqCase: newp = نیا AstNeqD{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Gt:
        حالت VNType::GtS: newp = نیا AstGtD{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Gte:
        حالت VNType::GteS: newp = نیا AstGteD{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Lt:
        حالت VNType::LtS: newp = نیا AstLtD{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Lte:
        حالت VNType::LteS: newp = نیا AstLteD{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Div:
        حالت VNType::DivS: newp = نیا AstDivD{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Mul:
        حالت VNType::MulS: newp = نیا AstMulD{fl, lhsp, rhsp}; توڑو;
        طےشدہ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            توڑو;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        واپس newp;
    }
    Ast_گرہ_Biop* replaceWithNVersion(Ast_گرہ_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        اگر (nodep->stringFlavor()) واپس خالی_اشاریہ;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
        Ast_گرہ_Biop* newp = خالی_اشاریہ;
        // No width change on output;...                // All below have bool or double outputs
        انتخاب (nodep->type()) {
        حالت VNType::Eq:
        حالت VNType::EqCase:
        حالت VNType::EqWild: newp = نیا AstEqN{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Neq:
        حالت VNType::NeqCase:
        حالت VNType::NeqWild: newp = نیا AstNeqN{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Gt:
        حالت VNType::GtS: newp = نیا AstGtN{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Gte:
        حالت VNType::GteS: newp = نیا AstGteN{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Lt:
        حالت VNType::LtS: newp = نیا AstLtN{fl, lhsp, rhsp}; توڑو;
        حالت VNType::Lte:
        حالت VNType::LteS: newp = نیا AstLteN{fl, lhsp, rhsp}; توڑو;
        طےشدہ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            توڑو;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        واپس newp;
    }
    Ast_گرہ_Uniop* replaceWithDVersion(Ast_گرہ_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        اگر (nodep->doubleFlavor()) واپس خالی_اشاریہ;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
        Ast_گرہ_Uniop* newp = خالی_اشاریہ;
        انتخاب (nodep->type()) {
        حالت VNType::Negate: newp = نیا AstNegateD{fl, lhsp}; توڑو;
        طےشدہ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            توڑو;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        واپس newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    خالی replaceWithSFormat(Ast_طریقہ_Call* nodep, ثابت string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ثابت argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_گرہ_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_عقب(), VarRef);
        Ast_گرہ* ثابت newp = نیا Ast_تفویض{
            nodep->fileline(), fromp,
            نیا AstSFormatF{nodep->fileline(), format, جھوٹ, argp->exprp()->unlink_Fr_عقب()}};
        fromp->access(VAccess::WRITE);
        ڈالو_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_گرہ_DType* iterateEditMoveDTypep(Ast_گرہ* parentp, Ast_گرہ_DType* dtnodep) {
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
        ثابت منطقی child1 = (parentp->get_بچہ_DTypep() == dtnodep);
        ثابت منطقی child2 = (parentp->getChild2DTypep() == dtnodep);
        اگر (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, خالی_اشاریہ), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->get_بچہ_DTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->did_چوڑائی(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlink_Fr_عقب();
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(dtnodep);
        }
        اگر (!dtnodep->did_چوڑائی()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, خالی_اشاریہ);
            UASSERT_OBJ(dtnodep->did_چوڑائی(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        واپس dtnodep;
    }

    AstConst* dimension_قدر(File_سطر* fileline, Ast_گرہ_DType* nodep, VAttrType attrType,
                             صحیح dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_گرہ_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        برائے (صحیح i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            اگر (ثابت Ast_گرہ_Array_DType* ثابت adtypep = VN_CAST(dtypep, گرہ_Array_DType)) {
                declRange = adtypep->declRange();
                اگر (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                جاری;
            } ورنہ اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت adtypep
                       = VN_CAST(dtypep, گرہ_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                توڑو;  // Sub elements don't look like arrays and can't iterate into
            } ورنہ اگر (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                اگر (adtypep->isRanged()) declRange = adtypep->declRange();
                توڑو;
            }
            توڑو;  // LCOV_EXCL_LINE
        }
        AstConst* valp = خالی_اشاریہ;  // If nullptr, construct from val
        صحیح val = 0;
        انتخاب (attrType) {
        حالت VAttrType::DIM_بٹ: {
            صحیح بٹ = 1;
            جب (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                اگر (ثابت Ast_گرہ_Array_DType* ثابت adtypep = VN_CAST(dtypep, گرہ_Array_DType)) {
                    بٹ *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    جاری;
                } ورنہ اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت adtypep
                           = VN_CAST(dtypep, گرہ_UOr_Struct_DType)) {
                    بٹ *= adtypep->isStreamableFixedAggregate() ? adtypep->چوڑائی_دھارا()
                                                                  : adtypep->چوڑائی();
                    توڑو;
                } ورنہ اگر (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                    بٹ *= adtypep->چوڑائی();
                    توڑو;
                }
                توڑو;
            }
            اگر (dim == 0) {
                val = 0;
            } ورنہ اگر (dim == 1 && !declRange.ranged()
                       && بٹ == 1) {  // $bits should be sane for non-arrays
                val = nodep->چوڑائی();
            } ورنہ {
                val = بٹ;
            }
            توڑو;  // LCOV_EXCL_LINE
        }
        حالت VAttrType::DIM_بلند: val = !declRange.ranged() ? 0 : declRange.hi(); توڑو;
        حالت VAttrType::DIM_بایاں: val = !declRange.ranged() ? 0 : declRange.بائیں(); توڑو;
        حالت VAttrType::DIM_کم: val = !declRange.ranged() ? 0 : declRange.lo(); توڑو;
        حالت VAttrType::DIM_دایاں: val = !declRange.ranged() ? 0 : declRange.دائیں(); توڑو;
        حالت VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            توڑو;
        حالت VAttrType::DIM_سائز: val = !declRange.ranged() ? 0 : declRange.elements(); توڑو;
        طےشدہ: nodep->v3fatalSrc("Missing DIM ATTR type case"); توڑو;
        }
        اگر (!valp) valp = نیا AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvt_مقصد_Hex(dtypep) << "," << dim
                                << ")=" << valp);
        واپس valp;
    }
    AstVarRef* dimensionVarRefp(Ast_گرہ_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_جدول_Map[std::make_pair(nodep, attrType)];
        اگر (cachedpr) واپس cachedpr->clone_درخت(جھوٹ);

        Ast_گرہ_Array_DType* ثابت vardtypep = نیا AstUnpackArrayDType{
            nodep->fileline(), nodep->ڈھونڈو_Int_DType(), نیا AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(vardtypep);
        userIterate(vardtypep, چوڑائی_VP{SELF, BOTH}.p());
        AstConsArray* ثابت consp = نیا AstConsArray{nodep->fileline(), vardtypep};
        برائے (غیرعلامتی i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_قدر(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::ڈھونڈو(consp);
        VL_DO_DANGLING(consp->delete_درخت(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->did_چوڑائی(سچ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_چوڑائی(سچ);
        ڈالو_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        واپس cachedpr->clone_درخت(جھوٹ);
    }
    ساکن uint64_t enum_Max_قدر(ثابت Ast_گرہ* errNodep, ثابت AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        برائے (ثابت Ast_Enum_شے* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شے)) {
            ثابت AstConst* ثابت vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            اگر (!vconstp->عدد().isAnyXZ() && vconstp->مقصد_UQuad() >= maxval)
                maxval = vconstp->مقصد_UQuad();
        }
        اگر (adtypep->itemsp()->چوڑائی() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            واپس 64;
        }
        واپس maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ثابت nodep, VAttrType attrType, منطقی assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_جدول_Map[std::make_pair(nodep, attrType)];
        اگر (cachedpr) واپس cachedpr->clone_درخت(جھوٹ);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_DType* basep;
        اگر (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->ڈھونڈو_String_DType();
        } ورنہ اگر (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->ڈھونڈو_ثنائی_ہندسہ_DType();
        } ورنہ {
            basep = nodep->dtypep();
        }
        Ast_گرہ_DType* vardtypep;
        اگر (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = نیا AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } ورنہ {
            vardtypep = نیا AstUnpackArrayDType{flp, basep, نیا AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = خالی_اشاریہ;
        اگر (attrType == VAttrType::ENUM_NAME) {
            defaultp = نیا AstConst{flp, AstConst::String{}, ""};
        } ورنہ اگر (attrType == VAttrType::ENUM_اگلا || attrType == VAttrType::ENUM_پچھلا) {
            defaultp = نیا AstConst{flp, V3_نمبر{nodep, nodep->چوڑائی(), 0}};
        } ورنہ اگر (attrType == VAttrType::ENUM_VALID) {
            defaultp = نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};
        } ورنہ {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_گرہ_اظہاریہ*> اقدار;
        {
            uint64_t ordinal = 0;
            Ast_Enum_شے* ثابت firstp = nodep->itemsp();
            ثابت Ast_Enum_شے* prevp = firstp;  // Prev must start with last item
            جب (prevp->nextp()) prevp = VN_AS(prevp->nextp(), Enum_شے);
            برائے (Ast_Enum_شے* itemp = firstp; itemp;) {
                Ast_Enum_شے* ثابت nextp = VN_AS(itemp->nextp(), Enum_شے);
                ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                اگر (!vconstp->عدد().isAnyXZ()) {  // Can 2-state runtime decode
                    ثابت uint64_t i = nodep->isWide() ? ++ordinal : vconstp->مقصد_UQuad();
                    اگر (attrType == VAttrType::ENUM_NAME) {
                        اقدار[i] = نیا AstConst{flp, AstConst::String{},
                                                 V3_نمبر::displayedEnumName(itemp)};
                    } ورنہ اگر (attrType == VAttrType::ENUM_اگلا) {
                        اقدار[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_درخت(جھوٹ);  // A const
                    } ورنہ اگر (attrType == VAttrType::ENUM_پچھلا) {
                        اقدار[i] = prevp->valuep()->clone_درخت(جھوٹ);  // A const
                    } ورنہ اگر (attrType == VAttrType::ENUM_VALID) {
                        اقدار[i] = نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
                    } ورنہ {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_گرہ_اظہاریہ* valuep = خالی_اشاریہ;
        اگر (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ثابت consp = نیا AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            برائے (ثابت خودکار& itr : اقدار) {
                V3_نمبر کلید{nodep, nodep->چوڑائی()};
                کلید.سیٹ_Quad(itr.پہلا);
                consp->جوڑو_Itemsp(نیا Ast_Cons_Assoc_شے{flp, نیا AstConst{flp, کلید}, itr.دوسرا});
            }
            valuep = consp;
        } ورنہ {
            AstConsArray* ثابت consp = نیا AstConsArray{flp, vardtypep, defaultp};
            برائے (uint64_t i = 0; i < (msbdim + 1); ++i) {
                اگر (اقدار[i]) consp->valuep(i, اقدار[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::ڈھونڈو(valuep);
        VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->did_چوڑائی(سچ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_چوڑائی(سچ);
        ڈالو_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        واپس cachedpr->clone_درخت(جھوٹ);
    }

    Ast_گرہ_اظہاریہ* enumSelect(Ast_گرہ_اظہاریہ* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        اگر (attrType == VAttrType::ENUM_NAME) {
            اگر (nodep->is_خالص()) nodep = V3Const::constifyEdit(nodep);
            اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                Ast_گرہ_اظہاریہ* ثابت newp = نیا AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->عدد().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس newp;
            }
        }
        اگر (attrType == VAttrType::ENUM_NAME && !nodep->is_خالص() && (m_modep || m_ftaskp)) {
            Ast_اظہاریہ_Stmt* ثابت capturep = newEnumCapture(nodep, adtypep);
            Ast_گرہ_اظہاریہ* ثابت valuep = capturep->resultp()->unlink_Fr_عقب();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtype_سیٹ_String();
            واپس capturep;
        }
        اگر (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_گرہ_اظہاریہ* indexp = نیا AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            برائے (ثابت Ast_Enum_شے* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_شے)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                اگر (constp->عدد().isAnyXZ()) جاری;
                indexp = نیا AstCond{nodep->fileline(),
                                     نیا AstEq{nodep->fileline(), nodep->clone_درخت(جھوٹ),
                                               itemp->valuep()->clone_درخت(جھوٹ)},
                                     نیا AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, جھوٹ, ordinal);
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtype_سیٹ_String();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس newp;
        }
        ثابت uint64_t msbdim = enum_Max_قدر(nodep, adtypep);
        ثابت منطقی assoc = msbdim > ENUM_LOOKUP_بٹ;
        Ast_گرہ_اظہاریہ* newp;
        اگر (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, سچ, 0);
            newp = نیا AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } ورنہ {
            ثابت صحیح selwidth = V3_نمبر::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(adtypep, attrType, جھوٹ, (1ULL << selwidth) - 1);
            Ast_گرہ_اظہاریہ* ثابت boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->چوڑائی() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? نیا AstGt{nodep->fileline(), nodep->clone_درخت(جھوٹ),
                                  نیا AstConst{nodep->fileline(),
                                               V3_نمبر{nodep, adtypep->چوڑائی(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : خالی_اشاریہ;
            newp = نیا AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                نیا AstSel{nodep->fileline(), nodep, 0, selwidth}};
            اگر (boundp) {
                newp = نیا AstCond{nodep->fileline(), boundp,
                                   نیا AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        اگر (attrType == VAttrType::ENUM_NAME) {
            newp->dtype_سیٹ_String();
        } ورنہ اگر (attrType == VAttrType::ENUM_VALID) {
            newp->dtype_سیٹ_ثنائی_ہندسہ();
        } ورنہ {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        واپس newp;
    }
    Ast_گرہ_اظہاریہ* enumTestValid(Ast_گرہ_اظہاریہ* valp, AstEnumDType* enumDtp) {
        ثابت uint64_t maxval = enum_Max_قدر(valp, enumDtp);
        ثابت منطقی assoc = maxval > ENUM_LOOKUP_بٹ;
        Ast_گرہ_اظہاریہ* testp = خالی_اشاریہ;
        File_سطر* ثابت fl_novalue = نیا File_سطر{valp->fileline()};
        fl_novalue->warnOff(V3_Error_رمز::ENUMVALUE, سچ);
        fl_novalue->warnOff(V3_Error_رمز::CMPCONST, سچ);
        اگر (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, سچ, 0);
            testp = نیا AstAssocSel{fl_novalue, tabRefp, valp->clone_درخت_خالص(جھوٹ)};
        } ورنہ {
            ثابت صحیح selwidth = V3_نمبر::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, جھوٹ, (1ULL << selwidth) - 1);
            File_سطر* ثابت fl_nowidth = نیا File_سطر{fl_novalue};
            fl_nowidth->warnOff(V3_Error_رمز::چوڑائی_2, سچ);
            testp = نیا AstCond{
                fl_novalue,
                نیا AstGt{fl_nowidth, valp->clone_درخت_خالص(جھوٹ),
                          نیا AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                نیا AstConst{fl_novalue, AstConst::ثنائی_ہندسہ_False{}},
                نیا AstArraySel{fl_novalue, tabRefp,
                                نیا AstSel{fl_novalue, valp->clone_درخت_خالص(جھوٹ), 0, selwidth}}};
        }
        واپس testp;
    }

    PatVecMap pat_سمتیہ_Map(Ast_نمونہ* nodep, ثابت VNumRange& range) {
        PatVecMap patmap;
        صحیح element = range.بائیں();
        برائے (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            اگر (patp->keyp()) {
                اگر (ثابت AstConst* ثابت constp = نمونہ_Const_Keyp(patp)) {
                    اگر (constp->عدد().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } ورنہ {
                        element = constp->مقصد_SInt();
                    }
                }
            }
            ثابت منطقی new_اندراج = patmap.emplace(element, patp).دوسرا;
            اگر (!new_اندراج) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.بایاں_مقصد_دایاں_Inc();
        }
        واپس patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    خالی patConcatConvertRecurse(Ast_نمونہ* patternp, Ast_گرہ_اظہاریہ* nodep) {
        اگر (AstConcat* ثابت concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } ورنہ {
            AstPatMember* ثابت newp
                = نیا AstPatMember{nodep->fileline(), nodep->unlink_Fr_عقب(), خالی_اشاریہ, خالی_اشاریہ};
            newp->isConcat(سچ);
            patternp->جوڑو_Itemsp(newp);
        }
    }

    خالی makeOpenArrayShell(Ast_گرہ_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_گرہ_FTask* ثابت oldTaskp = nodep->taskp();
        oldTaskp->dpi_Open_والد_Inc();
        UASSERT_OBJ(!oldTaskp->dpi_Open_بچہ(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_گرہ_FTask* ثابت newTaskp = oldTaskp->clone_درخت(جھوٹ);
        newTaskp->dpi_Open_بچہ(سچ);
        newTaskp->dpi_Open_والد_Clear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvt_مقصد_Str(oldTaskp->dpi_Open_والد()));
        oldTaskp->جوڑو_اگلا_یہاں(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        برائے (ثابت خودکار& tconnect : tconnects) {
            AstVar* ثابت portp = tconnect.پہلا;
            ثابت AstArg* ثابت argp = tconnect.دوسرا;
            ثابت Ast_گرہ* ثابت pinp = argp->exprp();
            اگر (!pinp) جاری;  // Argument error we'll find later
            اگر (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_گرہ_DType* ثابت oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_گرہ* nodep) {
                    اگر (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    منطقی markHasOpenArray(Ast_گرہ_FTask* nodep) {
        منطقی hasOpen = جھوٹ;
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(سچ);
                    hasOpen = سچ;
                }
            }
        }
        واپس hasOpen;
    }
    منطقی hasOpenArrayDTypeRecurse(Ast_گرہ_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        اگر (VN_IS(nodep, UnsizedArrayDType)) واپس سچ;
        اگر (nodep->subDTypep()) واپس hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        واپس جھوٹ;
    }
    Ast_گرہ_DType* dtypeNotIntAtomOrVecRecurse(Ast_گرہ_DType* nodep, منطقی ranged = جھوٹ) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skip_Ref_مقصد_Enump();
        اگر (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            اگر (ranged && (!dtp->is_ثنائی_ہندسہ_Logic() || dtp->isRanged()))
                واپس dtp;  // Packed when already packed
            اگر (dtp->keyword().isIntNumeric()) واپس خالی_اشاریہ;
            واپس dtp;
        } ورنہ اگر (AstPackArrayDType* ثابت dtp = VN_CAST(nodep, PackArrayDType)) {
            اگر (ranged) واپس dtp;  // Packed when already packed
            واپس dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), سچ);
        }
        واپس nodep;
    }
    Ast_گرہ_DType* dtypeNot4StateIntegralRecurse(Ast_گرہ_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        اگر (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            اگر (!dtp->keyword().isFourstate()) واپس dtp;
            واپس خالی_اشاریہ;
        } ورنہ اگر (Ast_گرہ_Array_DType* ثابت dtp = VN_CAST(nodep, گرہ_Array_DType)) {
            واپس dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } ورنہ اگر (Ast_گرہ_UOr_Struct_DType* ثابت dtp = VN_CAST(nodep, گرہ_UOr_Struct_DType)) {
            برائے (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_گرہ_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                اگر (badDtp) واپس badDtp;
            }
            واپس خالی_اشاریہ;
        }
        واپس nodep;
    }
    خالی checkForceReleaseLhs(Ast_گرہ* nodep, Ast_گرہ* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        اگر (Ast_گرہ* ثابت selNodep = V3_چوڑائی::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    خالی assert_At_اظہاریہ(Ast_گرہ* nodep) {
        اگر (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    خالی assertAtStatement(Ast_گرہ* nodep) {
        اگر (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    خالی checkConstantOrReplace(Ast_گرہ* nodep, منطقی noFourState, ثابت string& پیغام) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        اگر (!constp || (noFourState && constp->عدد().isFourState())) {
            nodep->v3error(پیغام);
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ڈالو_Deletep(nodep), nodep, constp);
            واپس;
        }
    }
    Ast_گرہ* گرہ_For_Unsized_Warning(Ast_گرہ* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        اگر (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->چوڑائی_Sized()) {
            واپس nodep->op1p();
        } ورنہ اگر (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->چوڑائی_Sized()) {
            واپس nodep->op2p();
        }
        واپس nodep;  // By default return this
    }
    AstRefDType* check_Ref_مقصد_Typedef_Recurse(Ast_گرہ* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        اگر (VL_LIKELY(!nodep)) واپس خالی_اشاریہ;
        اگر (خودکار* ثابت refp = VN_CAST(nodep, RefDType)) {
            اگر (refp->typedefp() == typedefp) واپس refp;
        }
        اگر (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep->op1p(), typedefp)) واپس refp;
        اگر (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep->op2p(), typedefp)) واپس refp;
        اگر (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep->op3p(), typedefp)) واپس refp;
        اگر (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep->op4p(), typedefp)) واپس refp;
        واپس خالی_اشاریہ;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    ساکن منطقی isIfaceArrayDType(ثابت Ast_گرہ_DType* dtypep) {
        اگر (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) واپس جھوٹ;
        ثابت AstIfaceRefDType* ثابت irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        واپس irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    ساکن std::string unpacked_Array_سائز(ثابت Ast_گرہ_DType* dtypep) {
        std::string سائز;
        dtypep = dtypep->skipRefp();
        جب (ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            اگر (!سائز.خالی_خانے()) سائز += "x";
            سائز += std::مقصد_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        واپس سائز;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    ساکن Ast_گرہ_اظہاریہ* newIfaceArrayElement(Ast_گرہ_اظہاریہ* fromp, صحیح ثنائی_ہندسہ_2) {
        AstConsArray* ثابت consp = VN_CAST(fromp, ConsArray);
        اگر (!consp || !isIfaceArrayDType(consp->dtypep())) واپس خالی_اشاریہ;
        ثابت AstUnpackArrayDType* ثابت arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (ثنائی_ہندسہ_2 < 0 || ثنائی_ہندسہ_2 >= arrp->elementsConst()) واپس خالی_اشاریہ;
        واپس consp->valuep(ثنائی_ہندسہ_2)->clone_درخت(جھوٹ);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_گرہ_اظہاریہ* new_Iface_Array_قدر(ثابت Ast_گرہ_Var_Ref* refp, Ast_گرہ_DType* dtypep,
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
            Ast_گرہ_Var_Ref* newp;
            اگر (ثابت AstVarXRef* ثابت xrefp = VN_CAST(refp, VarXRef)) {
                newp = نیا AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } ورنہ {
                newp = نیا AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, چوڑائی_VP{SELF, BOTH}.p());
            واپس newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ثابت consp = نیا AstConsArray{flp, arrp};
        ثابت VNumRange range = arrp->declRange();
        برائے (ثابت صحیح i : range.seq_بایاں_مقصد_دایاں()) {
            ثابت std::string s = name + "__BRA__" + Ast_گرہ::encode_نمبر(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_قدر(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        واپس consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    خالی replace_بانٹو_Iface_Array_Ref(Ast_گرہ_Var_Ref* nodep) {
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp->is_Iface_Array_بانٹو(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        اگر (varp->is_Iface_والد()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_قدر(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    خالی expandIfaceArrayPinDimensions(AstPin* nodep, ثابت Ast_گرہ_DType* dtypep,
                                       Ast_گرہ_اظہاریہ* conp, ثابت std::string& suffix,
                                       AstVar*& elemVarpr) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        اگر (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ثابت newp = nodep->clone_درخت(جھوٹ);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            اگر (conp) newp->exprp(conp->clone_درخت(جھوٹ));
            nodep->جوڑو_اگلا_یہاں(newp);
            واپس;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ثابت VNumRange range = arrp->declRange();
        برائے (صحیح n = range.elements() - 1; n >= 0; --n) {
            ثابت صحیح i = range.بائیں() + n * range.بایاں_مقصد_دایاں_Inc();
            ثابت std::string s = suffix + "__BRA__" + Ast_گرہ::encode_نمبر(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_گرہ_اظہاریہ* ثابت elemConnp = [&]() -> Ast_گرہ_اظہاریہ* {
                اگر (!conp) واپس خالی_اشاریہ;
                ثابت AstUnpackArrayDType* ثابت conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ثابت صحیح elements = conArrp->elementsConst();
                // Counted from lo
                Ast_گرہ_اظہاریہ* ثابت resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                واپس resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    خالی expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ثابت portp = nodep->modVarp();
        UASSERT_OBJ(portp->is_Iface_Array_بانٹو(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        برائے (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_گرہ_اظہاریہ* ثابت exprp = VN_AS(nodep->exprp(), گرہ_اظہاریہ);
        اگر (exprp) ڈالو_Deletep(exprp->unlink_Fr_عقب());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ثابت منطقی connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpacked_Array_سائز(exprp->dtypep()) == unpacked_Array_سائز(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : خالی_اشاریہ, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_گرہ* userIterateSubtreeReturnEdits(Ast_گرہ* nodep, چوڑائی_VP* vup) {
        اگر (!nodep) واپس خالی_اشاریہ;
        Ast_گرہ* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        واپس ret;
    }
    خالی userIterate(Ast_گرہ* nodep, چوڑائی_VP* vup) {
        اگر (!nodep) واپس;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    خالی user_Iterate_And_اگلا(Ast_گرہ* nodep, چوڑائی_VP* vup) {
        اگر (!nodep) واپس;
        اگر (nodep->did_چوڑائی()) واپس;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate_And_اگلا_Null(nodep);
    }
    خالی userIterateChildren(Ast_گرہ* nodep, چوڑائی_VP* vup) {
        اگر (!nodep) واپس;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    خالی userIterateChildrenBackwardsConst(Ast_گرہ* nodep, چوڑائی_VP* vup) {
        اگر (!nodep) واپس;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

عوامی:
    // CONSTRUCTORS
    چوڑائی_Visitor(منطقی paramsOnly,  // [in] TRUE if we are considering parameters only.
                 منطقی doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enum_عارضی_نام{"__Venum"}
        , m_inside_عارضی_نام{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_گرہ* mainAcceptEdit(Ast_گرہ* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, چوڑائی_VP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        برائے (AstVar* ثابت varp : m_بانٹو_Iface_Array_Varps) {
            VL_DO_DANGLING(ڈالو_Deletep(varp->unlink_Fr_عقب()), varp);
        }
        m_بانٹو_Iface_Array_Varps.clear();
        واپس nodep;
    }
    ساکن منطقی زیریں_As_Fixed_Aggregate(ثابت Ast_گرہ_DType* ثابت dtypep) {
        واپس dtypep->isStreamableFixedAggregate() && dtypep->شامل_ہے_Unpacked_Struct();
    }
    خالی دھارا_تفویض_زیریں(Ast_گرہ_تفویض* nodep) {
        Ast_گرہ_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        اگر (Ast_گرہ_دھارا* ثابت streamp = VN_CAST(nodep->rhsp(), گرہ_دھارا)) {
            Ast_گرہ_DType* ثابت lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ثابت صحیح lwidth = lhsDTypeSkippedRefp->چوڑائی_دھارا();
            ثابت صحیح rwidth = streamp->lhsp()->dtypep()->skipRefp()->چوڑائی_دھارا();
            اگر (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            اگر (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), قطار_DType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), قطار_DType)) {
                ثابت صحیح قطار_Element_سائز = streamp->lhsp()->dtypep()->subDTypep()->چوڑائی();
                UASSERT_OBJ(قطار_Element_سائز <= lwidth, nodep, "LHS < RHS");
            }
            اگر (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || زیریں_As_Fixed_Aggregate(lhsDTypeSkippedRefp)) {
                streamp->unlink_Fr_عقب();
                nodep->rhsp(
                    نیا Ast_Cvt_Packed_مقصد_Array{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        اگر (Ast_گرہ_دھارا* ثابت streamp = VN_CAST(nodep->lhsp(), گرہ_دھارا)) {
            ثابت Ast_گرہ_DType* ثابت rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_گرہ_DType* ثابت lhs_دھارا_DTypep = streamp->lhsp()->dtypep()->skipRefp();
            ثابت صحیح lwidth = lhs_دھارا_DTypep->چوڑائی_دھارا();
            ثابت صحیح rwidth = rhsDTypep->چوڑائی_دھارا();
            اگر (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            اگر (زیریں_As_Fixed_Aggregate(lhs_دھارا_DTypep)) {
                Ast_گرہ_اظہاریہ* ثابت دھارا_Exprp = nodep->lhsp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* ثابت dstp = streamp->lhsp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* srcp = nodep->rhsp()->unlink_Fr_عقب();
                اگر (VN_IS(streamp, دھارا_L)) {
                    streamp->lhsp(srcp);
                    streamp->dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(
                        srcp->چوڑائی(), srcp->چوڑائی_Min(), VSigning::UNSIGNED,
                        Ast_گرہ_اظہاریہ::isFourstateOrNoDtype(دھارا_Exprp, srcp));
                    srcp = دھارا_Exprp;
                } ورنہ {
                    اگر (srcp->چوڑائی() > lwidth) {
                        srcp = نیا AstSel{streamp->fileline(), srcp, srcp->چوڑائی() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ڈالو_Deletep(دھارا_Exprp), دھارا_Exprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(نیا Ast_Cvt_Packed_مقصد_Array{srcp->fileline(), srcp, lhs_دھارا_DTypep});
                nodep->dtypeFrom(dstp);
            } ورنہ اگر (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
                nodep->rhsp(نیا Ast_Cvt_Array_مقصد_Packed{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~چوڑائی_Visitor() override = طےشدہ;
};

//######################################################################
// Width class functions

خالی V3_چوڑائی::چوڑائی(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ثابت چوڑائی_Clear_Visitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ثابت VDTypeNameScopes scopes{nodep};
        چوڑائی_Visitor visitor{جھوٹ, جھوٹ};
        (خالی)visitor.mainAcceptEdit(nodep);
        چوڑائی_Remove_Visitor rvisitor;
        (خالی)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("width", 0, dump_درخت_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_گرہ* V3_چوڑائی::چوڑائی_Params_Edit(Ast_گرہ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    چوڑائی_Visitor visitor{سچ, جھوٹ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    واپس nodep;
}

خالی V3_چوڑائی::دھارا_تفویض_زیریں_Edit(Ast_گرہ_تفویض* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    چوڑائی_Visitor visitor{جھوٹ, جھوٹ};
    visitor.دھارا_تفویض_زیریں(nodep);
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
Ast_گرہ* V3_چوڑائی::چوڑائی_Generate_Params_Edit(
    Ast_گرہ* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    چوڑائی_Visitor visitor{سچ, سچ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    واپس nodep;
}

Ast_گرہ* V3_چوڑائی::selectNonConstantRecurse(Ast_گرہ* nodep, منطقی اندر_Sel) {
    // If node has a non-constant select, return that select
    Ast_گرہ* resultp = خالی_اشاریہ;
    اگر (Ast_گرہ_Sel* ثابت anodep = VN_CAST(nodep, گرہ_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), اندر_Sel);
        اگر (resultp) واپس resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), سچ);
    } ورنہ اگر (AstSel* ثابت anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), اندر_Sel);
        اگر (resultp) واپس resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), سچ);
    } ورنہ اگر (Ast_گرہ_Var_Ref* ثابت anodep = VN_CAST(nodep, گرہ_Var_Ref)) {
        اگر (اندر_Sel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) واپس anodep;
    } ورنہ {
        اگر (Ast_گرہ* ثابت refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, اندر_Sel);
        اگر (resultp) واپس resultp;
        اگر (Ast_گرہ* ثابت refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, اندر_Sel);
        اگر (resultp) واپس resultp;
        اگر (Ast_گرہ* ثابت refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, اندر_Sel);
        اگر (resultp) واپس resultp;
        اگر (Ast_گرہ* ثابت refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, اندر_Sel);
    }
    واپس resultp;
}
