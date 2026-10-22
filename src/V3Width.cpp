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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Width.h"

#ಅನ್ತರ್ಭಾವ "V3Assert.h"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Begin.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3LinkLValue.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3Number.h"
#ಅನ್ತರ್ಭಾವ "V3Randomize.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"
#ಅನ್ತರ್ಭಾವ "V3Task.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"
#ಅನ್ತರ್ಭಾವ "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3_ಅಗಲ_CPP
#ಅನ್ತರ್ಭಾವ "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*, AstClass*> m_cache;

    AstClass* ಕಂಡುಹಿಡಿ_Cached(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_cache.ಕಂಡುಹಿಡಿ(nodep);
        ಆದರೆ (it != m_cache.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;

        AstClass* classp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = ಕಂಡುಹಿಡಿ_Cached(nodep->backp());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClassPackage* ಸ್ಥಿರಾಂಕ packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } ಇಲ್ಲದಿದ್ದರೆ {
            classp = ಕಂಡುಹಿಡಿ_Cached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        ಹಿಂತಿರುಗಿ classp;
    }

ಸಾರ್ವಜನಿಕ:
    AstClass* ಕಂಡುಹಿಡಿ(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep, Class)) ಹಿಂತಿರುಗಿ classp;
        ಆದರೆ (AstClassPackage* ಸ್ಥಿರಾಂಕ packagep = VN_CAST(nodep, ClassPackage)) {
            ಹಿಂತಿರುಗಿ packagep->classp();
        }
        ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Cached(nodep);
    }
};

//######################################################################

ಎಣಿಕೆ Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& str, ಸ್ಥಿರಾಂಕ Stage& rhs) {
    ಹಿಂತಿರುಗಿ str << ("-PFB"[static_cast<ಪೂರ್ಣಾಂಕ>(rhs)]);
}

ಎಣಿಕೆ Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ನಿಯೋಜಿಸು_3  // Assignment-like where sign comes from RHS only
};
std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& str, ಸ್ಥಿರಾಂಕ Determ& rhs) {
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ s_det[] = {"SELF", "CNTX", "ASSN"};
    ಹಿಂತಿರುಗಿ str << s_det[rhs];
}

ಎಣಿಕೆ ಹರಿವು_Use : uint8_t {
    ಹರಿವು_USE_NONE,  // Stream concatenation is not allowed in this context
    ಹರಿವು_USE_ನಿಯೋಜಿಸು,  // Assignment-like context permits stream concatenation
    ಹರಿವು_USE_CAST,  // Explicit cast context permits stream concatenation
};

#ವ್ಯಾಖ್ಯಾನಿಸು v3widthWarn(lhs, rhs, ಸಂದೇಶ_2) \
    v3warn_ಸಂಕೇತ(((lhs) < (rhs)   ? V3_Error_ಸಂಕೇತ::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3_Error_ಸಂಕೇತ::WIDTHEXPAND \
                                : V3_Error_ಸಂಕೇತ::ಅಗಲ_2), \
               ಸಂದೇಶ_2)

//######################################################################
// Width state, as a visitor of each AstNode

ವರ್ಗ ಅಗಲ_VP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ m_dtypep;  // Parent's data type to resolve to
    ಸ್ಥಿರಾಂಕ Stage m_stage;  // If true, report errors
    ಸ್ಥಿರಾಂಕ ಹರಿವು_Use m_ಹರಿವು_Use;  // Current expression may be a stream concat

ಸಾರ್ವಜನಿಕ:
    ಅಗಲ_VP(Ast_ಗಂಟು_DType* dtypep, Stage stage, ಹರಿವು_Use ಹರಿವು_Use_2 = ಹರಿವು_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_ಹರಿವು_Use{ಹರಿವು_Use_2} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    ಅಗಲ_VP(Determ determ, Stage stage, ಹರಿವು_Use ಹರಿವು_Use_2 = ಹರಿವು_USE_NONE)
        : m_dtypep{ಶೂನ್ಯಸೂಚಕ}
        , m_stage{stage}
        , m_ಹರಿವು_Use{ಹರಿವು_Use_2} {
        ಆದರೆ (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    ಅಗಲ_VP* p() { ಹಿಂತಿರುಗಿ ಇದು; }
    ತಾರ್ಕಿಕ selfDtm() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dtypep == ಶೂನ್ಯಸೂಚಕ; }
    ಹರಿವು_Use ಹರಿವು_Use_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಹರಿವು_Use; }
    Ast_ಗಂಟು_DType* dtypep() ಸ್ಥಿರಾಂಕ {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        ಹಿಂತಿರುಗಿ m_dtypep;
    }
    Ast_ಗಂಟು_DType* dtypeNullp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dtypep; }
    Ast_ಗಂಟು_DType* dtypeNullSkipRefp() ಸ್ಥಿರಾಂಕ {
        Ast_ಗಂಟು_DType* dtp = dtypeNullp();
        ಆದರೆ (dtp) dtp = dtp->skipRefp();
        ಹಿಂತಿರುಗಿ dtp;
    }
    Ast_ಗಂಟು_DType* dtypeOverridep(Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ defaultp) ಸ್ಥಿರಾಂಕ {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        ಆದರೆ (!m_dtypep) ಹಿಂತಿರುಗಿ defaultp;
        ಆದರೆ (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            ಹಿಂತಿರುಗಿ defaultp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(m_dtypep->ಅಗಲ(), m_dtypep->ಅಗಲ_Min(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        ಹಿಂತಿರುಗಿ m_dtypep;
    }
    ಪೂರ್ಣಾಂಕ ಅಗಲ() ಸ್ಥಿರಾಂಕ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ಹಿಂತಿರುಗಿ m_dtypep->ಅಗಲ();
    }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Min() ಸ್ಥಿರಾಂಕ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ಹಿಂತಿರುಗಿ m_dtypep->ಅಗಲ_Min();
    }
    ತಾರ್ಕಿಕ prelim() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stage & PRELIM; }
    ತಾರ್ಕಿಕ final() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stage & FINAL; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } ಇಲ್ಲದಿದ್ದರೆ {
            str << "  VUP(s=" << m_stage << ",dt=" << cvt_ಗಮ್ಯ_Hex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& str, ಸ್ಥಿರಾಂಕ ಅಗಲ_VP* vup) {
    ಆದರೆ (vup) vup->dump(str);
    ಹಿಂತಿರುಗಿ str;
}

//######################################################################

ವರ್ಗ ಅಗಲ_Clear_Visitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    ಶೂನ್ಯ clear_ಅಗಲ_Recurse(Ast_ಗಂಟು* nodep) {
        ಗಾಗಿ (; nodep; nodep = nodep->nextp()) {
            nodep->did_ಅಗಲ(ಸುಳ್ಳು);
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op1p()) clear_ಅಗಲ_Recurse(refp);
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op2p()) clear_ಅಗಲ_Recurse(refp);
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op3p()) clear_ಅಗಲ_Recurse(refp);
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op4p()) clear_ಅಗಲ_Recurse(refp);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಅಗಲ_Clear_Visitor(AstNetlist* nodep) { clear_ಅಗಲ_Recurse(nodep); }
    ವಾಸ್ತವಿಕ ~ಅಗಲ_Clear_Visitor() = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################

#ವ್ಯಾಖ್ಯಾನಿಸು accept ಒಳ_ಅಗಲ_Visitor_use_Ast_ಗಂಟು_iterate_instead_ದ_Ast_ಗಂಟು_accept

//######################################################################

ವರ್ಗ ಅಗಲ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES
    ಬಳಸಿ PatVecMap = std::map<ಪೂರ್ಣಾಂಕ, AstPatMember*>;
    ಬಳಸಿ DTypeMap = std::map<ಸ್ಥಿರಾಂಕ std::string, AstPatMember*>;

    // STATE
    V3_Unique_ಹೆಸರುಗಳು m_enum_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು;  // Captured enum expressions
    V3_Unique_ಹೆಸರುಗಳು m_inside_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstModport*, std::unordered_ಗಣ<string>>
        m_modport_ಹೆಸರುಗಳು;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    ಅಗಲ_VP* m_vup = ಶೂನ್ಯಸೂಚಕ;  // Current node state
    ತಾರ್ಕಿಕ m_underFork = ಸುಳ್ಳು;  // Visiting under a fork
    ತಾರ್ಕಿಕ m_underSExpr = ಸುಳ್ಳು;  // Visiting under a sequence expression
    ತಾರ್ಕಿಕ m_underPackedArray = ಸುಳ್ಳು;  // Visiting under a AstPackArrayDType
    ತಾರ್ಕಿಕ m_underMemberSel = ಸುಳ್ಳು;  // Viting under a MemberSel
    Ast_ಗಂಟು* m_seqUnsupp = ಶೂನ್ಯಸೂಚಕ;  // Property has unsupported node
    ತಾರ್ಕಿಕ m_hasSExpr = ಸುಳ್ಳು;  // Property has a sequence expression
    ಸ್ಥಿರಾಂಕ AstCell* m_cellp = ಶೂನ್ಯಸೂಚಕ;  // Current cell for arrayed instantiations
    std::ಸದಿಶ<AstVar*> m_ವಿಭಜಿಸು_Iface_Array_Varps;  // Interface arrays split by V3Param
    ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* m_enumItemp = ಶೂನ್ಯಸೂಚಕ;  // Current enum item
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    Ast_ಗಂಟು_Module* m_modep = ಶೂನ್ಯಸೂಚಕ;  // Current module
    ಸ್ಥಿರಾಂಕ AstConstraint* m_constraintp = ಶೂನ್ಯಸೂಚಕ;  // Current constraint
    Ast_ಗಂಟು_Procedure* m_procedurep = ಶೂನ್ಯಸೂಚಕ;  // Current final/always
    ಸ್ಥಿರಾಂಕ AstWith* m_withp = ಶೂನ್ಯಸೂಚಕ;  // Current 'with' statement
    ಸ್ಥಿರಾಂಕ AstFunc* m_funcp = ಶೂನ್ಯಸೂಚಕ;  // Current function
    ಸ್ಥಿರಾಂಕ Ast_Attr_ದ* m_attrp = ಶೂನ್ಯಸೂಚಕ;  // Current attribute
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_randomizeFromp = ಶೂನ್ಯಸೂಚಕ;  // Current randomize method call fromp
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_paramsOnly;  // Computing parameter value; limit operation
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_doGenerate;  // Do errors later inside generate statement
    ತಾರ್ಕಿಕ m_ಹರಿವು_Concat = ಸುಳ್ಳು;  // True if visiting arguments of stream concatenation
    ತಾರ್ಕಿಕ m_ಒಳ_Param_Override = ಸುಳ್ಳು;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*, VAttrType>, AstVarRef*> m_ಕೋಷ್ಟಕ_Map;
    // Queues with given index type
    std::map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*, Ast_ಸರತಿ_DType*> m_ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_ಗಣ<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> m_curModVars;  // Variables declared in current module

    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ ENUM_LOOKUP_ಬಿಟ್ಟುಗಳು = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    ಎಣಿಕೆ Extend_ನಿಯಮ : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    ಎಣಿಕೆ ವರ್ಗ ComparisonType : uint8_t {
        ಸಾಮಾನ್ಯ_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        ಸಾಮಾನ್ಯ_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    ಸ್ಥಿರ ಹರಿವು_Use ಹರಿವು_Use_For_Determ(Determ determ) {
        ಹಿಂತಿರುಗಿ determ == ನಿಯೋಜಿಸು_3 ? ಹರಿವು_USE_ನಿಯೋಜಿಸು : ಹರಿವು_USE_NONE;
    }

    ಹರಿವು_Use current_ಹರಿವು_Use() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vup ? m_vup->ಹರಿವು_Use_2() : ಹರಿವು_USE_NONE; }

    ಸ್ಥಿರ ಶೂನ್ಯ packIfUnpacked(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elements_ಅಂಕೆ = unpackDTypep->arrayUnpackedElements();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ unpack_Min_ಬಿಟ್ಟುಗಳು = elements_ಅಂಕೆ * unpackDTypep->subDTypep()->ಅಗಲ_Min();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ unpack_ಬಿಟ್ಟುಗಳು = elements_ಅಂಕೆ * unpackDTypep->subDTypep()->ಅಗಲ();
            VNRelinker relinker;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
            relinker.relink(ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Packed{
                nodep->fileline(), nodep,
                nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(unpack_ಬಿಟ್ಟುಗಳು, unpack_Min_ಬಿಟ್ಟುಗಳು, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* fromDTypep(Ast_ಗಂಟು* fromp) {
        ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(fromp, ಗಂಟು_DType))
            ಹಿಂತಿರುಗಿ dtypep->skipRefOrNullp();
        ಹಿಂತಿರುಗಿ fromp ? fromp->dtypep() : ಶೂನ್ಯಸೂಚಕ;
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
    ಶೂನ್ಯ ಭೇಟಿ(AstLogNot* nodep) override { ಭೇಟಿ_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    ಶೂನ್ಯ ಭೇಟಿ(AstLogAnd* nodep) override { ಭೇಟಿ_log_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogOr* nodep) override { ಭೇಟಿ_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    ಶೂನ್ಯ ಭೇಟಿ(AstSAnd* nodep) override { ಭೇಟಿ_log_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSIntersect* nodep) override { ಭೇಟಿ_log_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSOr* nodep) override { ಭೇಟಿ_log_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSWithin* nodep) override { ಭೇಟಿ_log_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        ಭೇಟಿ_log_and_or(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        ಭೇಟಿ_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    ಶೂನ್ಯ ಭೇಟಿ(AstRedAnd* nodep) override { ಭೇಟಿ_red_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstRedOr* nodep) override { ಭೇಟಿ_red_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstRedXor* nodep) override { ಭೇಟಿ_red_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstOneHot* nodep) override { ಭೇಟಿ_red_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstOneHot0* nodep) override { ಭೇಟಿ_red_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstIsUnknown* nodep) override {
        ಭೇಟಿ_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    ಶೂನ್ಯ ಭೇಟಿ(AstEq* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_EQ); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeq* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_EQ); }
    ಶೂನ್ಯ ಭೇಟಿ(AstGt* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_GL); }
    ಶೂನ್ಯ ಭೇಟಿ(AstGte* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_GL); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLt* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_GL); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLte* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_GL); }
    ಶೂನ್ಯ ಭೇಟಿ(AstGtS* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_GL); }
    ಶೂನ್ಯ ಭೇಟಿ(AstGteS* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_GL); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLtS* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_GL); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLteS* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::ಸಾಮಾನ್ಯ_GL); }
    ಶೂನ್ಯ ಭೇಟಿ(AstEqCase* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::CASE); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqCase* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    ಶೂನ್ಯ ಭೇಟಿ(AstEqWild* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::WILD); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqWild* nodep) override { ಭೇಟಿ_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    ಶೂನ್ಯ ಭೇಟಿ(AstEqD* nodep) override { ಭೇಟಿ_cmp_real(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqD* nodep) override { ಭೇಟಿ_cmp_real(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLtD* nodep) override { ಭೇಟಿ_cmp_real(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLteD* nodep) override { ಭೇಟಿ_cmp_real(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstGtD* nodep) override { ಭೇಟಿ_cmp_real(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstGteD* nodep) override { ಭೇಟಿ_cmp_real(nodep); }
    // ...    String compares
    ಶೂನ್ಯ ಭೇಟಿ(AstEqN* nodep) override { ಭೇಟಿ_cmp_string(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqN* nodep) override { ಭೇಟಿ_cmp_string(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLtN* nodep) override { ಭೇಟಿ_cmp_string(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLteN* nodep) override { ಭೇಟಿ_cmp_string(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstGtN* nodep) override { ಭೇಟಿ_cmp_string(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstGteN* nodep) override { ಭೇಟಿ_cmp_string(nodep); }
    // ...    Data type compares
    ಶೂನ್ಯ ಭೇಟಿ(AstEqT* nodep) override { ಭೇಟಿ_cmp_type(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqT* nodep) override { ಭೇಟಿ_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    ಶೂನ್ಯ ಭೇಟಿ(AstAnd* nodep) override { ಭೇಟಿ_boolexpr_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstOr* nodep) override { ಭೇಟಿ_boolexpr_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstXor* nodep) override { ಭೇಟಿ_boolexpr_and_or(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstBufIf1* nodep) override {
        ಭೇಟಿ_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಕೂಡಿಸು* nodep) override { ಭೇಟಿ_ಕೂಡಿಸು_sub_replace(nodep, ನಿಜ, ಸುಳ್ಳು); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSub* nodep) override { ಭೇಟಿ_ಕೂಡಿಸು_sub_replace(nodep, ನಿಜ, ಸುಳ್ಳು); }
    ಶೂನ್ಯ ಭೇಟಿ(AstDiv* nodep) override { ಭೇಟಿ_ಕೂಡಿಸು_sub_replace(nodep, ನಿಜ, ನಿಜ); }
    ಶೂನ್ಯ ಭೇಟಿ(AstMul* nodep) override { ಭೇಟಿ_ಕೂಡಿಸು_sub_replace(nodep, ನಿಜ, ಸುಳ್ಳು); }
    // These can't promote to real
    ಶೂನ್ಯ ಭೇಟಿ(AstModDiv* nodep) override { ಭೇಟಿ_ಕೂಡಿಸು_sub_replace(nodep, ಸುಳ್ಳು, ನಿಜ); }
    ಶೂನ್ಯ ಭೇಟಿ(AstModDivS* nodep) override { ಭೇಟಿ_ಕೂಡಿಸು_sub_replace(nodep, ಸುಳ್ಳು, ನಿಜ); }
    ಶೂನ್ಯ ಭೇಟಿ(AstMulS* nodep) override { ಭೇಟಿ_ಕೂಡಿಸು_sub_replace(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು); }
    ಶೂನ್ಯ ಭೇಟಿ(AstDivS* nodep) override { ಭೇಟಿ_ಕೂಡಿಸು_sub_replace(nodep, ಸುಳ್ಳು, ನಿಜ); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    ಶೂನ್ಯ ಭೇಟಿ(AstNegate* nodep) override { ಭೇಟಿ_negate_not(nodep, ನಿಜ); }
    // Unary never real
    ಶೂನ್ಯ ಭೇಟಿ(AstNot* nodep) override { ಭೇಟಿ_negate_not(nodep, ಸುಳ್ಳು); }

    // Real: inputs and output real
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಕೂಡಿಸು_D* nodep) override { ಭೇಟಿ_real_ಕೂಡಿಸು_sub(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSubD* nodep) override { ಭೇಟಿ_real_ಕೂಡಿಸು_sub(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstDivD* nodep) override { ಭೇಟಿ_real_ಕೂಡಿಸು_sub(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstMulD* nodep) override { ಭೇಟಿ_real_ಕೂಡಿಸು_sub(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPowD* nodep) override { ಭೇಟಿ_real_ಕೂಡಿಸು_sub(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_System_Biop_D* nodep) override { ಭೇಟಿ_real_ಕೂಡಿಸು_sub(nodep); }
    // Real: Output real
    ಶೂನ್ಯ ಭೇಟಿ(AstNegateD* nodep) override { ಭೇಟಿ_real_neg_ceil(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_System_Uniop_D* nodep) override { ಭೇಟಿ_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    ಶೂನ್ಯ ಭೇಟಿ(AstSigned* nodep) override { ಭೇಟಿ_signed_unsigned(nodep, VSigning::SIGNED); }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnsigned* nodep) override { ಭೇಟಿ_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_L* nodep) override { ಭೇಟಿ_ಸರಿಸು_2(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_R* nodep) override { ಭೇಟಿ_ಸರಿಸು_2(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_RS* nodep) override { ಭೇಟಿ_ಸರಿಸು_2(nodep); }

    //========
    // Widths: Output real, input integer signed
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Real_D* nodep) override { ಭೇಟಿ_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    ಶೂನ್ಯ ಭೇಟಿ(AstRToIS* nodep) override { ಭೇಟಿ_Os32_Lr(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->ಅಗಲ_Sized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Real_ಗಮ್ಯ_ಬಿಟ್ಟುಗಳು* nodep) override { ಭೇಟಿ_Ou64_Lr(nodep); }

    // Output integer, input string
    ಶೂನ್ಯ ಭೇಟಿ(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        ಆದರೆ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ಗಣ_Int();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ಆದರೆ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_ಗಣ_String();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ಆದರೆ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್_Sized(8, VSigning::UNSIGNED);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ಆದರೆ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್_Sized(8, VSigning::UNSIGNED);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ಆದರೆ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_ಗಣ_String();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ಗಣ_Int();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            ಆದರೆ (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtype_ಗಣ_Double();
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->dtype_ಗಣ_Integer();
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ* nodep) override { nodep->dtype_ಗಣ_Time2_State(); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_D* nodep) override { nodep->dtype_ಗಣ_Double(); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_Precision* nodep) override { nodep->dtype_ಗಣ_Integer2_State(); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Get_Initial_Random_ಬೀಜ* nodep) override { nodep->dtype_ಗಣ_Int(); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_Unit* nodep) override {
        nodep->replaceWith(
            ಹೊಸ AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().power_ದ_Ten()});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScopeName* nodep) override {
        nodep->dtype_ಗಣ_UInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Override_ನಮೂನೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ನಮೂನೆ* ಸ್ಥಿರಾಂಕ patternp = VN_CAST(nodep, ನಮೂನೆ)) {
            ಹಿಂತಿರುಗಿ !patternp->ಮಗು_DTypep();
        }
        ಸ್ಥಿರಾಂಕ AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(nodep, Cond);
        ಹಿಂತಿರುಗಿ condp && is_Override_ನಮೂನೆ(condp->thenp()) && is_Override_ನಮೂನೆ(condp->elsep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_ಒಳ_Param_Override) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ errors = V3Error::error_ಎಣಿಕೆ();
            {
                VL_RESTORER(m_ಒಳ_Param_Override);
                m_ಒಳ_Param_Override = ಸುಳ್ಳು;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->condp(), Const);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ತಿಳಿದ = constp && !constp->ಅಂಕೆ().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            ಆದರೆ (!ತಿಳಿದ && V3Error::error_ಎಣಿಕೆ() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ keepp
                = (ತಿಳಿದ && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(keepp, ಅಗಲ_VP{SELF, BOTH}.p());
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            user_Iterate_And_ಮುಂದಿನ(nodep->thenp(), ಅಗಲ_VP{m_vup->dtypeNullp(), PRELIM}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->elsep(), ಅಗಲ_VP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ thenDTypep = nodep->thenp()->dtypep();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elseDTypep = nodep->elsep()->dtypep();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elseDTypeSkippedp = elseDTypep->skipRefp();
            ಆದರೆ (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->ಕಂಡುಹಿಡಿ_Null_DType());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->thenp()->is_Class_Handle_ಮೌಲ್ಯ()
                       || nodep->elsep()->is_Class_Handle_ಮೌಲ್ಯ()) {
                Ast_ಗಂಟು_DType* commonClassTypep = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (nodep->thenp()->is_Class_Handle_ಮೌಲ್ಯ() && nodep->elsep()->is_Class_Handle_ಮೌಲ್ಯ()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_ಗಂಟು::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                ಆದರೆ (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                ಆದರೆ (Ast_ಗಂಟು::computeCastable(elseDTypep, thenDTypep, ಶೂನ್ಯಸೂಚಕ).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    ಆದರೆ (!Ast_ಗಂಟು::computeCastable(thenDTypep, elseDTypep, ಶೂನ್ಯಸೂಚಕ)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtype_ಗಣ_Double();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtype_ಗಣ_String();
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ is_Any_ಫಲಿತಾಂಶ_Fourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                ತಾರ್ಕಿಕ handled = ಸುಳ್ಳು;
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ thenEnumDTypep = thenDTypep->skip_Ref_ಗಮ್ಯ_Enump();
                ಆದರೆ (VN_IS(thenDTypeSkippedp, ಗಂಟು_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_ವೃಕ್ಷ(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_ವೃಕ್ಷ(elseDTypep->skip_Ref_ಗಮ್ಯ_Enump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    ಆದರೆ (is_Any_ಫಲಿತಾಂಶ_Fourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = ನಿಜ;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(ಹೊಸ File_ಸಾಲು{nodep->fileline()});
                        nodep->fileline()->warnOff(V3_Error_ಸಂಕೇತ::ENUMVALUE, ನಿಜ);
                    }
                }
                ಆದರೆ (!handled) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = std::max(thenDTypep->ಅಗಲ(), elseDTypep->ಅಗಲ());
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Min = std::max(thenDTypep->ಅಗಲ_Min(), elseDTypep->ಅಗಲ_Min());
                    ಸ್ಥಿರಾಂಕ VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isFourstate = is_Any_ಫಲಿತಾಂಶ_Fourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    ಆದರೆ ((ಅಗಲ == elseDTypeSkippedp->ಅಗಲ()
                         && ಅಗಲ_Min == elseDTypeSkippedp->ಅಗಲ_Min())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((ಅಗಲ == thenDTypeSkippedp->ಅಗಲ()
                                && ಅಗಲ_Min == thenDTypeSkippedp->ಅಗಲ_Min())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // None of the children dtypes match generate one
                        nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(ಅಗಲ, ಅಗಲ_Min, numeric, isFourstate);
                    }
                }
            }
        }
        ಆದರೆ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, ಅಗಲ_VP{SELF, BOTH}.p());
        // Conversions
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = new_Cons_Dyn_Array_Or_ಸರತಿ(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{vdtypep, PRELIM}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            ಕೂಡಿಸು_Cons_ಅಂಶ(newp, nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ());
            ಕೂಡಿಸು_Cons_ಅಂಶ(newp, nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (VN_IS(vdtypep, UnpackArrayDType)) {
            Ast_ನಮೂನೆ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ನಮೂನೆ{nodep->fileline(), ಶೂನ್ಯಸೂಚಕ};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ಹಿಂತಿರುಗಿ;
        }

        // Concat handling
        ಆದರೆ (m_vup->prelim()) {
            ಆದರೆ (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, ಸರತಿ_DType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            ಆದರೆ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtype_ಗಣ_String();
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                ಆದರೆ (m_ಹರಿವು_Concat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(
                    nodep->lhsp()->ಅಗಲ() + nodep->rhsp()->ಅಗಲ(),
                    nodep->lhsp()->ಅಗಲ_Min() + nodep->rhsp()->ಅಗಲ_Min(), VSigning::UNSIGNED,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstReplicate* ಸ್ಥಿರಾಂಕ repp = VN_CAST(nodep->lhsp(), Replicate)) {
                ಆದರೆ (repp->ಅಗಲ() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstReplicate* ಸ್ಥಿರಾಂಕ repp = VN_CAST(nodep->rhsp(), Replicate)) {
                ಆದರೆ (repp->ಅಗಲ() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
        ಆದರೆ (m_vup->final()) {
            ಆದರೆ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                ಆದರೆ (!lhsp->isString()) lhsp = ಹೊಸ AstCvtPackString{lhsp->fileline(), lhsp};
                ಆದರೆ (!rhsp->isString()) rhsp = ಹೊಸ AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (!nodep->dtypep()->ಅಗಲ_Sized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConcatN* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ಗಣ_String();
        }
        ಆದರೆ (m_vup->final()) {
            nodep->did_ಅಗಲ(ನಿಜ);
            ಆದರೆ (!nodep->dtypep()->ಅಗಲ_Sized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDelay* nodep) override {
        ಆದರೆ (nodep->is_ಚಕ್ರ_Delay() && m_underSExpr) {
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
            ಆದರೆ (nodep->rhsp() && !nodep->isUnbounded()) {
                user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            ಆದರೆ (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                ಹಿಂತಿರುಗಿ;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::check_Sva_ಚಕ್ರ_Delay(nodep, nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->fileline()->timingOn()) {
            ಆದರೆ (v3Global.opt.timing().is_ಗಣ_True()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.timing().is_ಗಣ_False()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        ಆದರೆ (nodep->stmtsp()) nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        ಆದರೆ (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().is_ಗಣ_True()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstBegin{nodep->fileline(), nodep->name(), ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
            nodep->replaceWith(newp);
            ಆದರೆ (nodep->declsp()) newp->ಕೂಡಿಸು_Declsp(nodep->declsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            ಆದರೆ (nodep->stmtsp()) newp->ಕೂಡಿಸು_Stmtsp(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            ಆದರೆ (nodep->forksp()) newp->ಕೂಡಿಸು_Stmtsp(nodep->forksp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.timing().is_ಗಣ_True()) {
            VL_RESTORER(m_underFork);
            m_underFork = ನಿಜ;
            iterateChildren(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.timing().is_ಗಣ_False()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_Wait_Or_Disable_Fork(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (nodep->fileline()->timingOn()) {
            ಆದರೆ (v3Global.opt.timing().is_ಗಣ_False()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!v3Global.opt.timing().is_ಗಣ_True()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisableFork* nodep) override { ಭೇಟಿ_Wait_Or_Disable_Fork(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstWaitFork* nodep) override { ಭೇಟಿ_Wait_Or_Disable_Fork(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಮ್ಯ_ಅಧೋ_N* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ಗಣ_String();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಮ್ಯ_ಊರ್ಧ್ವ_N* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ಗಣ_String();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->countp(), Const);
            ಆದರೆ (constp) {
                ಆದರೆ (constp->ಅಂಕೆ().isFourState()
                    || (constp->dtypep()->isSigned() && constp->ಅಂಕೆ().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    times = constp->ಗಮ್ಯ_SInt();
                }
            }

            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ vdtypep = m_vup->dtypeNullSkipRefp();
            ಆದರೆ (VN_IS(vdtypep, ಸರತಿ_DType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                ಆದರೆ (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                ಆದರೆ (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    user_Iterate_And_ಮುಂದಿನ(nodep->srcp(), ಅಗಲ_VP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlink_Fr_ಹಿಂಭಾಗ());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                } ಇಲ್ಲದಿದ್ದರೆ {  // int a[] = {lhs} -> same as '{lhs}
                    Ast_ನಮೂನೆ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ನಮೂನೆ{
                        nodep->fileline(),
                        ಹೊಸ AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                         ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            ಆದರೆ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            ಆದರೆ ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstReplicateN{nodep->fileline(), nodep->srcp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                        nodep->countp()->unlink_Fr_ಹಿಂಭಾಗ()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (!constp) nodep->v3error("Replication value isn't a constant.");
                ಆದರೆ (times == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    times = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(
                    (nodep->srcp()->ಅಗಲ() * times), (nodep->srcp()->ಅಗಲ_Min() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        ಆದರೆ (m_vup->final()) {
            ಆದರೆ (!nodep->dtypep()->ಅಗಲ_Sized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtype_ಗಣ_String();
        }
        ಆದರೆ (m_vup->final()) {
            ಆದರೆ (!nodep->dtypep()->ಅಗಲ_Sized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Dist_Biop* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Dist_Triop* nodep) override {
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    ತಾರ್ಕಿಕ ಹರಿವು_Implicit_Use_Allowed(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಹರಿವು* nodep) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_ಹರಿವು_Concat) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (m_vup && m_vup->ಹರಿವು_Use_2() != ಹರಿವು_USE_NONE) ಹಿಂತಿರುಗಿ ನಿಜ;

        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = nodep->backp();
        ಆದರೆ (!backp) ಹಿಂತಿರುಗಿ ನಿಜ;  // Error elsewhere

        ಹಿಂತಿರುಗಿ VN_IS(backp, ಗಂಟು_ನಿಯೋಜಿಸು);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಹರಿವು* nodep) override {
        VL_RESTORER(m_ಹರಿವು_Concat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            m_ಹರಿವು_Concat = ನಿಜ;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_ಹರಿವು_Concat = ಸುಳ್ಳು;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->rhsp(), Const)) {
                ಆದರೆ (constp->ಗಮ್ಯ_UInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDtypep = nodep->lhsp()->dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
            ಆದರೆ (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, ಸರತಿ_DType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, ಗಂಟು_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, ಗಂಟು_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtype_ಗಣ_ಹರಿವು();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtype_ಗಣ_ಹರಿವು();
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(
                    nodep->lhsp()->ಅಗಲ(), nodep->lhsp()->ಅಗಲ_Min(), VSigning::UNSIGNED,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ಆದರೆ (m_vup->final()) {
            ಆದರೆ (!ಹರಿವು_Implicit_Use_Allowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warn_ಇನ್ನಷ್ಟು() << "... Suggest use a cast");
            }
            ಆದರೆ (!nodep->dtypep()->ಅಗಲ_Sized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), ನಿಜ,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), ನಿಜ,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        ಆದರೆ (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->elementsConst();
            ಆದರೆ (nodep->fromBracket() && nodep->ಎಡ_Const() > nodep->ಬಲ_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-ಅಗಲ + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಅಗಲ > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << ಅಗಲ << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Dead_Module = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Type_ಕೋಷ್ಟಕ = !m_modep;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* basep = nodep->backp();
            ಆಗ (VN_IS(basep, Range)) basep = basep->backp();
            ಆದರೆ (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !ಒಳ_Dead_Module && !ಒಳ_Parameterized_Template && !ಒಳ_Type_ಕೋಷ್ಟಕ) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->ಎಡ_Const() << ":" << nodep->ಬಲ_Const()
                                            << "]");
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->lsbp(), ಅಗಲ_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), ಸುಳ್ಳು);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->ಅಗಲ_Const();
            ಆದರೆ (ಅಗಲ <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtype_ಗಣ_ಬಿಟ್();
                ಹಿಂತಿರುಗಿ;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            ಆದರೆ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                ಅಗಲ = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Sized(ಅಗಲ, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->ಅಗಲ_Const(ಅಗಲ);
                ತಳ್ಳು_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(ಹೊಸ AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            ಆದರೆ (nodep->fromp()->ಅಗಲ() < ಅಗಲ && !ಒಳ_Parameterized_Template) {
                // If it is not a lvalue, extend it
                ಆದರೆ (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << ಅಗಲ << " bits from only "
                                                          << nodep->fromp()->ಅಗಲ()
                                                          << " bit number");
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                        ಅಗಲ, ಅಗಲ, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    ಅಗಲ_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ಸುಳ್ಳು /*noerror*/);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << ಅಗಲ << " bits to only "
                                                         << nodep->fromp()->ಅಗಲ()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            ಪೂರ್ಣಾಂಕ frommsb = nodep->fromp()->ಅಗಲ() - 1;
            ಪೂರ್ಣಾಂಕ fromlsb = 0;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elw = nodep->decl_El_ಅಗಲ();  // Must adjust to tell user bit ranges
            ಆದರೆ (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ selwidth = V3_ಸಂಖ್ಯೆ::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ selwidthDTypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, FINAL}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->lsbp(), ಅಗಲ_VP{SELF, FINAL}.p());
            ಆದರೆ (ಅಗಲ_Bad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->ಅಗಲ() != 32) {
                ಆದರೆ (!nodep->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::ಅಗಲ_2)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->ಅಗಲ() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->ಅಗಲ() / elw)
                            << (nodep->lsbp()->ಅಗಲ() != nodep->lsbp()->ಅಗಲ_Min()
                                    ? " or " + cvt_ಗಮ್ಯ_Str(nodep->lsbp()->ಅಗಲ_Min() / elw)
                                    : "")
                            << " bits.");
                    UINFO(1, "    Related node: " << nodep);
                }
            }
            ಆದರೆ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                ಆದರೆ (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಒಳ_Parameterized_Template) {
                    ಆದರೆ (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                ಆದರೆ (!isWriteSelect) {
                    // Extend it
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ extend_ಗಮ್ಯ = nodep->msbConst() + 1;
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                        extend_ಗಮ್ಯ, extend_ಗಮ್ಯ, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    ಅಗಲ_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ಸುಳ್ಳು /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            ಆದರೆ (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                ಅಗಲ_Check_Sized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                ಸುಳ್ಳು /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            ಆದರೆ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->bitp(), Const)) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = constp->ಗಮ್ಯ_SInt();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = newIfaceArrayElement(nodep->fromp(), ಬಿಟ್_2);
                    ಆದರೆ (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << ಬಿಟ್_2 + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            //
            ಪೂರ್ಣಾಂಕ frommsb;
            ಪೂರ್ಣಾಂಕ fromlsb;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                ಆದರೆ (fromlsb > frommsb) std::ಅದಲುಬದಲು(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                ಆದರೆ (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ selwidth = V3_ಸಂಖ್ಯೆ::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ selwidthDTypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            ಆದರೆ (ಅಗಲ_Bad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->ಅಗಲ() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->ಅಗಲ(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->ಅಗಲ()
                                       << (nodep->bitp()->ಅಗಲ() != nodep->bitp()->ಅಗಲ_Min()
                                               ? " or " + cvt_ಗಮ್ಯ_Str(nodep->bitp()->ಅಗಲ_Min())
                                               : "")
                                       << " bits.");
                ಆದರೆ (!nodep->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::ಅಗಲ_2)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            ಆದರೆ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ಆದರೆ (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->ಗಮ್ಯ_SInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->ಗಮ್ಯ_SInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    ಆದರೆ (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->ಗಮ್ಯ_SInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                ಅಗಲ_Check_Sized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                ಸುಳ್ಳು /*NOWARN*/);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ಸ್ಥಿರಾಂಕ AstAssocArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, AssocArrayDType);
            ಆದರೆ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->ಕೀಲಿ_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstAlias* nodep) override {
        ಆದರೆ (!nodep->did_ಅಗಲ_And_ಗಣ()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->itemsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        }

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ check_If_ಅಭಿವ್ಯಕ್ತಿ_Ok = [ಇದು](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp) {
            ಆದರೆ (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }

            ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(exprp, VarRef);
            ಆದರೆ (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            AstVar* ಸ್ಥಿರಾಂಕ varp = varRefp->varp();
            ಆದರೆ (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಆದರೆ (m_aliasedVars.ಕಂಡುಹಿಡಿ(varp) != m_aliasedVars.ಕೊನೆ()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_aliasedVars.ಸೇರಿಸು(varp);
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        };

        check_If_ಅಭಿವ್ಯಕ್ತಿ_Ok(nodep->itemsp());
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಮೊದಲ_ಅಂಶ_Dtypep = nodep->itemsp()->dtypep();
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_ಅಭಿವ್ಯಕ್ತಿ_Ok(VN_AS(itemp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            ಆದರೆ (!ಮೊದಲ_ಅಂಶ_Dtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << ಮೊದಲ_ಅಂಶ_Dtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ಸ್ಥಿರಾಂಕ AstWildcardArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            ಆದರೆ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            ಆದರೆ (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->ಕಂಡುಹಿಡಿ_String_DType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ತುಂಡು_Sel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            ಆದರೆ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ newDtp
                = ಹೊಸ AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          ಹೊಸ AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(newDtp);
            nodep->dtypeFrom(newDtp);

            ಆದರೆ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ಸ್ಥಿರಾಂಕ VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                ಆದರೆ ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(ಹೊಸ AstConsArray{
                        nodep->fileline(), newDtp,
                        ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}}});
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            ಆದರೆ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConsArray{nodep->fileline(), newDtp};
                ಸ್ಥಿರಾಂಕ VNumRange range = nodep->declRange();
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sel_ಬಿಟ್* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->bitp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->thsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->attrp(), ಅಗಲ_VP{SELF, BOTH}.p());
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ selp = V3_ಅಗಲ::ಅಗಲ_Sel_No_Iter_Edit(nodep);
        ಆದರೆ (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            ಹಿಂತಿರುಗಿ;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sel_ಹೊರತೆಗೆ* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->leftp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->rightp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->attrp(), ಅಗಲ_VP{SELF, BOTH}.p());
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ selp = V3_ಅಗಲ::ಅಗಲ_Sel_No_Iter_Edit(nodep);
        ಆದರೆ (selp != nodep) {
            nodep = ಶೂನ್ಯಸೂಚಕ;
            userIterate(selp, m_vup);
            ಹಿಂತಿರುಗಿ;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSelPlus* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->bitp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->widthp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->attrp(), ಅಗಲ_VP{SELF, BOTH}.p());
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ selp = V3_ಅಗಲ::ಅಗಲ_Sel_No_Iter_Edit(nodep);
        ಆದರೆ (selp != nodep) {
            nodep = ಶೂನ್ಯಸೂಚಕ;
            userIterate(selp, m_vup);
            ಹಿಂತಿರುಗಿ;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSelMinus* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->bitp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->widthp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ಮುಂದಿನ(nodep->attrp(), ಅಗಲ_VP{SELF, BOTH}.p());
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ selp = V3_ಅಗಲ::ಅಗಲ_Sel_No_Iter_Edit(nodep);
        ಆದರೆ (selp != nodep) {
            nodep = ಶೂನ್ಯಸೂಚಕ;
            userIterate(selp, m_vup);
            ಹಿಂತಿರುಗಿ;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_vup && m_vup->prelim()) {
            ಆದರೆ (VN_IS(nodep->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಅಂಕೆ().isNull()) {
                nodep->dtypep(nodep->ಕಂಡುಹಿಡಿ_Null_DType());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಅಂಕೆ().isString()) {
                nodep->dtype_ಗಣ_String();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಅಂಕೆ().isDouble()) {
                nodep->dtype_Chg_ಅಗಲ(nodep->ಅಂಕೆ().ಅಗಲ(), nodep->ಅಂಕೆ().sized()
                                                               ? nodep->ಅಂಕೆ().ಅಗಲ()
                                                               : nodep->ಅಂಕೆ().ಅಗಲ_ಗಮ್ಯ_Fit());
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(
                    nodep->ಅಂಕೆ().ಅಗಲ(),
                    nodep->ಅಂಕೆ().sized() ? nodep->ಅಂಕೆ().ಅಗಲ() : nodep->ಅಂಕೆ().ಅಗಲ_ಗಮ್ಯ_Fit(),
                    nodep->dtypep()->numeric(), nodep->ಅಂಕೆ().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಖಾಲಿಗಳು_ಸರತಿ* nodep) override {
        nodep->dtype_ಗಣ_ಖಾಲಿಗಳು_ಸರತಿ();
        ಆದರೆ (!VN_IS(nodep->backp(), ನಿಯೋಜಿಸು) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFell* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ನಿಜ);
            userIterate(nodep->sentreep(), ಶೂನ್ಯಸೂಚಕ);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFalling* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ನಿಜ);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFuture* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ನಿಜ);
            userIterate(nodep->sentreep(), ಶೂನ್ಯಸೂಚಕ);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPast* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ನಿಜ);
            nodep->dtypeFrom(nodep->exprp());
            ಆದರೆ (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ ನಿಜ);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->ticksp(), Const);
                ಆದರೆ (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp->ಗಮ್ಯ_SInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (V3AssertCommon::check_Sva_ಮೌಲ್ಯ_ಎಲ್ಲೆ(nodep, constp, "$past tick value")) {
                    ಆದರೆ (constp->ಗಮ್ಯ_SInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->ಗಮ್ಯ_SInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), ಶೂನ್ಯಸೂಚಕ);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{SELF, BOTH}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->countp(), ಅಗಲ_VP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ minConstp = VN_CAST(nodep->countp(), Const);
            ಆದರೆ (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (minConstp->ಗಮ್ಯ_SInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->unbounded() && !nodep->maxCountp() && minConstp->ಗಮ್ಯ_SInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            ಆದರೆ (nodep->maxCountp()) {
                user_Iterate_And_ಮುಂದಿನ(nodep->maxCountp(), ಅಗಲ_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ maxConstp = VN_CAST(nodep->maxCountp(), Const);
                ಆದರೆ (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (minConstp && maxConstp->ಗಮ್ಯ_SInt() < minConstp->ಗಮ್ಯ_SInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (maxConstp->ಗಮ್ಯ_SInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->propp(), ಅಗಲ_VP{SELF, BOTH}.p());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp = nodep->propp();
            ಆದರೆ (!VN_IS(nodep->loBoundp(), Unbounded)) {
                user_Iterate_And_ಮುಂದಿನ(nodep->loBoundp(), ಅಗಲ_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            ಆದರೆ (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                user_Iterate_And_ಮುಂದಿನ(nodep->hiBoundp(), ಅಗಲ_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            ಆದರೆ (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtype_ಗಣ_ಬಿಟ್();
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtype_ಗಣ_ಬಿಟ್();
                ಹಿಂತಿರುಗಿ;
            }
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ loConstp = VN_CAST(nodep->loBoundp(), Const);
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            ಆದರೆ (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ಆದರೆ (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ಆದರೆ (loConstp && loConstp->ಗಮ್ಯ_SInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (loConstp) {
                V3AssertCommon::check_Sva_ಮೌಲ್ಯ_ಎಲ್ಲೆ(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            ಆದರೆ (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_ಮೌಲ್ಯ_ಎಲ್ಲೆ(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            ಆದರೆ (!hiUnbounded && loConstp && hiConstp && hiConstp->ಗಮ್ಯ_SInt() < loConstp->ಗಮ್ಯ_SInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ತಾರ್ಕಿಕ hasPropertyOp = propp->is_Multi_ಚಕ್ರ_Sva();
            ಆದರೆ (!hasPropertyOp) {
                propp->foreach([&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* np) {
                    ಆದರೆ (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = ನಿಜ;
                    }
                });
            }
            ಆದರೆ (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fix_ಅಗಲ_Reduce(propp), propp);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRising* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ನಿಜ);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRose* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ನಿಜ);
            userIterate(nodep->sentreep(), ಶೂನ್ಯಸೂಚಕ);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSampled* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ನಿಜ);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSetuphold* nodep) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ನಿಯೋಜಿಸು_W* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->delrefp()) {
            newp = convert_Setuphold_ಗಮ್ಯ_ನಿಯೋಜಿಸು(flp, nodep->refevp(), nodep->delrefp());
        }
        ಆದರೆ (nodep->deldatap()) {
            ಆದರೆ (!newp) {
                newp = convert_Setuphold_ಗಮ್ಯ_ನಿಯೋಜಿಸು(flp, nodep->dataevp(), nodep->deldatap());
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(
                    convert_Setuphold_ಗಮ್ಯ_ನಿಯೋಜಿಸು(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        ಆದರೆ (!newp) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        nodep->replaceWith(ಹೊಸ AstAlways{newp});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    Ast_ನಿಯೋಜಿಸು_W* convert_Setuphold_ಗಮ್ಯ_ನಿಯೋಜಿಸು(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ evp,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ delp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = delp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = evp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        UASSERT_OBJ(VN_IS(lhsp, ಗಂಟು_Var_Ref) || VN_IS(lhsp, ಗಂಟು_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        ಆದರೆ (Ast_ಗಂಟು_Var_Ref* varRefp = VN_CAST(lhsp, ಗಂಟು_Var_Ref)) {
            ಆದರೆ (varRefp->varp()->direction() == VDirection::ಒಳಹರಿವು) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
            varRefp->access(VAccess::WRITE);
        }
        ಆದರೆ (Ast_ಗಂಟು_Pre_Sel* selp = VN_CAST(lhsp, ಗಂಟು_Pre_Sel)) {
            ಆದರೆ (Ast_ಗಂಟು_Var_Ref* varRefp = VN_CAST(selp->fromp(), ಗಂಟು_Var_Ref)) {
                ಆದರೆ (varRefp->varp()->direction() == VDirection::ಒಳಹರಿವು) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
                    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{flp, lhsp, rhsp};
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstStable* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ನಿಜ);
            userIterate(nodep->sentreep(), ಶೂನ್ಯಸೂಚಕ);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSteady* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ನಿಜ);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstUntil* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            ಆದರೆ (nodep->lhsp()->ಅಗಲ() != 1) fix_ಅಗಲ_Reduce(nodep->lhsp());
            ಆದರೆ (nodep->rhsp()->ಅಗಲ() != 1) fix_ಅಗಲ_Reduce(nodep->rhsp());
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAbortOn* nodep) override { ಭೇಟಿ_Abort_Prop(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(AstRand* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            ಆದರೆ (nodep->urandom()) {
                nodep->dtype_ಗಣ_UInt32();  // Says the spec
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->dtype_ಗಣ_Int();  // Says the spec
            }
            ಆದರೆ (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSEventually* nodep) override {
        ಆದರೆ (v3Global.opt.timing().is_ಗಣ_False() || !v3Global.opt.timing().is_ಗಣ_True()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSGotoRep* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_ಮುಂದಿನ(nodep->countp(), ಅಗಲ_VP{SELF, BOTH}.p());
            ಆದರೆ (nodep->maxCountp()) {
                ಅಗಲ_Check_Goto_Rep_Range(nodep, "Goto");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಅಗಲ_Check_Rep_ಎಣಿಕೆ(nodep);
            }
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSNonConsRep* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_ಮುಂದಿನ(nodep->countp(), ಅಗಲ_VP{SELF, BOTH}.p());
            ಆದರೆ (nodep->maxCountp()) {
                ಅಗಲ_Check_Goto_Rep_Range(nodep, "Nonconsecutive");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಅಗಲ_Check_Rep_ಎಣಿಕೆ(nodep);
            }
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Rep>
    ಶೂನ್ಯ ಅಗಲ_Check_Rep_ಎಣಿಕೆ(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->countp(), Const);
        ಆದರೆ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp->dtypep()->isSigned() && constp->ಅಂಕೆ().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp->ಅಂಕೆ().most_ಗಣ_ಬಿಟ್_P1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Rep>
    ಶೂನ್ಯ ಅಗಲ_Check_Goto_Rep_Range(T_Rep* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಬಗೆ) {
        user_Iterate_And_ಮುಂದಿನ(nodep->maxCountp(), ಅಗಲ_VP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ minConstp = VN_CAST(nodep->countp(), Const);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ maxConstp = VN_CAST(nodep->maxCountp(), Const);
        ಆದರೆ (!minConstp || !maxConstp) {
            nodep->v3error(std::string{ಬಗೆ}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (minConstp->ಗಮ್ಯ_SInt() < 0) {
            nodep->v3error(std::string{ಬಗೆ}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (maxConstp->ಗಮ್ಯ_SInt() < minConstp->ಗಮ್ಯ_SInt()) {
            nodep->v3error(std::string{ಬಗೆ}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + ಬಗೆ
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSThroughout* nodep) override {
        m_hasSExpr = ನಿಜ;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = ಸುಳ್ಳು;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = ನಿಜ;
            iterate(nodep->rhsp());
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ನಿಜ;
        m_hasSExpr = ನಿಜ;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            ಆದರೆ (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ನಿಜ;
        m_hasSExpr = ನಿಜ;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            ಗಾಗಿ (Ast_Sen_ಅಂಶ* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), Sen_ಅಂಶ)) {
                user_Iterate_And_ಮುಂದಿನ(senp->sensp(), ಅಗಲ_VP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstURandomRange* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            nodep->dtype_ಗಣ_UInt32();  // Says the spec
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = nodep->ಕಂಡುಹಿಡಿ_UInt32_DType();
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnbounded* nodep) override {
        nodep->dtype_ಗಣ_Int();  // Used in int context
        ಆದರೆ (VN_IS(nodep->backp(), IsUnbounded)) ಹಿಂತಿರುಗಿ;  // Ok, leave
        ಆದರೆ (VN_IS(nodep->backp(), BracketArrayDType)) ಹಿಂತಿರುಗಿ;  // Ok, leave
        ಆದರೆ (VN_IS(nodep->backp(), InsideRange)) ಹಿಂತಿರುಗಿ;  // Ok, leave
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep->backp(), Var)) {
            ಆದರೆ (varp->isParam()) ಹಿಂತಿರುಗಿ;  // Ok, leave
        }
        Ast_ಗಂಟು* backp = nodep->backp();
        ಆದರೆ (VN_IS(backp, Sub)) backp = backp->backp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ selp = VN_CAST(backp, Sel_ಹೊರತೆಗೆ)) {
            ಆದರೆ (VN_IS(selp->fromp()->dtypep()->skipRefp(), ಸರತಿ_DType)) ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ selp = VN_CAST(backp, Sel_ಬಿಟ್)) {
            ಆದರೆ (VN_IS(selp->fromp()->dtypep()->skipRefp(), ಸರತಿ_DType)) ಹಿಂತಿರುಗಿ;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInferredDisable* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) nodep->dtype_ಗಣ_ಬಿಟ್();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIsUnbounded* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        ಆದರೆ (m_vup && m_vup->prelim()) {
            nodep->dtype_ಗಣ_ಬಿಟ್_Unsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
        }
        ಆದರೆ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            ಆದರೆ (nodep->ಅಗಲ() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCLog2* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Option>
    ಶೂನ್ಯ ಅಗಲ_Cover_ತೂಕ(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = nodep->valuep();
        ಸ್ಥಿರಾಂಕ std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const)) {
            ಆದರೆ (!constp->ಅಂಕೆ().isNegative()) ಹಿಂತಿರುಗಿ;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->ಗಮ್ಯ_SInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ;
        }
        // Continue with the default weight
        valuep->replaceWith(ಹೊಸ AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Option>
    ಶೂನ್ಯ ಅಗಲ_Cover_Int(T_Option* nodep) {
        user_Iterate_And_ಮುಂದಿನ(nodep->valuep(), ಅಗಲ_VP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cg_Option_ನಿಯೋಜಿಸು* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ಸ್ಥಿರಾಂಕ cgClassp = m_containingClassFinder.ಕಂಡುಹಿಡಿ(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        ಆದರೆ (nodep->optType() == VCoverOptionType::ತೂಕ) {
            ಅಗಲ_Cover_ತೂಕ(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        ಆದರೆ (nodep->optType() == VCoverOptionType::AT_ಕನಿಷ್ಠ) {
            ಅಗಲ_Cover_Int(nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        ಆದರೆ (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            ಆದರೆ (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->ಗಮ್ಯ_SInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->ಗಮ್ಯ_SInt() << ಸಾಲು_ಅಂತ್ಯ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->optType() == VCoverOptionType::ವಿಲೀನ_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = nodep->valuep();
            ಆದರೆ (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(ಹೊಸ AstConst{valuep->fileline(), AstConst::ಬಿಟ್_False{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            }
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->optType() == VCoverOptionType::DISTRIBUTE_ಮೊದಲ) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->valuep(), Const);
            ಆದರೆ (!constp || !constp->ಅಂಕೆ().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverOption* nodep) override {
        ಆದರೆ (nodep->optType() == VCoverOptionType::ತೂಕ) {
            ಅಗಲ_Cover_ತೂಕ(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->optType() == VCoverOptionType::AT_ಕನಿಷ್ಠ
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            ಅಗಲ_Cover_Int(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        user_Iterate_And_ಮುಂದಿನ(nodep->valuep(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCross* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->itemsp(), ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_ಮುಂದಿನ(nodep->optionsp(), ಶೂನ್ಯಸೂಚಕ);
        user_Iterate_And_ಮುಂದಿನ(nodep->binsp(), ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossBin* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->selectp(), ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        user_Iterate_And_ಮುಂದಿನ(nodep->binsp(), ಅಗಲ_VP{nodep->exprp()->dtypep(), BOTH}.p());
        ಆದರೆ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_ಮುಂದಿನ(nodep->optionsp(), ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಅಗಲ_Covergroup_Ranges(Ast_ಗಂಟು* rangesp, ಪೂರ್ಣಾಂಕ fill_ಅಗಲ) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ fill = [&](Ast_ಗಂಟು* nodep) {
            ಆದರೆ (!fill_ಅಗಲ) ಹಿಂತಿರುಗಿ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = VN_AS(nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            fixAutoExtend(exprp /*ref*/, fill_ಅಗಲ);
        };
        ಗಾಗಿ (Ast_ಗಂಟು *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಆದರೆ (AstInsideRange* ಸ್ಥಿರಾಂಕ rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, ಶೂನ್ಯಸೂಚಕ);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } ಇಲ್ಲದಿದ್ದರೆ {
                itemp = userIterateSubtreeReturnEdits(itemp, ಅಗಲ_VP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverBinsof* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->pointp(), ಶೂನ್ಯಸೂಚಕ);
        ಅಗಲ_Covergroup_Ranges(nodep->rangesp(), 0);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        ಅಗಲ_Covergroup_Ranges(nodep->rangesp(), m_vup ? m_vup->dtypep()->ಅಗಲ() : 0);
        ಆದರೆ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        ಆದರೆ (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            user_Iterate_And_ಮುಂದಿನ(nodep->arraySizep(), ಅಗಲ_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        user_Iterate_And_ಮುಂದಿನ(nodep->transp(), m_vup);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        user_Iterate_And_ಮುಂದಿನ(nodep->itemp(), ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        ಆದರೆ (!VN_IS(nodep->subp(), CoverpointRef)) {
            ಅಗಲ_Covergroup_Ranges(nodep->subp(), m_vup->dtypep()->ಅಗಲ());
        }
        ಅಗಲ_Cover_Filter(nodep->filterp(), "Bin");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossWith* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->selectp(), ಶೂನ್ಯಸೂಚಕ);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        user_Iterate_And_ಮುಂದಿನ(nodep->varsp(), ಶೂನ್ಯಸೂಚಕ);
        ಅಗಲ_Cover_Filter(nodep->filterp(), "Cross bin");
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ countp = nodep->countp()) {
            ಆದರೆ (VN_IS(countp, Unbounded)) {
                countp->dtype_ಗಣ_Int();  // 'matches $'
            } ಇಲ್ಲದಿದ್ದರೆ {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                user_Iterate_And_ಮುಂದಿನ(countp, ಅಗಲ_VP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    ಶೂನ್ಯ ಅಗಲ_Cover_Filter(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filterp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, ಅಗಲ_VP{SELF, BOTH}.p()), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = filterp->fileline();
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ resultp = filterp->dtypep()->skipRefp();
        ಆದರೆ (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
            relinker.relink(
                ಹೊಸ AstNeqD{fl, filterp, ಹೊಸ AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(filterp), filterp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            fix_ಅಗಲ_Reduce(filterp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cover_Trans_ಗಣ* nodep) override { user_Iterate_And_ಮುಂದಿನ(nodep->itemsp(), m_vup); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cover_Trans_ಅಂಶ* nodep) override {
        ಅಗಲ_Covergroup_Ranges(nodep->valuesp(), m_vup ? m_vup->dtypep()->ಅಗಲ() : 0);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            ಆದರೆ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            checkCvtUS(nodep->lhsp(), ಸುಳ್ಳು);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ಆದರೆ (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtype_ಗಣ_Logic_Unsized(nodep->ಅಗಲ(), nodep->ಅಗಲ_Min(),
                                            nodep->dtypep()->numeric());
            }
        }

        ಆದರೆ (m_vup->final()) {
            Ast_ಗಂಟು_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(expDTypep->ಅಗಲ(), expDTypep->ಅಗಲ_Min(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_ಗಂಟು* newp = ಶೂನ್ಯಸೂಚಕ;  // No change
            ಆದರೆ (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = ಹೊಸ AstPowSS{nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                    nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = ಹೊಸ AstPowSU{nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                    nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = ಹೊಸ AstPowUS{nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                    nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()};
            }
            ಆದರೆ (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Min = V3_ಸಂಖ್ಯೆ::log2b(nodep->lhsp()->ಅಗಲ()) + 1;
            nodep->dtype_ಗಣ_ಬಿಟ್_Unsized(32, ಅಗಲ_Min, VSigning::SIGNED);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಎಣಿಕೆ_Ones* nodep) override {
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Min = V3_ಸಂಖ್ಯೆ::log2b(nodep->lhsp()->ಅಗಲ()) + 1;
            nodep->dtype_ಗಣ_ಬಿಟ್_Unsized(32, ಅಗಲ_Min, VSigning::SIGNED);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCvtPackString* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH, current_ಹರಿವು_Use()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dt = nodep->lhsp()->dtypep()) {
            ಆದರೆ (VN_IS(dt->skip_Ref_ಗಮ್ಯ_Enump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Packed_ಗಮ್ಯ_Array* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, BOTH, current_ಹರಿವು_Use()}.p());
        // Type set in constructor
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Array_ಗಮ್ಯ_Array* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, BOTH, current_ಹರಿವು_Use()}.p());
        // Type set in constructor
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Array_ಗಮ್ಯ_Packed* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, BOTH, current_ಹರಿವು_Use()}.p());
        // Type set in constructor
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Unpack_ಗಮ್ಯ_ಸರತಿ* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, BOTH}.p());
        // Type set in constructor
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack* nodep) override {
        // Type set when created, the value converted is of its own type
        ಆದರೆ (m_vup->final()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_Import* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಸಮಯ_Prescale = constp->ಅಂಕೆ().ಗಮ್ಯ_Double();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಸಮಯ_2 = ಸಮಯ_Prescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::RealDouble{}, ಸಮಯ_2};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEventControl* nodep) override {
        ಆದರೆ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->fileline()->timingOn()) {
            ಆದರೆ (v3Global.opt.timing().is_ಗಣ_True()) {
                iterateChildren(nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.timing().is_ಗಣ_False()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warn_ಇನ್ನಷ್ಟು()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        ಆದರೆ (nodep->stmtsp()) nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    ಸ್ಥಿರ string typenameExtends(ಸ್ಥಿರಾಂಕ AstClass* classp) {
        // An interface class may extend several, so those are not named
        ಆದರೆ (!classp || classp->isInterfaceClass()) ಹಿಂತಿರುಗಿ "";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ಆದರೆ (extendsp->isImplements()) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ basep = extendsp->classOrNullp();
            ಹಿಂತಿರುಗಿ basep ? " extends class " + basep->dtypeName(ನಿಜ) : "";
        }
        ಹಿಂತಿರುಗಿ "";
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Attr_ದ* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, BOTH}.p());
        ಆದರೆ (nodep->dimp()) user_Iterate_And_ಮುಂದಿನ(nodep->dimp(), ಅಗಲ_VP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        ಆಯ್ಕೆ (nodep->attrType()) {
        ಸ್ಥಿತಿ VAttrType::DIM_DIMENSIONS:
        ಸ್ಥಿತಿ VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ಸ್ಥಿರಾಂಕ std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(ನಿಜ);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.ಎರಡನೇ
                                                                       : (dim.ಮೊದಲ + dim.ಎರಡನೇ));
            nodep->replaceWith(ಹೊಸ AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ VAttrType::DIM_ಬಿಟ್ಟುಗಳು_OR_ಸಂಖ್ಯೆ: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            ಆದರೆ (VN_IS(nodep->fromp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                nodep->replaceWith(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ());
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                Ast_ಗಂಟು* newp = ಹೊಸ Ast_Attr_ದ{nodep->fileline(), VAttrType::DIM_ಬಿಟ್ಟುಗಳು,
                                              nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                user_Iterate_And_ಮುಂದಿನ(newp, ಅಗಲ_VP{SELF, BOTH}.p());  // Convert AttrOf
            }
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿತಿ VAttrType::DIM_ಬಿಟ್ಟುಗಳು:
        ಸ್ಥಿತಿ VAttrType::DIM_ಮೇಲಿನ:
        ಸ್ಥಿತಿ VAttrType::DIM_INCREMENT:
        ಸ್ಥಿತಿ VAttrType::DIM_ಎಡ:
        ಸ್ಥಿತಿ VAttrType::DIM_ಕೆಳಗಿನ:
        ಸ್ಥಿತಿ VAttrType::DIM_ಬಲ:
        ಸ್ಥಿತಿ VAttrType::DIM_ಗಾತ್ರ: {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            ಆದರೆ (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownModp = v3Global.rootp()->containingModule(dtypep);
                ಆದರೆ (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtype_ಗಣ_Int();
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (VN_IS(dtypep, ಸರತಿ_DType) || VN_IS(dtypep, DynArrayDType)) {
                ಆಯ್ಕೆ (nodep->attrType()) {
                ಸ್ಥಿತಿ VAttrType::DIM_ಗಾತ್ರ: {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_ಗಾತ್ರ};
                    newp->dtype_ಗಣ_Int();
                    newp->did_ಅಗಲ(ನಿಜ);
                    newp->protect(ಸುಳ್ಳು);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ನಿಲ್ಲಿಸು;
                }
                ಸ್ಥಿತಿ VAttrType::DIM_ಎಡ:
                ಸ್ಥಿತಿ VAttrType::DIM_ಕೆಳಗಿನ: {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ನಿಲ್ಲಿಸು;
                }
                ಸ್ಥಿತಿ VAttrType::DIM_ಬಲ:
                ಸ್ಥಿತಿ VAttrType::DIM_ಮೇಲಿನ: {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sizep
                        = ಹೊಸ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_ಗಾತ್ರ};
                    sizep->dtype_ಗಣ_Int();
                    sizep->did_ಅಗಲ(ನಿಜ);
                    sizep->protect(ಸುಳ್ಳು);
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstSub{nodep->fileline(), sizep,
                                     ಹೊಸ AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ನಿಲ್ಲಿಸು;
                }
                ಸ್ಥಿತಿ VAttrType::DIM_INCREMENT: {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ನಿಲ್ಲಿಸು;
                }
                ಸ್ಥಿತಿ VAttrType::DIM_ಬಿಟ್ಟುಗಳು: {
                    ಆದರೆ (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    ನಿಲ್ಲಿಸು;
                }
                ಪೂರ್ವನಿಯೋಜಿತ: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(ನಿಜ);
                ಸ್ಥಿರಾಂಕ uint32_t msbdim = dimpair.ಮೊದಲ + dimpair.ಎರಡನೇ;
                ಆದರೆ (!nodep->dimp() || msbdim < 1) {
                    ಆದರೆ (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ಸರಿಸು_L{
                            nodep->fileline(), ಹೊಸ AstLenN{nodep->fileline(), fromp},
                            ಹೊಸ AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ತಳ_DTypep = dtypep->skipRefp();
                        UASSERT_OBJ(ತಳ_DTypep, nodep, "Unsized expression");
                        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ dim = 1;
                        AstConst* ಸ್ಥಿರಾಂಕ newp = dimension_ಮೌಲ್ಯ(nodep->fileline(), ತಳ_DTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->dimp(), Const)) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ dim = VN_AS(nodep->dimp(), Const)->ಗಮ್ಯ_SInt();
                    AstConst* ಸ್ಥಿರಾಂಕ newp
                        = dimension_ಮೌಲ್ಯ(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ಸ್ಥಿರಾಂಕ tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ dimp = nodep->dimp()->unlink_Fr_ಹಿಂಭಾಗ();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                }
            }
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ protoDtp = nodep->fromp()->dtypep();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ declDtp = [&]() {
                ಆದರೆ (m_ftaskp->fvarp()) ಹಿಂತಿರುಗಿ m_ftaskp->fvarp()->dtypep();
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ voidp = ಹೊಸ AstVoidDType{m_ftaskp->fileline()};
                ತಳ್ಳು_Deletep(voidp);  // Note voidp used past here
                ಹಿಂತಿರುಗಿ voidp;
            }();
            ಆದರೆ (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warn_ಇನ್ನಷ್ಟು()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warn_ಇನ್ನಷ್ಟು()
                        << "... Declaration data type: " << declDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnContextPrimary() << '\n'
                        << declDtp->warnOther() << "... Location of out-of-block declaration\n"
                        << declDtp->warnContextSecondary());
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->fromp()->dtypep();
            string ಫಲಿತಾಂಶ = dtypep->prettyDTypeName(ನಿಜ);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                ಫಲಿತಾಂಶ += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << ಫಲಿತಾಂಶ << "' from " << dtypep);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::String{}, ಫಲಿತಾಂಶ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ VAttrType::TYPEID:
            ಆದರೆ (Ast_ಗಂಟು_DType* dtypep = VN_CAST(nodep->fromp(), ಗಂಟು_DType)) {
                nodep->dtypep(dtypep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->dtypep(VN_AS(nodep->fromp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->dtypep());
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VAttrType::VAR_ತಳ:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: {
            // Everything else resolved earlier
            nodep->dtype_ಗಣ_ಬಿಟ್_Unsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            ನಿಲ್ಲಿಸು;
        }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪಠ್ಯ* nodep) override {
        // Only used in CStmts which don't care....
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCellArrayRef* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        user_Iterate_And_ಮುಂದಿನ(nodep->selp(), ಅಗಲ_VP{SELF, PRELIM}.p());
        user_Iterate_And_ಮುಂದಿನ(nodep->selp(), ಅಗಲ_VP{SELF, FINAL}.p());
        nodep->dtype_ಗಣ_Void();  // placeholder; this node shouldnt survive beyond linking
        nodep->did_ಅಗಲ(ನಿಜ);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCellRef* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cellExprp = VN_CAST(nodep->cellp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            user_Iterate_And_ಮುಂದಿನ(cellExprp, ಅಗಲ_VP{SELF, PRELIM}.p());
            user_Iterate_And_ಮುಂದಿನ(cellExprp, ಅಗಲ_VP{SELF, FINAL}.p());
        }

        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            user_Iterate_And_ಮುಂದಿನ(exprp, ಅಗಲ_VP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            user_Iterate_And_ಮುಂದಿನ(exprp, ಅಗಲ_VP{SELF, FINAL}.p());
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->did_ಅಗಲ(ನಿಜ);
    }

    // DTYPES
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Array_DType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed

        ಆದರೆ (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            ಆದರೆ (basicp->implicit()) {
                UASSERT_OBJ(basicp->ಅಗಲ() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->ಅಗಲ() << " bits");
                AstBasicDType* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->ಅಗಲ_Force(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        ಆದರೆ (VN_IS(nodep, PackArrayDType)) m_underPackedArray = ನಿಜ;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        user_Iterate_And_ಮುಂದಿನ(nodep->rangep(), ಅಗಲ_VP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->ಅಗಲ_From_Sub(nodep->subDTypep());
            ಆದರೆ (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDTypep = nodep->subDTypep()->skipRefp();
            ಆದರೆ (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->subDTypep()->ಅಗಲ() * nodep->rangep()->elementsConst();
            nodep->ಅಗಲ_Force(ಅಗಲ, ಅಗಲ);
            ಆದರೆ (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ತಳ_DTypep = nodep->subDTypep();
                ಆಗ (!VN_IS(ತಳ_DTypep, RefDType) && ತಳ_DTypep->subDTypep()) {
                    ತಳ_DTypep = ತಳ_DTypep->subDTypep();
                }
                ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = nodep->basicp();
                ಆದರೆ (!VN_IS(ತಳ_DTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(ನಿಜ));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssocArrayDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->ಕೀಲಿ_DTypep(iterateEditMoveDTypep(nodep, nodep->ಕೀಲಿ_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ childp = nodep->ಮಗು_DTypep();
        childp->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ elementsp = nodep->elementsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು* newp;
        ಆದರೆ (VN_IS(elementsp, Unbounded)) {
            newp = ಹೊಸ Ast_ಸರತಿ_DType{nodep->fileline(), VFlag_ಮಗು_DType{}, childp, ಶೂನ್ಯಸೂಚಕ};
            VL_DO_DANGLING(elementsp->delete_ವೃಕ್ಷ(), elementsp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ keyp = VN_CAST(elementsp, ಗಂಟು_DType)) {
            newp = ಹೊಸ AstAssocArrayDType{nodep->fileline(), VFlag_ಮಗು_DType{}, childp, keyp};
        } ಇಲ್ಲದಿದ್ದರೆ {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3_Error_ಸಂಕೇತ::WIDTHEXPAND, ನಿಜ);
            // Must be expression that is constant, but we'll determine that later
            newp = ಹೊಸ AstUnpackArrayDType{
                nodep->fileline(), VFlag_ಮಗು_DType{}, childp,
                ಹೊಸ AstRange{nodep->fileline(), ಹೊಸ AstConst(elementsp->fileline(), 0),
                             ಹೊಸ AstSub{elementsNewFl, VN_AS(elementsp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ),
                                        ಹೊಸ AstConst(elementsNewFl, 1)},
                             ನಿಜ}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, ಶೂನ್ಯಸೂಚಕ), newp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintRefDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDynArrayDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರತಿ_DType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        user_Iterate_And_ಮುಂದಿನ(nodep->boundp(), ಅಗಲ_VP{SELF, BOTH}.p());
        ಆದರೆ (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVoidDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnsizedArrayDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        ಆದರೆ (nodep->ಮಗು_DTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWildcardArrayDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBasicDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        ಆದರೆ (nodep->generic()) ಹಿಂತಿರುಗಿ;  // Already perfect
        ಆದರೆ (nodep->rangep()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->rangep(), ಅಗಲ_VP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->ಅಗಲ_Force(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isRanged()) {
            nodep->ಅಗಲ_Force(nodep->nrange().elements(), nodep->nrange().elements());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->ಅಗಲ_Force(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->ಅಗಲ_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_ಅಗಲ(ನಿಜ);
        ಆದರೆ (nodep->typeofp()) {  // type(typeofp_expression)
            ಆದರೆ (Ast_ಗಂಟು_DType* typeofDtp = VN_CAST(nodep->typeofp(), ಗಂಟು_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Type comes from expression's type, e.g. "type(variable)"
                user_Iterate_And_ಮುಂದಿನ(nodep->typeofp(), ಅಗಲ_VP{SELF, BOTH}.p());
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), ಶೂನ್ಯಸೂಚಕ);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), ಶೂನ್ಯಸೂಚಕ);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        ಆದರೆ (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Template_Module = !m_modep || m_modep->parameterizedTemplate();
            ಆದರೆ (ಒಳ_Template_Module) {
                nodep->doing_ಅಗಲ(ಸುಳ್ಳು);
                ಹಿಂತಿರುಗಿ;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->ಅಗಲ_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doing_ಅಗಲ(ಸುಳ್ಳು);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    ಶೂನ್ಯ ಅಗಲ_DType_Name_Params(ಸ್ಥಿರಾಂಕ AstTypedef* nodep) {
        ಸ್ಥಿರಾಂಕ VDTypeNameScopes* ಸ್ಥಿರಾಂಕ scopesp = VDTypeNameScopes::currentp();
        ಆದರೆ (!scopesp) ಹಿಂತಿರುಗಿ;  // Computing parameters, as by V3Param, so naming as found
        Ast_ಗಂಟು* scopep = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು*>* ಸ್ಥಿರಾಂಕ paramsp = scopesp->paramsp(scopep)) {
                ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ paramp : *paramsp) {
                    ಆದರೆ (!paramp->did_ಅಗಲ()) userIterate(paramp, ಶೂನ್ಯಸೂಚಕ);
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedef* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ refp = check_Ref_ಗಮ್ಯ_Typedef_Recurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(ಶೂನ್ಯಸೂಚಕ);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ declares = nodep->ಮಗು_DTypep() != ಶೂನ್ಯಸೂಚಕ;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ಆದರೆ (declares) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->dtypep();
            ಆದರೆ (Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
                ಅಗಲ_DType_Name_Params(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstEnumDType* ಸ್ಥಿರಾಂಕ edtypep = VN_CAST(dtypep, EnumDType)) {
                ಅಗಲ_DType_Name_Params(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParamTypeDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
        nodep->ಅಗಲ_From_Sub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        ಆದರೆ (nodep->dtypep() && nodep->ಮಗು_DTypep()) { nodep->ಮಗು_DTypep(ಶೂನ್ಯಸೂಚಕ); }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRequireDType* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(nodep->lhsp(), ಗಂಟು_DType)) {
            nodep->replaceWith(dtp->unlink_Fr_ಹಿಂಭಾಗ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(ಹೊಸ AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Dtp = nodep->ತುದಿ()->dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->fromp()->dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ castable = Ast_ಗಂಟು::computeCastable(ಗಮ್ಯ_Dtp, fromDtp, nodep->fromp());
        Ast_ಗಂಟು* newp;
        ಆದರೆ (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ಸ್ಥಿರಾಂಕ enumDtp = VN_AS(ಗಮ್ಯ_Dtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ testp = enumTestValid(nodep->fromp(), enumDtp);
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl_novalue = ಹೊಸ File_ಸಾಲು{fl};
            fl_novalue->warnOff(V3_Error_ಸಂಕೇತ::ENUMVALUE, ನಿಜ);
            newp = ಹೊಸ AstCond{
                fl, testp,
                ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{fl,
                                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl_novalue, nodep->ತುದಿ()->unlink_Fr_ಹಿಂಭಾಗ(),
                                              nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()},
                                ಹೊಸ AstConst{fl, AstConst::Signed32{}, 1}},
                ಹೊಸ AstConst{fl, AstConst::Signed32{}, 0}};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(ಗಮ್ಯ_Dtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << ಗಮ್ಯ_Dtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warn_ಇನ್ನಷ್ಟು() << "... Suggest static cast");
            newp = ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                fl,
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, nodep->ತುದಿ()->unlink_Fr_ಹಿಂಭಾಗ(),
                              ಹೊಸ AstCast{fl, nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಮ್ಯ_Dtp}},
                ಹೊಸ AstConst{fl, AstConst::Signed32{}, 1}};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (castable == VCastable::INCOMPATIBLE) {
            newp = ಹೊಸ AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << ಗಮ್ಯ_Dtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp = ಹೊಸ AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << ಗಮ್ಯ_Dtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warn_ಇನ್ನಷ್ಟು()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ Ast_Cast_ಗಾತ್ರ{nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->dtp(), ಗಂಟು_DType)) {
            refp->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCast{nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                              VFlag_ಮಗು_DType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCast* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ಆದರೆ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            ಆದರೆ (AstSigned* ಸ್ಥಿರಾಂಕ fromp = VN_CAST(nodep->fromp(), Signed)) {
                ಆದರೆ (VN_IS(fromp->lhsp(), ಗಂಟು_ಹರಿವು)) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ lhsp = fromp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_ವೃಕ್ಷ(), fromp);
                }
            }
            user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, PRELIM, ಹರಿವು_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Dtp = nodep->dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->fromp()->dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ castable = Ast_ಗಂಟು::computeCastable(ಗಮ್ಯ_Dtp, fromDtp, nodep->fromp());
            ತಾರ್ಕಿಕ bad = ಸುಳ್ಳು;
            ಆದರೆ (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << ಗಮ್ಯ_Dtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << ಗಮ್ಯ_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warn_ಇನ್ನಷ್ಟು() << "... Suggest dynamic $cast");
                bad = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << ಗಮ್ಯ_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (bad) {
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = ಗಮ್ಯ_Dtp->basicp()) {
                ಆದರೆ (!basicp->isString() && fromDtp->isString()) {
                    newp = ಹೊಸ AstNToI{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಮ್ಯ_Dtp};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ origDTypep = nodep->dtypep();
                    ಆದರೆ (!VN_IS(fromDtp, ಹರಿವು_DType)) {
                        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = ಗಮ್ಯ_Dtp->ಅಗಲ();
                        castSized(nodep, nodep->fromp(), ಅಗಲ);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } ಇಲ್ಲದಿದ್ದರೆ {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 ಸುಳ್ಳು);
                }
                ಆದರೆ (newp) {
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    ಆದರೆ (nodep->fromp()->isSigned()) {
                        newp = ಹೊಸ AstISToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        newp = ಹೊಸ AstIToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = ಹೊಸ AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
                    newp->dtypep(basicp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = ಹೊಸ AstSigned{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = ಹೊಸ AstUnsigned{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ಗಮ್ಯ_Dtp, ಸರತಿ_DType)) {
                ಆದರೆ (VN_IS(fromDtp, BasicDType)) {
                    newp = ಹೊಸ Ast_Cvt_Packed_ಗಮ್ಯ_Array{nodep->fileline(),
                                                   nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಮ್ಯ_Dtp};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(fromDtp, ಸರತಿ_DType) || VN_IS(fromDtp, ಹರಿವು_DType)) {
                    ಪೂರ್ಣಾಂಕ src_Element_ಬಿಟ್ಟುಗಳು = 1;
                    ಪೂರ್ಣಾಂಕ dst_Element_ಬಿಟ್ಟುಗಳು = 1;
                    ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = fromDtp->subDTypep()) {
                        src_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                    }
                    ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ dst_ಸರತಿ_Dtp = VN_AS(ಗಮ್ಯ_Dtp, ಸರತಿ_DType);
                    ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = dst_ಸರತಿ_Dtp->subDTypep()) {
                        dst_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                    }
                    newp = ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Array{nodep->fileline(),
                                                  nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                                  ಗಮ್ಯ_Dtp,
                                                  ಸುಳ್ಳು,
                                                  1,
                                                  dst_Element_ಬಿಟ್ಟುಗಳು,
                                                  src_Element_ಬಿಟ್ಟುಗಳು};
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ಗಮ್ಯ_Dtp, ClassRefDType)) {
                // Can just remove cast
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << ಗಮ್ಯ_Dtp->prettyDTypeNameQ());
            }
            ಆದರೆ (!newp) newp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        ಆದರೆ (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, ಸುಳ್ಳು, ಹರಿವು_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ underp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
            underp->dtypeFrom(nodep);
            underp->did_ಅಗಲ(ನಿಜ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCastWrap{nodep->fileline(), underp};
            newp->did_ಅಗಲ(ನಿಜ);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cast_ಗಾತ್ರ* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->rhsp()->ಗಮ್ಯ_SInt();
            ಆದರೆ (ಅಗಲ < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << ಅಗಲ);
                ಅಗಲ = 1;
            }
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, PRELIM, ಹರಿವು_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), ಅಗಲ);  // lhsp may change
        }
        ಆದರೆ (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ underp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{nodep->dtypep(), BOTH, ಹರಿವು_USE_CAST}.p());
    }
    ಶೂನ್ಯ castSized(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* underp, ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        ಸ್ಥಿರಾಂಕ AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        ಆದರೆ (!underDtp) underDtp = underp->dtypep()->basicp();
        ಆದರೆ (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ calc_ಅಗಲ = std::max(ಅಗಲ, underDtp->ಅಗಲ());
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ calcDtp = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                calc_ಅಗಲ, calc_ಅಗಲ, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         ಸುಳ್ಳು, ಹರಿವು_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಹೊರಹೋಗು_Dtp = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                ಅಗಲ, ಅಗಲ, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(ಹೊರಹೋಗು_Dtp);
            // We ignore warnings as that is sort of the point of a cast
            ಅಗಲ_Check_Sized(nodep, "Cast expr", VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), ಹೊರಹೋಗು_Dtp, EXTEND_EXP,
                            ಸುಳ್ಳು);
            VL_DANGLING(underp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintBefore* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->lhssp(), ಅಗಲ_VP{SELF, BOTH}.p());
        user_Iterate_And_ಮುಂದಿನ(nodep->rhssp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintUnique* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->rangesp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Dist_ಅಂಶ* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), ಅಗಲ_VP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->doing_ಅಗಲ()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ತಳ್ಳು_Deletep(nodep->valuep()->unlink_Fr_ಹಿಂಭಾಗ());
            nodep->valuep(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->did_ಅಗಲ(ನಿಜ);
            ಹಿಂತಿರುಗಿ;
        }
        nodep->doing_ಅಗಲ(ನಿಜ);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        ಆದರೆ (!m_paramsOnly && nodep->is_Iface_Array_ವಿಭಜಿಸು()) m_ವಿಭಜಿಸು_Iface_Array_Varps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        ಆದರೆ (nodep->attrsp()) {
            nodep->attrsp()->foreach([ಇದು, nodep](Ast_Attr_ದ* attrp) {
                ಆದರೆ (attrp->attrType() == VAttrType::VAR_ಪೋರ್ಟ್_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    ಆದರೆ (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), ಗಂಟು_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(attrp->unlink_Fr_ಹಿಂಭಾಗ()), attrp);
                }
            });
        }
        ಆದರೆ (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_ಗಂಟು_DType* dtp = nodep->dtypep();
            Ast_ಗಂಟು_DType* np = ಶೂನ್ಯಸೂಚಕ;
            ಆಗ (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDynArrayDType* ಸ್ಥಿರಾಂಕ unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    ಆದರೆ (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    ಆದರೆ (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } ಇಲ್ಲದಿದ್ದರೆ {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(newp);
                    np = newp;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        ಆದರೆ (AstWildcardArrayDType* ಸ್ಥಿರಾಂಕ wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        ಆದರೆ (VN_IS(nodep->dtypep()->skip_Ref_ಗಮ್ಯ_Constp(), ConstDType)) nodep->isConst(ನಿಜ);
        // Parameters if implicit untyped inherit from what they are assigned to
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        ತಾರ್ಕಿಕ didchk = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        ಆದರೆ (implicitParam) {
            ಆದರೆ (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                user_Iterate_And_ಮುಂದಿನ(nodep->valuep(), ಅಗಲ_VP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                ಆದರೆ (nodep->valuep()->isDouble()) {
                    nodep->dtype_ಗಣ_Double();
                    VL_DANGLING(bdtypep);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->valuep()->isString()) {
                    nodep->dtype_ಗಣ_String();
                    VL_DANGLING(bdtypep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಪೂರ್ಣಾಂಕ ಅಗಲ = 0;
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(ಮೌಲ್ಯ_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_Bdtypep = ಮೌಲ್ಯ_DTypep->basicp();
                    ತಾರ್ಕಿಕ issigned = ಸುಳ್ಳು;
                    ಆದರೆ (bdtypep->isNosign()) {
                        ಆದರೆ (ಮೌಲ್ಯ_Bdtypep && ಮೌಲ್ಯ_Bdtypep->isSigned()) issigned = ನಿಜ;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        issigned = bdtypep->isSigned();
                    }
                    ಆದರೆ (ಮೌಲ್ಯ_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->valuep(), Const)) {
                            ಆದರೆ (constp->ಅಂಕೆ().isString()) {
                                ಅಗಲ = constp->ಅಂಕೆ().ಗಮ್ಯ_String().ಉದ್ದಳತೆ() * 8;
                            }
                        }
                        ಆದರೆ (ಅಗಲ < 8) ಅಗಲ = 8;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->valuep()->dtypep()->ಅಗಲ_Sized()) {
                        ಅಗಲ = nodep->valuep()->ಅಗಲ();
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಆದರೆ (nodep->valuep()->ಅಗಲ() > 32) {
                            nodep->valuep()->v3warn(
                                ಅಗಲ_2,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        ಅಗಲ = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ is_ಮೌಲ್ಯ_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    ಆದರೆ (ಅಗಲ == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_Range_DType(
                            VNumRange{0, 0}, nodep->valuep()->ಅಗಲ_Min(),
                            VSigning::fromBool(issigned), is_ಮೌಲ್ಯ_Fourstate));
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(ಅಗಲ, nodep->valuep()->ಅಗಲ_Min(),
                                                         VSigning::fromBool(issigned),
                                                         is_ಮೌಲ್ಯ_Fourstate);
                    }
                    didchk = ನಿಜ;
                }
                iterate_Check_ನಿಯೋಜಿಸು(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Or, if nothing assigned, they're integral
                nodep->dtype_ಗಣ_Integer();
                VL_DANGLING(bdtypep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtype_ಗಣ_Logic_Sized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        ಆದರೆ (nodep->isNet()) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            ಆದರೆ (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        ಆದರೆ (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            user_Iterate_And_ಮುಂದಿನ(nodep->valuep(), ಅಗಲ_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_ನಿಯೋಜಿಸು(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        user_Iterate_And_ಮುಂದಿನ(nodep->delayp(), ಅಗಲ_VP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->did_ಅಗಲ(ನಿಜ);
        nodep->doing_ಅಗಲ(ಸುಳ್ಳು);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!nodep->varp()) {
            ಆದರೆ (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, ಸುಳ್ಳು,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        ಆದರೆ (!nodep->varp()->did_ಅಗಲ()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), ಶೂನ್ಯಸೂಚಕ);
        }
        ಆದರೆ (!m_paramsOnly && nodep->varp()->is_Iface_Array_ವಿಭಜಿಸು()) {
            replace_ವಿಭಜಿಸು_Iface_Array_Ref(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        ಆದರೆ (VN_IS(nodep->backp(), ಗಂಟು_ನಿಯೋಜಿಸು) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        ಆದರೆ (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(ನಿಜ);
        ಆದರೆ (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->access().isWriteOrRW() && nodep->varp()->is_ಒಳಹರಿವು()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), ನಿಯೋಜಿಸು_Force) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hierRef = !m_curModVars.ಎಣಿಕೆ_2(nodep->varp());
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ net_ಪೋರ್ಟ್
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::ಪೋರ್ಟ್;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ cont_ನಿಯೋಜಿಸು = VN_IS(nodep->abovep(), ನಿಯೋಜಿಸು_W);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ pinConn = VN_IS(nodep->abovep(), Pin);
            ಆದರೆ (!(hierRef && net_ಪೋರ್ಟ್ && (cont_ನಿಯೋಜಿಸು || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        ಆದರೆ (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->did_ಅಗಲ(ನಿಜ);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstEnumDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_ಗಂಟು_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        ಆದರೆ (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->ಕಂಡುಹಿಡಿ_Int_DType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->ಅಗಲ_From_Sub(nodep->subDTypep());
        // Assign widths
        user_Iterate_And_ಮುಂದಿನ(nodep->itemsp(), ಅಗಲ_VP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ(nodep, nodep->ಅಗಲ(), 0);
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ one{nodep, nodep->ಅಗಲ(), 1};
        ತಾರ್ಕಿಕ wrapAround = ಸುಳ್ಳು;
        std::map<ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ, Ast_Enum_ಅಂಶ*> inits;
        ಗಾಗಿ (Ast_Enum_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
            ಆದರೆ (itemp->valuep()) {
                ಆದರೆ (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_ವೃಕ್ಷ("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                ಆದರೆ (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                    ಮುಂದುವರಿ;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = ಸುಳ್ಳು;  // Explicit value resets wrap-around tracking
            }
            ಆದರೆ (!itemp->valuep()) {
                ಆದರೆ (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                ಆದರೆ (ಅಂಕೆ.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(ಹೊಸ AstConst{itemp->fileline(), ಅಂಕೆ});
            }

            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(itemp->valuep(), Const);
            ಆದರೆ (constp->ಅಂಕೆ().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            ಅಂಕೆ.op_ನಿಯೋಜಿಸು(constp->ಅಂಕೆ());
            // Look for duplicates
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = inits.emplace(ಅಂಕೆ, itemp);
            ಆದರೆ (!pair.ಎರಡನೇ) {  // IEEE says illegal
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ otherp = pair.ಮೊದಲ->ಎರಡನೇ;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ prevNeg = constp->ಅಂಕೆ().isNegative();
            ಅಂಕೆ.op_ಕೂಡಿಸು(one, constp->ಅಂಕೆ());
            ಆದರೆ (basicp->isSigned()) {
                wrapAround = !prevNeg && ಅಂಕೆ.isNegative();
            } ಇಲ್ಲದಿದ್ದರೆ {
                wrapAround = ಅಂಕೆ.isEqZero();
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        ಆದರೆ (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            user_Iterate_And_ಮುಂದಿನ(nodep->valuep(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            ತಾರ್ಕಿಕ warnOn = ನಿಜ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nodep->valuep();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಕೂಡಿಸು* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(valuep, ಕೂಡಿಸು_2)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                ಆದರೆ (anodep->fileline()->equal_ಮೊದಲ_ಸಾಲು_ಲಂಬಸಾಲು(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const)) {
                ಆದರೆ (static_cast<ಪೂರ್ಣಾಂಕ>(constp->ಅಂಕೆ().most_ಗಣ_ಬಿಟ್_P1()) > nodep->ಅಗಲ()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = ಸುಳ್ಳು;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(ಹೊಸ AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      nodep->dtypep()});
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ_Ref* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        ಆದರೆ (!nodep->itemp()->did_ಅಗಲ()) {
            // We need to do the whole enum en masse
            Ast_ಗಂಟು* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            ಗಾಗಿ (; enump; enump = enump->backp()) {
                ಆದರೆ (VN_IS(enump, EnumDType)) ನಿಲ್ಲಿಸು;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cons* nodep) override {}  // Widthed when created
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cons_ಅಂಶ* nodep) override {}  // Widthed when created
    ಶೂನ್ಯ ಭೇಟಿ(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
        ಗಾಗಿ (Ast_ಗಂಟು *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ಗಂಟು_DType* subDTypep = ಶೂನ್ಯಸೂಚಕ;
        nodep->dtype_ಗಣ_ಬಿಟ್();

        ಆದರೆ (dtype && dtype->isString()) {
            nodep->dtype_ಗಣ_String();
            subDTypep = nodep->ಕಂಡುಹಿಡಿ_String_DType();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dtype && dtype->isDouble()) {
            nodep->dtype_ಗಣ_Double();
            subDTypep = nodep->ಕಂಡುಹಿಡಿ_Double_DType();
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Take width as maximum across all items
            ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->exprp()->ಅಗಲ();
            ಪೂರ್ಣಾಂಕ mwidth = nodep->exprp()->ಅಗಲ_Min();
            ತಾರ್ಕಿಕ isFourstate = nodep->exprp()->dtypep()->isFourstate();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                ಅಗಲ = std::max(ಅಗಲ, itemp->ಅಗಲ());
                mwidth = std::max(mwidth, itemp->ಅಗಲ_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                ಅಗಲ, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        ಗಾಗಿ (Ast_ಗಂಟು *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, Dist_ಅಂಶ)->rangep();
            ಆದರೆ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, ಅಗಲ_VP{subDTypep, FINAL}.p());
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        ಆದರೆ (m_constraintp) {
            ತಾರ್ಕಿಕ can_ಅಧೋ = ನಿಜ;
            ಗಾಗಿ (Ast_Dist_ಅಂಶ* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), Dist_ಅಂಶ)) {
                ಆದರೆ (VN_IS(ditemp->rangep(), InsideRange)) ಮುಂದುವರಿ;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಅಂಶ_Dtp = ditemp->rangep()->dtypep()->skipRefp();
                ಆದರೆ (!ಅಂಶ_Dtp->isIntegralOrPacked() || !ditemp->rangep()->is_ಶುದ್ಧ()) {
                    can_ಅಧೋ = ಸುಳ್ಳು;
                    ನಿಲ್ಲಿಸು;
                }
            }
            ಆದರೆ (can_ಅಧೋ) ಹಿಂತಿರುಗಿ;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_Dist_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Dist_ಅಂಶ)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inewp
                = inside_ಅಂಶ(nodep, nodep->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), itemp->rangep());
            ಆದರೆ (!inewp) ಮುಂದುವರಿ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cmpp
                = ಹೊಸ AstGt{itemp->fileline(), itemp->weightp()->unlink_Fr_ಹಿಂಭಾಗ(),
                            ಹೊಸ AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNSIGNED, ನಿಜ);
            cmpp->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::CMPCONST, ನಿಜ);
            inewp = ಹೊಸ AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? ಹೊಸ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ಆದರೆ (!newp) newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstInside* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
        ಗಾಗಿ (Ast_ಗಂಟು *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ಗಂಟು_DType* expDTypep = ಶೂನ್ಯಸೂಚಕ;

        ಆದರೆ (dtype && dtype->isString()) {
            nodep->dtype_ಗಣ_String();
            expDTypep = nodep->ಕಂಡುಹಿಡಿ_String_DType();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dtype && dtype->isDouble()) {
            nodep->dtype_ಗಣ_Double();
            expDTypep = nodep->ಕಂಡುಹಿಡಿ_Double_DType();
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Take width as maximum across all items
            ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->exprp()->ಅಗಲ();
            ಪೂರ್ಣಾಂಕ mwidth = nodep->exprp()->ಅಗಲ_Min();
            ತಾರ್ಕಿಕ isFourstate = nodep->exprp()->dtypep()->isFourstate();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                ಅಗಲ = std::max(ಅಗಲ, itemp->ಅಗಲ());
                mwidth = std::max(mwidth, itemp->ಅಗಲ_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtype_ಗಣ_ಬಿಟ್();
            ಸ್ಥಿರಾಂಕ VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(ಅಗಲ, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        ಗಾಗಿ (Ast_ಗಂಟು *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            ಆದರೆ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, ಅಗಲ_VP{expDTypep, FINAL}.p());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;
        Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಅಭಿವ್ಯಕ್ತಿ_Stmtp = ಶೂನ್ಯಸೂಚಕ;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        ಆದರೆ (!m_constraintp && !nodep->exprp()->is_ಶುದ್ಧ()) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Dtp = nodep->exprp()->dtypep();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ w = ಅಭಿವ್ಯಕ್ತಿ_Dtp->ಅಗಲ();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ತಾತ್ಕಾಲಿಕ_DTypep
                = ಅಭಿವ್ಯಕ್ತಿ_Dtp->ಅಗಲ_Sized() ? ಅಭಿವ್ಯಕ್ತಿ_Dtp
                                        : nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(w, w, ಅಭಿವ್ಯಕ್ತಿ_Dtp->numeric(),
                                                                     ಅಭಿವ್ಯಕ್ತಿ_Dtp->isFourstate());
            AstVar* ಸ್ಥಿರಾಂಕ varp
                = ಹೊಸ AstVar{fl, VVarType::XTEMP, m_inside_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು.get(nodep), ತಾತ್ಕಾಲಿಕ_DTypep};
            exprp = ಹೊಸ AstVarRef{fl, varp, VAccess::READ};
            ಅಭಿವ್ಯಕ್ತಿ_Stmtp = ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{fl,
                                        ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ()},
                                        exprp};
            ಆದರೆ (m_ftaskp) {
                varp->funcLocal(ನಿಜ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(ಅಭಿವ್ಯಕ್ತಿ_Stmtp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);  // Will be unlinking
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inewp;
            ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_Stmtp) {
                inewp = inside_ಅಂಶ(nodep, ಅಭಿವ್ಯಕ್ತಿ_Stmtp, itemp);
                ಅಭಿವ್ಯಕ್ತಿ_Stmtp = ಶೂನ್ಯಸೂಚಕ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                inewp = inside_ಅಂಶ(nodep, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), itemp);
            }
            ಆದರೆ (!inewp) ಮುಂದುವರಿ;
            newp = newp ? ಹೊಸ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_Stmtp) VL_DO_DANGLING(ಅಭಿವ್ಯಕ್ತಿ_Stmtp->delete_ವೃಕ್ಷ(), ಅಭಿವ್ಯಕ್ತಿ_Stmtp);
        ಆದರೆ (!newp) newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inside_ಅಂಶ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemp) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಅಂಶ_Dtp = itemp->dtypep()->skipRefp();
        ಆದರೆ (AstInsideRange* ಸ್ಥಿರಾಂಕ irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            ಹಿಂತಿರುಗಿ irangep->newAndFromInside(exprp, irangep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                             irangep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ಅಂಶ_Dtp, UnpackArrayDType) || VN_IS(ಅಂಶ_Dtp, DynArrayDType)
                   || VN_IS(ಅಂಶ_Dtp, ಸರತಿ_DType)) {
            // Unsupported in parameters
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ inewp = ಹೊಸ AstCMethodHard{nodep->fileline(), itemp->unlink_Fr_ಹಿಂಭಾಗ(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, ಅಂಶ_Dtp->subDTypep(), BOTH);
            inewp->dtype_ಗಣ_ಬಿಟ್();
            inewp->did_ಅಗಲ(ನಿಜ);
            ಹಿಂತಿರುಗಿ inewp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ಅಂಶ_Dtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_ವೃಕ್ಷ(), exprp);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlink_Fr_ಹಿಂಭಾಗ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        ಆದರೆ (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_vup->prelim()) {
                user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), m_vup);
                user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), m_vup);
            }
            ಆದರೆ (m_vup->final()) {
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ exp_ಅಗಲ = expDTypep->ಅಗಲ();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ waiveLhs = exp_ಅಗಲ == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->ಅಗಲ_Min() >= nodep->lhsp()->ಅಗಲ();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ waiveRhs = exp_ಅಗಲ == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && exp_ಅಗಲ >= nodep->rhsp()->ಅಗಲ_Min();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        ಆದರೆ (nodep->isVirtual()) nodep->iface_ಮೂಲಕ_Cellp()->ಗಣ_Has_Virtual_Ref();
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಆದರೆ (nodep->doing_ಅಗಲ()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtype_ಗಣ_ಬಿಟ್();
            nodep->doing_ಅಗಲ(ಸುಳ್ಳು);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_ಅಗಲ(ನಿಜ);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnionDType* ಸ್ಥಿರಾಂಕ unionp = VN_CAST(nodep, UnionDType)) {
            ಆದರೆ (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(ಸುಳ್ಳು);
        // Error checks
        ಗಾಗಿ (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = itemp->subDTypep()->skipRefp();
            ಆದರೆ (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            ಆದರೆ ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ತಳ್ಳು_Deletep(itemp->valuep()->unlink_Fr_ಹಿಂಭಾಗ());
            }
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Template_Module = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        ಆದರೆ (VN_IS(nodep, UnionDType) || nodep->packed()) {
            ಪೂರ್ಣಾಂಕ lsb = 0;
            ಪೂರ್ಣಾಂಕ ಅಗಲ = 0;
            ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            ಗಾಗಿ (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            ಗಾಗಿ (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                ಆದರೆ (itemp->isFourstate()) nodep->isFourstate(ನಿಜ);
                itemp->lsb(lsb);
                ಆದರೆ (VN_IS(nodep, UnionDType)) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಂಶ_ಅಗಲ = itemp->ಅಗಲ();
                    // Skip union size check for template modules with unresolved parameters
                    ಆದರೆ (!ಮೊದಲ && isHardPackedUnion && ಅಂಶ_ಅಗಲ != ಅಗಲ && !ಒಳ_Template_Module) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    ಅಗಲ = std::max(ಅಗಲ, ಅಂಶ_ಅಗಲ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    lsb += itemp->ಅಗಲ();
                    ಅಗಲ += itemp->ಅಗಲ();
                }
                ಮೊದಲ = ಸುಳ್ಳು;
            }
            nodep->ಅಗಲ_Force(ಅಗಲ, ಅಗಲ);  // Signing stays as-is, as parsed from declaration
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->ಅಗಲ_Force(1, 1);
        }
        nodep->doing_ಅಗಲ(ಸುಳ್ಳು);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstThisRef* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->ಮಗು_DTypep()));
        ಆದರೆ (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassRefDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warn_ಇನ್ನಷ್ಟು() << "... Suggest use a typedef");
        nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassExtends* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (VN_IS(nodep->ಮಗು_DTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->ಮಗು_DTypep()));
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberDType* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->ಅಗಲ_From_Sub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        ಆದರೆ (nodep->valuep()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->valuep(), ಅಗಲ_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_ನಿಯೋಜಿಸು(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = ನಿಜ;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->fromp()->dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
        UINFO(9, "     from dt " << fromDtp);
        ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ fromSel = VN_CAST(nodep->fromp(), MemberSel);
        ಆದರೆ (AstClocking* ಸ್ಥಿರಾಂಕ clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->ಮೊದಲ_Abovep(), Clocking)
                                               : ಶೂನ್ಯಸೂಚಕ) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = memberSelClocking(nodep, clockingp)) {
                ಆದರೆ (!varp->did_ಅಗಲ()) userIterate(varp, ಶೂನ್ಯಸೂಚಕ);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlink_Fr_ಹಿಂಭಾಗ());
                VL_DO_DANGLING(fromp->delete_ವೃಕ್ಷ(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                ಆದರೆ (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::link_LValue_ಗಣ(nodep, nodep->access());
                }
                ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->did_ಅಗಲ(ನಿಜ);  // Must not visit again: will confuse scopes
                ಹಿಂತಿರುಗಿ;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, ಗಂಟು_UOr_Struct_DType)) {
            ಆದರೆ (memberSelStruct(nodep, adtypep)) ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            ಆದರೆ (memberSelClass(nodep, adtypep)) ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            ಆದರೆ (Ast_ಗಂಟು* foundp = memberSelIface(nodep, adtypep)) {
                ಆದರೆ (AstClocking* ಸ್ಥಿರಾಂಕ clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(foundp, Var)) {
                    ಆದರೆ (!varp->did_ಅಗಲ()) userIterate(varp, ಶೂನ್ಯಸೂಚಕ);
                    AstIface* ಸ್ಥಿರಾಂಕ ifacep = adtypep->iface_ಮೂಲಕ_Cellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->fromp(), VarRef);
                    ಆದರೆ (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->did_ಅಗಲ(ನಿಜ);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                        ಹಿಂತಿರುಗಿ;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_ಅಗಲ(ನಿಜ);
                    ಹಿಂತಿರುಗಿ;
                }
                ಆದರೆ (AstModport* ಸ್ಥಿರಾಂಕ modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ಸ್ಥಿರಾಂಕ newDtypep = ಹೊಸ AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(newDtypep);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
                ಆದರೆ (AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    ಆದರೆ (VN_IS(cellp->modp(), Iface)) {
                        ಸ್ಥಿರಾಂಕ string viftopName = cellp->name() + "__Viftop";
                        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Ifacep = adtypep->iface_ಮೂಲಕ_Cellp();
                        AstVar* viftopVarp = ಶೂನ್ಯಸೂಚಕ;
                        ಗಾಗಿ (Ast_ಗಂಟು* itemp = ಪೋಷಕ_Ifacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ vp = VN_CAST(itemp, Var)) {
                                ಆದರೆ (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    ನಿಲ್ಲಿಸು;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        ಆದರೆ (!viftopVarp->did_ಅಗಲ()) userIterate(viftopVarp, ಶೂನ್ಯಸೂಚಕ);
                        Ast_ಗಂಟು_DType* subDtypep = viftopVarp->dtypep();
                        ಆದರೆ (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ಸ್ಥಿರಾಂಕ newDtypep = ಹೊಸ AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(ನಿಜ);
                                v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->did_ಅಗಲ(ನಿಜ);
                        ಹಿಂತಿರುಗಿ;
                    }
                }
                ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp = VN_CAST(foundp, ಗಂಟು_FTask)) {
                    Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ವಿಧಾನ_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ಹಿಂತಿರುಗಿ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, ಸರತಿ_DType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ವಿಧಾನ_Call{nodep->fileline(),
                                                    nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ತಾರ್ಕಿಕ memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        ಆದರೆ (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ವಿಧಾನ_Call{
                nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಭೇಟಿ(newp);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ಸ್ಥಿರಾಂಕ ಮೊದಲ_classp = adtypep->classp();
        UASSERT_OBJ(ಮೊದಲ_classp, nodep, "Unlinked");
        ಗಾಗಿ (AstClass* classp = ಮೊದಲ_classp; classp;) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ foundp = m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, nodep->name())) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(foundp, Var)) {
                    ಆದರೆ (!varp->did_ಅಗಲ()) userIterate(varp, ಶೂನ್ಯಸೂಚಕ);
                    ಆದರೆ (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ಸ್ಥಿರಾಂಕ varRefp
                            = ಹೊಸ AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                    ಆದರೆ (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_ಅಗಲ(ನಿಜ);
                    ಆದರೆ (nodep->fromp()->same_ವೃಕ್ಷ(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::link_LValue_ಗಣ(nodep);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
                ಆದರೆ (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(ಹೊಸ AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), constrp});
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
                ಆದರೆ (Ast_Enum_ಅಂಶ_Ref* ಸ್ಥಿರಾಂಕ adfoundp = VN_CAST(foundp, Enum_ಅಂಶ_Ref)) {
                    nodep->replaceWith(adfoundp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
                ಆದರೆ (Ast_ಗಂಟು_FTask* ftaskp = VN_CAST(foundp, ಗಂಟು_FTask)) {
                    Ast_ವಿಧಾನ_Call* newp = ಹೊಸ Ast_ವಿಧಾನ_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
                ಆದರೆ (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->ಕಂಡುಹಿಡಿ_Constraint_Ref_DType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ;
        }

        VSpellCheck speller;
        ಗಾಗಿ (AstClass* classp = ಮೊದಲ_classp; classp;) {
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                ಆದರೆ (VN_IS(itemp, Constraint) || VN_IS(itemp, Enum_ಅಂಶ_Ref) || VN_IS(itemp, Var)) {
                    speller.ತಳ್ಳು_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ;
        }
        ಸ್ಥಿರಾಂಕ string suggest = speller.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << ಮೊದಲ_classp->verilogKwd()
                      << " " << ಮೊದಲ_classp->prettyNameQ() << "\n"
                      << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->fileline()->warn_ಇನ್ನಷ್ಟು() + suggest));
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Caller handles error
    }
    ತಾರ್ಕಿಕ modport_Allows_ಅಂಶ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ AstIfaceRefDType* adtypep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ಸ್ಥಿರಾಂಕ AstModport* ಸ್ಥಿರಾಂಕ modportp = adtypep->modportp();
        ಆದರೆ (!modportp) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(itemp, Var)) {
            ಆದರೆ (varp->isParam()) ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(itemp, ಗಂಟು_FTask) && !VN_IS(itemp, Clocking)) {
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಸ್ವಯಂಚಾಲಿತ pair = m_modport_ಹೆಸರುಗಳು.emplace(modportp, std::unordered_ಗಣ<string>{});
        ಆದರೆ (pair.ಎರಡನೇ) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.ಮೊದಲ->ಎರಡನೇ.ಸೇರಿಸು(mpItemp->name());
            }
        }
        ಆದರೆ (pair.ಮೊದಲ->ಎರಡನೇ.ಎಣಿಕೆ_2(itemp->name())) ಹಿಂತಿರುಗಿ ನಿಜ;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->iface_ಮೂಲಕ_Cellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    Ast_ಗಂಟು* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ifacep = adtypep->iface_ಮೂಲಕ_Cellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (itemp->name() == nodep->name()) {
                ಆದರೆ (!modport_Allows_ಅಂಶ(nodep, adtypep, itemp)) {
                    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Caller handles error
                }
                ಹಿಂತಿರುಗಿ itemp;
            }
            ಆದರೆ (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ತಳ್ಳು_Candidate(itemp->prettyName());
            }
        }
        ಸ್ಥಿರಾಂಕ string suggest = speller.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->fileline()->warn_ಇನ್ನಷ್ಟು() + suggest));
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        ಗಾಗಿ (Ast_ಗಂಟು* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (Ast_Clocking_ಅಂಶ* citemp = VN_CAST(itemp, Clocking_ಅಂಶ)) {
                ಆದರೆ (citemp->varp()->name() == nodep->name()) ಹಿಂತಿರುಗಿ citemp->varp();
                speller.ತಳ್ಳು_Candidate(citemp->varp()->prettyName());
            }
        }
        ಸ್ಥಿರಾಂಕ string suggest = speller.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->fileline()->warn_ಇನ್ನಷ್ಟು() + suggest));
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Caller handles error
    }
    ತಾರ್ಕಿಕ memberSelStruct(AstMemberSel* nodep, Ast_ಗಂಟು_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        ಆದರೆ (AstMemberDType* ಸ್ಥಿರಾಂಕ memberp
            = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(adtypep, nodep->name()), MemberDType)) {
            ಆದರೆ (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (adtypep->packed()) {
                AstSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSel{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                                memberp->lsb(), memberp->ಅಗಲ()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump());
                newp->did_ಅಗಲ(ನಿಜ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstStructSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump());
                newp->did_ಅಗಲ(ನಿಜ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ವಿಧಾನ_Call* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), ಅಗಲ_VP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->fromp()->dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
        AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = fromDtp ? fromDtp->basicp() : ಶೂನ್ಯಸೂಚಕ;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, ಅಗಲ_VP{SELF, BOTH}.p());
        ಆದರೆ (nodep->name() == "rand_mode") {
            ವಿಧಾನ_Call_Rand_ಬಹುಲಕ(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "constraint_mode") {
            ವಿಧಾನ_Call_Constraint(nodep, ಶೂನ್ಯಸೂಚಕ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstEnumDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, EnumDType)) {
            ವಿಧಾನ_Call_Enum(nodep, adtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstAssocArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            ವಿಧಾನ_Call_Assoc(nodep, adtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstWildcardArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            ವಿಧಾನ_Call_Wildcard(nodep, adtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstDynArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            ವಿಧಾನ_Call_Dyn(nodep, adtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, ಸರತಿ_DType)) {
            ವಿಧಾನ_Call_ಸರತಿ(nodep, adtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            ವಿಧಾನ_Call_Class(nodep, adtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            ವಿಧಾನ_Call_Iface_Ref(nodep, adtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            ವಿಧಾನ_Call_Unpack(nodep, adtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstConstraintRefDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            ವಿಧಾನ_Call_Constraint(nodep, adtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->isEvent()) {
            ವಿಧಾನ_Call_Event(nodep, basicp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->isString()) {
            ವಿಧಾನ_Call_String(nodep, basicp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtype_ಗಣ_Void();
        }
    }
    AstWith* ವಿಧಾನ_With_Clause(Ast_ಗಂಟು_FTask_Ref* nodep, ತಾರ್ಕಿಕ required, ತಾರ್ಕಿಕ arbReturn,
                              Ast_ಗಂಟು_DType* returnDtp, Ast_ಗಂಟು_DType* ಸೂಚ್ಯಂಕ_Dtp,
                              Ast_ಗಂಟು_DType* ಮೌಲ್ಯ_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        ಆದರೆ (AstWith* ಸ್ಥಿರಾಂಕ withp = nodep->withp()) {
            withp->ಸೂಚ್ಯಂಕ_Arg_Refp()->dtypep(ಸೂಚ್ಯಂಕ_Dtp);
            withp->ಮೌಲ್ಯ_Arg_Refp()->dtypep(ಮೌಲ್ಯ_Dtp);
            userIterate(withp, ಅಗಲ_VP{returnDtp, BOTH}.p());
            withp->unlink_Fr_ಹಿಂಭಾಗ();
            ಹಿಂತಿರುಗಿ withp;
        }
        ಆದರೆ (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ ವಿಧಾನ_Ok_Arguments(Ast_ಗಂಟು_FTask_Ref* nodep, ಪೂರ್ಣಾಂಕ minArg, ಪೂರ್ಣಾಂಕ maxArg) {
        ಪೂರ್ಣಾಂಕ narg = 0;
        ಆದರೆ (AstWith* ಸ್ಥಿರಾಂಕ withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(withp->unlink_Fr_ಹಿಂಭಾಗ()), withp);
        }
        ಗಾಗಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ok = (narg >= minArg) && (narg <= maxArg);
        ಆದರೆ (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvt_ಗಮ್ಯ_Str(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvt_ಗಮ್ಯ_Str(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            ಗಾಗಿ (; narg < minArg; ++narg) {
                nodep->ಕೂಡಿಸು_Argsp(
                    ಹೊಸ AstArg{nodep->fileline(), "",
                               ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}}});
            }
            ಗಾಗಿ (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                ಆಗ (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlink_Fr_ಹಿಂಭಾಗ();
                VL_DO_DANGLING(argp->delete_ವೃಕ್ಷ(), argp);
            }
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ವಿಧಾನ_Arg(Ast_ವಿಧಾನ_Call* nodep, ಪೂರ್ಣಾಂಕ arg) {
        AstArg* argp = nodep->argsp();
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        ಹಿಂತಿರುಗಿ argp->exprp();
    }

    ಶೂನ್ಯ ವಿಧಾನ_Call_Enum(Ast_ವಿಧಾನ_Call* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        ಆದರೆ (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = ಶೂನ್ಯಸೂಚಕ;
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ಆದರೆ (nodep->name() == "num") {
                ಪೂರ್ಣಾಂಕ ವಸ್ತುಗಳು = 0;
                ಗಾಗಿ (Ast_ಗಂಟು* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++ವಸ್ತುಗಳು;
                newp = ಹೊಸ AstConst(nodep->fileline(), AstConst::Signed32{}, ವಸ್ತುಗಳು);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "first") {
                ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp = adtypep->itemsp();
                ಆದರೆ (!itemp) {
                    newp = ಹೊಸ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newp = VN_AS(itemp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "last") {
                ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp = adtypep->itemsp();
                ಆಗ (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ);
                ಆದರೆ (!itemp) {
                    newp = ಹೊಸ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newp = VN_AS(itemp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            ಆದರೆ (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "next") {
                attrType = VAttrType::ENUM_ಮುಂದಿನ;
                ವಿಧಾನ_Ok_Arguments(nodep, 0, 1);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                ವಿಧಾನ_Ok_Arguments(nodep, 0, 1);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Bad case");
            }

            ಆದರೆ (nodep->name() != "name" && nodep->argsp()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* stepp = ವಿಧಾನ_Arg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = ವಿಧಾನ_Arg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::WIDTHEXPAND, ನಿಜ);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                ಆದರೆ (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ಸ್ಥಿರಾಂಕ uint32_t ಹೆಜ್ಜೆ_ಅಗಲ = VN_AS(stepp, Const)->ಗಮ್ಯ_UInt();
                ಆದರೆ (ಹೆಜ್ಜೆ_ಅಗಲ == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ವಿಧಾನ_Call{
                        nodep->fileline(),
                        ಹೊಸ Ast_ವಿಧಾನ_Call{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ಹಿಂತಿರುಗಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಹೆಜ್ಜೆ_ಅಗಲ != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ತಳ್ಳು_Deletep(nodep->argsp()->unlink_Fr_ಹಿಂಭಾಗ());
                    Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ clonep = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    VN_AS(stepp, Const)->ಅಂಕೆ().ಗಣ_Long(1);
                    AstConst* ಸ್ಥಿರಾಂಕ constp = ಹೊಸ AstConst(nodep->fileline(), ಹೆಜ್ಜೆ_ಅಗಲ - 1);
                    AstArg* ಸ್ಥಿರಾಂಕ argp = ಹೊಸ AstArg{nodep->fileline(), "", constp};
                    Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ Ast_ವಿಧಾನ_Call{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp
                = enumSelect(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Wildcard(Ast_ವಿಧಾನ_Call* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::ASSOC_ಗಾತ್ರ};  // So don't need num()
            newp->dtype_ಗಣ_Int();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), SELF, BOTH);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = ವಿಧಾನ_Call_Wildcard_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::ASSOC_EXISTS, ಸೂಚ್ಯಂಕ_exprp->unlink_Fr_ಹಿಂಭಾಗ()};
            newp->dtype_ಗಣ_ಬಿಟ್_Unsized(32, 1, VSigning::SIGNED);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "delete") {  // function void delete([input integer index])
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 1);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            ಆದರೆ (!nodep->argsp()) {
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_ಗಣ_Void();
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateCheckSelf(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), SELF, BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = ವಿಧಾನ_Call_Wildcard_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          VCMethod::ASSOC_ERASE, ಸೂಚ್ಯಂಕ_exprp->unlink_Fr_ಹಿಂಭಾಗ()};
                newp->dtype_ಗಣ_Void();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ಸ್ಥಿರಾಂಕ withp
                = ವಿಧಾನ_With_Clause(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, adtypep->subDTypep(),
                                   adtypep->ಕಂಡುಹಿಡಿ_String_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(adtypep->subDTypep()));
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ಸ್ಥಿರಾಂಕ withp
                = ವಿಧಾನ_With_Clause(nodep, ನಿಜ, ಸುಳ್ಳು, nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(),
                                   adtypep->ಕಂಡುಹಿಡಿ_String_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(adtypep->subDTypep()));
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "map") {
            AstWith* ಸ್ಥಿರಾಂಕ withp
                = ವಿಧಾನ_With_Clause(nodep, ನಿಜ, ಸುಳ್ಳು, adtypep->subDTypep(),
                                   adtypep->ಕಂಡುಹಿಡಿ_String_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ಆದರೆ (newp) {
            newp->protect(ಸುಳ್ಳು);
            newp->did_ಅಗಲ(ನಿಜ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Assoc(Ast_ವಿಧಾನ_Call* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        ಆದರೆ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::ASSOC_ಗಾತ್ರ};  // So don't need num()
            newp->dtype_ಗಣ_Int();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), adtypep->ಕೀಲಿ_DTypep(), BOTH);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = ವಿಧಾನ_Call_Assoc_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, ಸೂಚ್ಯಂಕ_exprp, VAccess::READWRITE);
            newp
                = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                     VCMethod::array_ವಿಧಾನ(nodep->name()),  // first/last/next/prev
                                     ಸೂಚ್ಯಂಕ_exprp->unlink_Fr_ಹಿಂಭಾಗ()};
            newp->dtype_ಗಣ_Int();
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), adtypep->ಕೀಲಿ_DTypep(), BOTH);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = ವಿಧಾನ_Call_Assoc_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::ASSOC_EXISTS, ಸೂಚ್ಯಂಕ_exprp->unlink_Fr_ಹಿಂಭಾಗ()};
            newp->dtype_ಗಣ_ಬಿಟ್_Unsized(32, 1, VSigning::SIGNED);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "delete") {  // function void delete([input integer index])
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 1);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            ಆದರೆ (!nodep->argsp()) {
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_ಗಣ_Void();
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateCheckTyped(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), adtypep->ಕೀಲಿ_DTypep(),
                                  BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = ವಿಧಾನ_Call_Assoc_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          VCMethod::ASSOC_ERASE, ಸೂಚ್ಯಂಕ_exprp->unlink_Fr_ಹಿಂಭಾಗ()};
                newp->dtype_ಗಣ_Void();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ಸ್ಥಿರಾಂಕ withp = ವಿಧಾನ_With_Clause(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, adtypep->subDTypep(),
                                                    adtypep->ಕೀಲಿ_DTypep(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ಸ್ಥಿರಾಂಕ withp = ವಿಧಾನ_With_Clause(
                nodep, ಸುಳ್ಳು, ನಿಜ, ಶೂನ್ಯಸೂಚಕ, nodep->ಕಂಡುಹಿಡಿ_UInt32_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->withp(withp);
            ಆದರೆ (nodep->name() == "unique_index") {
                newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(adtypep->ಕೀಲಿ_DTypep()));
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(adtypep->subDTypep()));
            }
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ಸ್ಥಿರಾಂಕ withp = ವಿಧಾನ_With_Clause(nodep, ನಿಜ, ಸುಳ್ಳು, nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(),
                                                    adtypep->ಕೀಲಿ_DTypep(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(adtypep->subDTypep()));
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ಸ್ಥಿರಾಂಕ withp = ವಿಧಾನ_With_Clause(nodep, ನಿಜ, ಸುಳ್ಳು, nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(),
                                                    adtypep->ಕೀಲಿ_DTypep(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(adtypep->ಕೀಲಿ_DTypep()));
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "map") {
            AstWith* ಸ್ಥಿರಾಂಕ withp = ವಿಧಾನ_With_Clause(nodep, ನಿಜ, ಸುಳ್ಳು, adtypep->subDTypep(),
                                                    adtypep->ಕೀಲಿ_DTypep(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ಆದರೆ (newp) {
            newp->protect(ಸುಳ್ಳು);
            newp->did_ಅಗಲ(ನಿಜ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ವಿಧಾನ_Call_Assoc_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(Ast_ವಿಧಾನ_Call* nodep, AstAssocArrayDType* adtypep) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", ಸೂಚ್ಯಂಕ_exprp, CONTEXT_DET, FINAL, adtypep->ಕೀಲಿ_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(ಸೂಚ್ಯಂಕ_exprp);  // May have been edited
        ಹಿಂತಿರುಗಿ nodep->argsp()->exprp();
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ವಿಧಾನ_Call_Wildcard_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(Ast_ವಿಧಾನ_Call* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", ಸೂಚ್ಯಂಕ_exprp, CONTEXT_DET, FINAL, adtypep->ಕಂಡುಹಿಡಿ_String_DType(),
                     EXTEND_EXP);
        VL_DANGLING(ಸೂಚ್ಯಂಕ_exprp);  // May have been edited
        ಹಿಂತಿರುಗಿ nodep->argsp()->exprp();
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_LValue_Recurse(Ast_ವಿಧಾನ_Call* nodep, Ast_ಗಂಟು* childp, ಸ್ಥಿರಾಂಕ VAccess& access) {
        ಆದರೆ (AstCMethodHard* ಸ್ಥಿರಾಂಕ ichildp = VN_CAST(childp, CMethodHard)) {
            ತಾರ್ಕಿಕ methAt = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ meth_At_ಹಿಂಭಾಗ = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ methAtWrite = ಸುಳ್ಳು;
            ಆಯ್ಕೆ (ichildp->ವಿಧಾನ()) {
            ಸ್ಥಿತಿ VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = ನಿಜ;
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ VCMethod::ARRAY_AT_ಹಿಂಭಾಗ:  // FALLTHRU
                meth_At_ಹಿಂಭಾಗ = ನಿಜ;
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            ಸ್ಥಿತಿ VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            ಸ್ಥಿತಿ VCMethod::DYN_AT_WRITE_APPEND_ಹಿಂಭಾಗ:  //
                methAtWrite = ನಿಜ;
                ನಿಲ್ಲಿಸು;
            ಪೂರ್ವನಿಯೋಜಿತ: ನಿಲ್ಲಿಸು;
            }
            ಆದರೆ (methAt || meth_At_ಹಿಂಭಾಗ || methAtWrite) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                ಆದರೆ (VN_IS(fromDtypep, ಸರತಿ_DType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    ಆದರೆ (methAt) {
                        ichildp->ವಿಧಾನ(VCMethod::ARRAY_AT_WRITE);
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (meth_At_ಹಿಂಭಾಗ) {
                        ichildp->ವಿಧಾನ(VCMethod::DYN_AT_WRITE_APPEND_ಹಿಂಭಾಗ);
                    }
                }
                ವಿಧಾನ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
                ಹಿಂತಿರುಗಿ;
            }
        }
        ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(childp, ಗಂಟು_Var_Ref)) {
            varrefp->access(access);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ ichildp = VN_CAST(childp, MemberSel)) {
            ವಿಧಾನ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ ichildp = VN_CAST(childp, StructSel)) {
            ವಿಧಾನ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ ichildp = VN_CAST(childp, ಗಂಟು_Sel)) {
            ವಿಧಾನ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } ಇಲ್ಲದಿದ್ದರೆ {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* ವಿಧಾನ_Call_Array(Ast_ವಿಧಾನ_Call* nodep, Ast_ಗಂಟು_DType* adtypep) {
        AstCMethodHard* newp = ಶೂನ್ಯಸೂಚಕ;

        ಆದರೆ (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = ವಿಧಾನ_With_Clause(nodep, ಸುಳ್ಳು, ನಿಜ, ಶೂನ್ಯಸೂಚಕ, nodep->ಕಂಡುಹಿಡಿ_UInt32_DType(),
                                         adtypep->subDTypep());
            }
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->withp(withp);
            newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ಸ್ಥಿರಾಂಕ withp = ವಿಧಾನ_With_Clause(
                nodep, ಸುಳ್ಳು, ನಿಜ, ಶೂನ್ಯಸೂಚಕ, nodep->ಕಂಡುಹಿಡಿ_UInt32_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->withp(withp);
            ಆದರೆ (nodep->name() == "unique_index") {
                newp->dtypep(newp->ಕಂಡುಹಿಡಿ_ಸರತಿ_ಸೂಚ್ಯಂಕ_DType());
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(adtypep->subDTypep()));
            }
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ಸ್ಥಿರಾಂಕ withp
                = ವಿಧಾನ_With_Clause(nodep, ನಿಜ, ಸುಳ್ಳು, nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(),
                                   nodep->ಕಂಡುಹಿಡಿ_UInt32_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(adtypep->subDTypep()));
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ಸ್ಥಿರಾಂಕ withp
                = ವಿಧಾನ_With_Clause(nodep, ನಿಜ, ಸುಳ್ಳು, nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(),
                                   nodep->ಕಂಡುಹಿಡಿ_UInt32_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->ಕಂಡುಹಿಡಿ_ಸರತಿ_ಸೂಚ್ಯಂಕ_DType());
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ಸ್ಥಿರಾಂಕ withp
                = ವಿಧಾನ_With_Clause(nodep, ನಿಜ, ಸುಳ್ಳು, adtypep->subDTypep(),
                                   nodep->ಕಂಡುಹಿಡಿ_UInt32_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        }
        ಹಿಂತಿರುಗಿ newp;
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Dyn(Ast_ವಿಧಾನ_Call* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->name() == "at") {  // Created internally for []
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "atWrite") {  // Created internally for []
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "size") {
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::DYN_ಗಾತ್ರ};
            newp->dtype_ಗಣ_Int();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "delete") {  // function void delete()
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::DYN_CLEAR};
            newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ಸ್ಥಿರಾಂಕ withp
                = ವಿಧಾನ_With_Clause(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, adtypep->subDTypep(),
                                   nodep->ಕಂಡುಹಿಡಿ_UInt32_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((newp = ವಿಧಾನ_Call_Array(nodep, adtypep))) {
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ಆದರೆ (newp) {
            newp->protect(ಸುಳ್ಳು);
            newp->did_ಅಗಲ(ನಿಜ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_ಸರತಿ(Ast_ವಿಧಾನ_Call* nodep, Ast_ಸರತಿ_DType* adtypep) {
        AstCMethodHard* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::DYN_ಗಾತ್ರ};
            newp->dtype_ಗಣ_Int();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "delete") {  // function void delete([input integer index])
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 1);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            ಆದರೆ (!nodep->argsp()) {
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          VCMethod::DYN_CLEAR};
                newp->dtype_ಗಣ_Void();
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = ವಿಧಾನ_Call_ಸರತಿ_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(nodep);
                ಆದರೆ (ಸೂಚ್ಯಂಕ_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                              VCMethod::DYN_ತೆಗೆ_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtype_ಗಣ_Void();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                              VCMethod::DYN_ERASE, ಸೂಚ್ಯಂಕ_exprp->unlink_Fr_ಹಿಂಭಾಗ()};
                    newp->dtype_ಗಣ_Void();
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "insert") {
            ವಿಧಾನ_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = ವಿಧಾನ_Call_ಸರತಿ_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(nodep);
            AstArg* ಸ್ಥಿರಾಂಕ argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            ಆದರೆ (ಸೂಚ್ಯಂಕ_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          VCMethod::DYN_ತಳ್ಳು_FRONT, argp->exprp()->unlink_Fr_ಹಿಂಭಾಗ()};
                newp->dtype_ಗಣ_Void();
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          VCMethod::DYN_ಸೇರಿಸು, ಸೂಚ್ಯಂಕ_exprp->unlink_Fr_ಹಿಂಭಾಗ()};
                newp->ಕೂಡಿಸು_Pinsp(argp->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
                newp->dtype_ಗಣ_Void();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "push_back" || nodep->name() == "push_front") {
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), adtypep->subDTypep(), BOTH);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ಸ್ಥಿರಾಂಕ argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ(nodep->name()),
                                      argp->exprp()->unlink_Fr_ಹಿಂಭಾಗ()};
            newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ಸ್ಥಿರಾಂಕ withp
                = ವಿಧಾನ_With_Clause(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, adtypep->subDTypep(),
                                   nodep->ಕಂಡುಹಿಡಿ_UInt32_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                      VCMethod::array_ವಿಧಾನ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((newp = ವಿಧಾನ_Call_Array(nodep, adtypep))) {
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ಆದರೆ (newp) {
            newp->protect(ಸುಳ್ಳು);
            newp->did_ಅಗಲ(ನಿಜ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ವಿಧಾನ_Call_ಸರತಿ_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(Ast_ವಿಧಾನ_Call* nodep) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", ಸೂಚ್ಯಂಕ_exprp, BOTH);
        VL_DANGLING(ಸೂಚ್ಯಂಕ_exprp);  // May have been edited
        ಹಿಂತಿರುಗಿ nodep->argsp()->exprp();
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Warn_Timing(Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ nodep, ಸ್ಥಿರಾಂಕ std::string& className) {
        ಆದರೆ (v3Global.opt.timing().is_ಗಣ_False()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!v3Global.opt.timing().is_ಗಣ_True()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Iface_Ref(Ast_ವಿಧಾನ_Call* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ಸ್ಥಿರಾಂಕ ifacep = adtypep->iface_ಮೂಲಕ_Cellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp
            = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(ifacep, nodep->name()), ಗಂಟು_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            ಆದರೆ (!modport_Allows_ಅಂಶ(nodep, adtypep, ftaskp)) {
                nodep->dtype_ಗಣ_Void();
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (adtypep->isVirtual()) {
                ಗಾಗಿ (Ast_ಗಂಟು* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, ಶೂನ್ಯಸೂಚಕ);
            ಆದರೆ (ftaskp->isStatic()) {
                AstArg* ಸ್ಥಿರಾಂಕ argsp = nodep->argsp();
                ಆದರೆ (argsp) argsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                Ast_ಗಂಟು_FTask_Ref* newp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (VN_IS(ftaskp, Task)) {
                    newp = ಹೊಸ AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newp = ಹೊಸ AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                ಆದರೆ (VN_IS(ftaskp, Task)) nodep->dtype_ಗಣ_Void();
                process_FTask_Ref_ವಾದಗಳು(nodep);
            }
            ಹಿಂತಿರುಗಿ;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    ಶೂನ್ಯ handle_Randomize_ವಾದಗಳು(Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ nodep, AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ತಾರ್ಕಿಕ has_Non_Null_ವಾದಗಳು = ಸುಳ್ಳು;
        AstConst* nullp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = ಶೂನ್ಯಸೂಚಕ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = argp->exprp();
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(exprp, Const)) {
                ಆದರೆ (constp->ಅಂಕೆ().isNull()) {
                    nullp = constp;
                    ಮುಂದುವರಿ;
                }
            }
            has_Non_Null_ವಾದಗಳು = ನಿಜ;
            AstVar* fromVarp = ಶೂನ್ಯಸೂಚಕ;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            ಆದರೆ (Ast_ವಿಧಾನ_Call* ವಿಧಾನ_Callp = VN_CAST(nodep, ವಿಧಾನ_Call)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp = ವಿಧಾನ_Callp->fromp();
                ಆಗ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            ಆದರೆ (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), argp);
                ಮುಂದುವರಿ;
            }
            ಆಗ (exprp) {
                ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), argp);
                    }
                    exprp = ಶೂನ್ಯಸೂಚಕ;
                }
                // All variables in the dot hierarchy must be randomizable
                ಆದರೆ (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            ಆದರೆ (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (classp->existsMember([&](ಸ್ಥಿರಾಂಕ AstClass*, ಸ್ಥಿರಾಂಕ AstVar* memberVarp) {
                           ಹಿಂತಿರುಗಿ memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), argp);
            }
        }
        ಆದರೆ (nullp && has_Non_Null_ವಾದಗಳು) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Class(Ast_ವಿಧಾನ_Call* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ಸ್ಥಿರಾಂಕ ಮೊದಲ_classp = adtypep->classp();
        AstWith* withp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = ವಿಧಾನ_With_Clause(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, adtypep->ಕಂಡುಹಿಡಿ_Void_DType(),
                                     adtypep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(), adtypep);
            ಗಾಗಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                ಆದರೆ (argp->exprp()) userIterate(argp->exprp(), ಅಗಲ_VP{SELF, BOTH}.p());
            }
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, ಮೊದಲ_classp);
            handle_Randomize_ವಾದಗಳು(nodep, ಮೊದಲ_classp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "srandom") {
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, ಮೊದಲ_classp);
        }
        UASSERT_OBJ(ಮೊದಲ_classp, nodep, "Unlinked");
        ಗಾಗಿ (AstClass* classp = ಮೊದಲ_classp; classp;) {
            ಆದರೆ (nodep->fileline()->timingOn()) {
                ಆದರೆ (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ಸ್ಥಿರಾಂಕ packagep = get_ಅಂಶ_Package(classp);
                    // Check if it's std
                    ಆದರೆ (packagep && packagep->name() == "std") {
                        ಆದರೆ (classp->name() == "process") {
                            ವಿಧಾನ_Call_Warn_Timing(nodep, "process");
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (classp->name() == "semaphore" && nodep->name() == "get") {
                            ವಿಧಾನ_Call_Warn_Timing(nodep, "semaphore");
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            ವಿಧಾನ_Call_Warn_Timing(nodep, "mailbox");
                        }
                    }
                }
            }
            ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp
                = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, nodep->name()), ಗಂಟು_FTask)) {
                userIterate(ftaskp, ಶೂನ್ಯಸೂಚಕ);
                ಆದರೆ (ftaskp->isStatic()) {
                    AstArg* ಸ್ಥಿರಾಂಕ argsp = nodep->argsp();
                    ಆದರೆ (argsp) argsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                    Ast_ಗಂಟು_FTask_Ref* newp = ಶೂನ್ಯಸೂಚಕ;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    ಆದರೆ (!m_vup) {  // Called as task
                        newp = ಹೊಸ AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        newp = ಹೊಸ AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    ಆದರೆ (VN_IS(ftaskp, Task)) {
                        ಆದರೆ (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtype_ಗಣ_Void();
                    }
                    ಆದರೆ (withp) nodep->withp(withp);
                    process_FTask_Ref_ವಾದಗಳು(nodep);
                }
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "get_randstate") {
                ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
                ಮೊದಲ_classp->ತಳ_Most_Classp()->needRNG(ನಿಜ);
                v3Global.useRandomizeMethods(ನಿಜ);
                AstCMethodHard* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                         VCMethod::RNG_GET_RANDSTATE, ಶೂನ್ಯಸೂಚಕ};
                newp->usePtr(ನಿಜ);
                newp->dtype_ಗಣ_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "set_randstate") {
                ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->argsp()->exprp();
                ಮೊದಲ_classp->ತಳ_Most_Classp()->needRNG(ನಿಜ);
                v3Global.useRandomizeMethods(ನಿಜ);
                AstCMethodHard* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                         VCMethod::RNG_ಗಣ_RANDSTATE, exprp->unlink_Fr_ಹಿಂಭಾಗ()};
                newp->usePtr(ನಿಜ);
                newp->dtype_ಗಣ_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(ನಿಜ);
                nodep->dtypep(nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ;
        }
        {
            VSpellCheck speller;
            ಗಾಗಿ (AstClass* classp = ಮೊದಲ_classp; classp;) {
                ಗಾಗಿ (Ast_ಗಂಟು* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    ಆದರೆ (VN_IS(itemp, ಗಂಟು_FTask)) speller.ತಳ್ಳು_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ;
            }
            ಸ್ಥಿರಾಂಕ string suggest = speller.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << ಮೊದಲ_classp->verilogKwd() << " " << ಮೊದಲ_classp->prettyNameQ()
                           << "\n"
                           << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->fileline()->warn_ಇನ್ನಷ್ಟು() + suggest));
        }
        nodep->dtype_ಗಣ_Int();  // Guess on error
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Constraint(Ast_ವಿಧಾನ_Call* nodep, AstConstraintRefDType*) {
        ಆದರೆ (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 1);
            ಆದರೆ (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
                nodep->dtypep(nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::INT));
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->dtype_ಗಣ_Void();
            }
            v3Global.useRandomizeMethods(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Rand_ಬಹುಲಕ(Ast_ವಿಧಾನ_Call* nodep) {
        ವಿಧಾನ_Ok_Arguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        ಆದರೆ (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            nodep->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->dtypep(nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(ನಿಜ);
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Unpack(Ast_ವಿಧಾನ_Call* nodep, AstUnpackArrayDType* adtypep) {
        ಎಣಿಕೆ : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_ಮೊತ್ತ,
            ARRAY_ಗುಣಲಬ್ಧ
        } ವಿಧಾನ_Id;

        ವಿಧಾನ_Id = UNKNOWN;
        ಆದರೆ (nodep->name() == "or") {
            ವಿಧಾನ_Id = ARRAY_OR;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "and") {
            ವಿಧಾನ_Id = ARRAY_AND;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "xor") {
            ವಿಧಾನ_Id = ARRAY_XOR;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "sum") {
            ವಿಧಾನ_Id = ARRAY_ಮೊತ್ತ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "product") {
            ವಿಧಾನ_Id = ARRAY_ಗುಣಲಬ್ಧ;
        }

        ಆದರೆ (ವಿಧಾನ_Id) {
            AstWith* ಸ್ಥಿರಾಂಕ withp
                = ವಿಧಾನ_With_Clause(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, adtypep->subDTypep(),
                                   nodep->ಕಂಡುಹಿಡಿ_UInt32_DType(), adtypep->subDTypep());
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            ಆದರೆ (withp) {
                ವಿಧಾನ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                         VCMethod::array_ವಿಧಾನ("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                ಆದರೆ (!nodep->ಮೊದಲ_Abovep()) newp->dtype_ಗಣ_Void();
                newp->protect(ಸುಳ್ಳು);
                newp->did_ಅಗಲ(ನಿಜ);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಶೂನ್ಯಸೂಚಕ;
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ arrayRef = nodep->fromp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ selector = ಹೊಸ AstArraySel{fl, arrayRef, i};
                    ಆದರೆ (!newp) {
                        newp = selector;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಆಯ್ಕೆ (ವಿಧಾನ_Id) {
                        ಸ್ಥಿತಿ ARRAY_OR: newp = ಹೊಸ AstOr{fl, newp, selector}; ನಿಲ್ಲಿಸು;
                        ಸ್ಥಿತಿ ARRAY_AND: newp = ಹೊಸ AstAnd{fl, newp, selector}; ನಿಲ್ಲಿಸು;
                        ಸ್ಥಿತಿ ARRAY_XOR: newp = ಹೊಸ AstXor{fl, newp, selector}; ನಿಲ್ಲಿಸು;
                        ಸ್ಥಿತಿ ARRAY_ಮೊತ್ತ: newp = ಹೊಸ Ast_ಕೂಡಿಸು{fl, newp, selector}; ನಿಲ್ಲಿಸು;
                        ಸ್ಥಿತಿ ARRAY_ಗುಣಲಬ್ಧ: newp = ಹೊಸ AstMul{fl, newp, selector}; ನಿಲ್ಲಿಸು;
                        ಪೂರ್ವನಿಯೋಜಿತ: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCMethodHard* newp = ವಿಧಾನ_Call_Array(nodep, adtypep)) {
            newp->protect(ಸುಳ್ಳು);
            newp->did_ಅಗಲ(ನಿಜ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_Event(Ast_ವಿಧಾನ_Call* nodep, AstBasicDType*) {
        // Method call on event
        ಆದರೆ (nodep->name() == "triggered") {
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtype_ಗಣ_ಬಿಟ್();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ ವಿಧಾನ_Call_String(Ast_ವಿಧಾನ_Call* nodep, AstBasicDType*) {
        // Method call on string
        ಆದರೆ (nodep->name() == "len") {
            // Constant value
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstLenN{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "itoa") {
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "hextoa") {
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "octtoa") {
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "bintoa") {
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "realtoa") {
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "tolower") {
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ Ast_ಗಮ್ಯ_ಅಧೋ_N{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "toupper") {
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ Ast_ಗಮ್ಯ_ಊರ್ಧ್ವ_N{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "compare" || nodep->name() == "icompare") {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ignoreCase = nodep->name()[0] == 'i';
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            AstArg* ಸ್ಥಿರಾಂಕ argp = nodep->argsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhs = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhs = argp->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "putc") {
            ವಿಧಾನ_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", ವಿಧಾನ_Arg(nodep, 1), BOTH);
            AstArg* ಸ್ಥಿರಾಂಕ arg0p = nodep->argsp();
            AstArg* ಸ್ಥಿರಾಂಕ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), VarRef);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = arg0p->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ thsp = arg1p->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstVarRef* ಸ್ಥಿರಾಂಕ varrefp
                = ಹೊಸ AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                nodep->fileline(), fromp, ಹೊಸ AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ತಳ್ಳು_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "getc") {
            ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            AstArg* ಸ್ಥಿರಾಂಕ arg0p = nodep->argsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = arg0p->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "substr") {
            ವಿಧಾನ_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", ವಿಧಾನ_Arg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", ವಿಧಾನ_Arg(nodep, 1), BOTH);
            AstArg* ಸ್ಥಿರಾಂಕ arg0p = nodep->argsp();
            AstArg* ಸ್ಥಿರಾಂಕ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = arg0p->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ thsp = arg1p->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            ಆದರೆ (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstAtoN{nodep->fileline(), nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    Ast_ಸರತಿ_DType* ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ_By(Ast_ಗಂಟು_DType* ಸೂಚ್ಯಂಕ_DTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        ಆದರೆ (Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ queuep = m_ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ[ಸೂಚ್ಯಂಕ_DTypep]) {
            ಹಿಂತಿರುಗಿ queuep;
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ Ast_ಸರತಿ_DType{ಸೂಚ್ಯಂಕ_DTypep->fileline(), ಸೂಚ್ಯಂಕ_DTypep, ಶೂನ್ಯಸೂಚಕ};
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(newp);
            m_ಸರತಿ_DType_ಸೂಚ್ಯಂಕಿತ[ಸೂಚ್ಯಂಕ_DTypep] = newp;
            ಹಿಂತಿರುಗಿ newp;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstNew* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        AstClass* classp = ಶೂನ್ಯಸೂಚಕ;
        ತಾರ್ಕಿಕ ನಿಯೋಜಿಸು_2 = ಸುಳ್ಳು;
        ಆದರೆ (VN_IS(nodep->backp(), ನಿಯೋಜಿಸು)) {  // assignment case
            ನಿಯೋಜಿಸು_2 = ನಿಜ;
            Ast_ಗಂಟು* warnp = ಶೂನ್ಯಸೂಚಕ;
            AstClassRefDType* refp = ಶೂನ್ಯಸೂಚಕ;

            ಆದರೆ (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ಸ್ಥಿರಾಂಕ adtypep
                        = ಹೊಸ AstClassRefDType{nodep->fileline(), clsp, ಶೂನ್ಯಸೂಚಕ};
                    v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(adtypep);
                    refp = adtypep;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : ಶೂನ್ಯಸೂಚಕ;
            }
            ಆದರೆ (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, ಗಂಟು_DType)
                           ? ("data type "s + VN_AS(warnp, ಗಂಟು_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                ಹಿಂತಿರುಗಿ;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp
                = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        process_FTask_Ref_ವಾದಗಳು(nodep);
        ಆದರೆ (!ನಿಯೋಜಿಸು_2) nodep->dtypeFrom(nodep->taskp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNewCopy* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        AstClassRefDType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        ಆದರೆ (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            ಹಿಂತಿರುಗಿ;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
        ಆದರೆ (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNewDynamic* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        AstDynArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        ಆದರೆ (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            ಹಿಂತಿರುಗಿ;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        ಆದರೆ (!VN_IS(nodep->backp(), ಗಂಟು_ನಿಯೋಜಿಸು)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            ಹಿಂತಿರುಗಿ;
        }
        nodep->dtypep(adtypep);
        ಆದರೆ (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        ಆದರೆ (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ಸ್ಥಿರಾಂಕ consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            ಆದರೆ (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlink_Fr_ಹಿಂಭಾಗ());
                nodep->replaceWith(consp->unlink_Fr_ಹಿಂಭಾಗ());
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtype_ಗಣ_ಬಿಟ್();
        userIterateChildren(nodep, m_vup);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Tagged_ನಮೂನೆ* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtype_ಗಣ_ಬಿಟ್();
        userIterateChildren(nodep, m_vup);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಮೂನೆ_Var* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtype_ಗಣ_ಬಿಟ್();
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಮೂನೆ_ನಕ್ಷತ್ರ* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtype_ಗಣ_ಬಿಟ್();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtype_ಗಣ_ಬಿಟ್();
        userIterateChildren(nodep, m_vup);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಮೂನೆ* nodep) override {
        ಆದರೆ (m_ಒಳ_Param_Override && !nodep->ಮಗು_DTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_ಒಳ_Param_Override);
            ಗಾಗಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_ಒಳ_Param_Override = ಸುಳ್ಳು;
                user_Iterate_And_ಮುಂದಿನ(patp->repp(), ಅಗಲ_VP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                ಆದರೆ (!VN_IS(patp->keyp(), ಪಠ್ಯ_2) && !VN_IS(patp->keyp(), ಗಂಟು_DType)) {
                    user_Iterate_And_ಮುಂದಿನ(patp->keyp(), ಅಗಲ_VP{SELF, BOTH}.p());
                }
                ಗಾಗಿ (Ast_ಗಂಟು *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_ಒಳ_Param_Override = VN_IS(valuep, ನಮೂನೆ);
                    userIterate(valuep, ಅಗಲ_VP{SELF, BOTH}.p());
                }
            }
            ಹಿಂತಿರುಗಿ;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_ಒಳ_Param_Override);
        m_ಒಳ_Param_Override = ಸುಳ್ಳು;
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        UINFO(9, "PATTERN " << nodep);
        ಆದರೆ (nodep->ಮಗು_DTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        ಆದರೆ (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_ಗಂಟು_DType* dtypep = nodep->dtypep();
        ಆದರೆ (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            ಆದರೆ (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                ಹಿಂತಿರುಗಿ;
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            ಆದರೆ (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                ಹಿಂತಿರುಗಿ;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            ಗಾಗಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ times = ಭೇಟಿ_Pat_Member_Rep(patp);
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1; i < times; i++) {
                    AstPatMember* ಸ್ಥಿರಾಂಕ newp = patp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    patp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            ಗಾಗಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ಆದರೆ (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
                    ಆಗ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ movep = VN_AS(patp->lhssp()->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                        movep->unlink_Fr_ಹಿಂಭಾಗ();  // Not unlinkFrBackWithNext, just one
                        Ast_ಗಂಟು* newkeyp = ಶೂನ್ಯಸೂಚಕ;
                        ಆದರೆ (patp->keyp()) newkeyp = patp->keyp()->clone_ವೃಕ್ಷ(ನಿಜ);
                        AstPatMember* ಸ್ಥಿರಾಂಕ newp
                            = ಹೊಸ AstPatMember{patp->fileline(), movep, newkeyp, ಶೂನ್ಯಸೂಚಕ};
                        patp->ಕೂಡಿಸು_ಮುಂದಿನ(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ಸ್ಥಿರಾಂಕ nextp = VN_AS(patp->nextp(), PatMember);
                ಆದರೆ (patp->isDefault()) {
                    ಆದರೆ (defaultp) {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlink_Fr_ಹಿಂಭಾಗ();
                }
                patp = nextp;
            }
            ಆಗ (ಸ್ಥಿರಾಂಕ AstConstDType* ಸ್ಥಿರಾಂಕ vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, ಅಗಲ_VP{SELF, BOTH}.p());

            ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ vdtypep = VN_CAST(dtypep, StructDType)) {
                ನಮೂನೆ_Struct(nodep, vdtypep, defaultp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ vdtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                ನಮೂನೆ_Array(nodep, vdtypep, defaultp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsAssoc{nodep->fileline()};
                ನಮೂನೆ_Keyed<Ast_Cons_Assoc_ಅಂಶ>(nodep, consp, dtypep, defaultp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsWildcard{nodep->fileline()};
                ನಮೂನೆ_Keyed<Ast_Cons_Wildcard_ಅಂಶ>(nodep, consp, dtypep, defaultp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, ಸರತಿ_DType)) {
                ನಮೂನೆ_Dyn_Array_Or_ಸರತಿ(nodep, dtypep, defaultp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                ನಮೂನೆ_Basic(nodep, dtypep, defaultp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಆದರೆ (defaultp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(defaultp), defaultp);
        }
    }
    ಶೂನ್ಯ ನಮೂನೆ_Struct(Ast_ನಮೂನೆ* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        ಬಳಸಿ PatMap = std::map<ಸ್ಥಿರಾಂಕ AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ಸ್ಥಿರಾಂಕ AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            ಆಗ (patp) {
                ಮಾಡು {
                    ಆದರೆ (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಪಠ್ಯ* textp = VN_CAST(patp->keyp(), ಪಠ್ಯ_2)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(vdtypep, textp->ಪಠ್ಯ()),
                                           MemberDType);
                            ಆದರೆ (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->ಪಠ್ಯ() << "' not found as member");
                                ನಿಲ್ಲಿಸು;
                            } ಇಲ್ಲದಿದ್ದರೆ {
                                ಸ್ಥಿರಾಂಕ std::pair<PatMap::iterator, ತಾರ್ಕಿಕ> ret
                                    = patmap.emplace(memp, patp);
                                ಆದರೆ (!ret.ಎರಡನೇ) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), ಪಠ್ಯ_2)->ಪಠ್ಯ());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_DType* nodedtypep = VN_CAST(patp->keyp(), ಗಂಟು_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, ಅಗಲ_VP{SELF, BOTH}.p());
                            ಸ್ಥಿರಾಂಕ string dtype = nodedtypep->dtypep()->prettyDTypeName(ನಿಜ);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (memp) {
                        // constant expr
                        ಸ್ಥಿರಾಂಕ std::pair<PatMap::iterator, ತಾರ್ಕಿಕ> ret = patmap.emplace(memp, patp);
                        ಆದರೆ (!ret.ಎರಡನೇ) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), ಪಠ್ಯ_2)->ಪಠ್ಯ());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } ಆಗ (ಸುಳ್ಳು);

                //ಮುಂದಿನ
                ಆದರೆ (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (vdtypep->packed()) {
            ಗಾಗಿ (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = patmap.ಕಂಡುಹಿಡಿ(memp);
                ಆದರೆ (it == patmap.ಕೊನೆ()) {  // default or default_type assignment
                    ಆದರೆ (AstStructDType* ಸ್ಥಿರಾಂಕ memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalue_Concat_ನಮೂನೆ_Struct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        AstPatMember* ಸ್ಥಿರಾಂಕ patp
                            = default_Patp_ನಮೂನೆ_Struct(nodep, memp, defaultp, dtypemap);
                        newp = ಮೌಲ್ಯ_Concat_ನಮೂನೆ_Struct(patp, newp, memp, nodep);
                        ಆದರೆ (patp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(patp), patp);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {  // member assignment
                    newp = ಮೌಲ್ಯ_Concat_ನಮೂನೆ_Struct(it->ಎರಡನೇ, newp, memp, nodep);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {  // Unpacked
            AstConsStruct* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsStruct{nodep->fileline(), vdtypep};
            ಗಾಗಿ (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = patmap.ಕಂಡುಹಿಡಿ(memp);
                AstPatMember* patp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (it == patmap.ಕೊನೆ()) {  // Default or default_type assignment
                    patp = default_Patp_ನಮೂನೆ_Struct(nodep, memp, defaultp, dtypemap);
                    ತಳ್ಳು_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ತಳ್ಳು_Deletep(patp);  // patp used below
                } ಇಲ್ಲದಿದ್ದರೆ {
                    patp = it->ಎರಡನೇ;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = ನಮೂನೆ_Member_ಮೌಲ್ಯ_Iterate(patp);
                consp->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Cons_Struct_ಅಂಶ{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        ಆದರೆ (newp) {
            nodep->replaceWith(newp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nestedvalue_Concat_ನಮೂನೆ_Struct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp,
                                                 Ast_ನಮೂನೆ* nodep, ಸ್ಥಿರಾಂಕ DTypeMap& dtypemap) {
        ಗಾಗಿ (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            ಆದರೆ (AstStructDType* ಸ್ಥಿರಾಂಕ memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalue_Concat_ನಮೂನೆ_Struct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstPatMember* ಸ್ಥಿರಾಂಕ patp
                    = default_Patp_ನಮೂನೆ_Struct(nodep, memp_nested, defaultp, dtypemap);
                newp = ಮೌಲ್ಯ_Concat_ನಮೂನೆ_Struct(patp, newp, memp_nested, nodep);
                ಆದರೆ (patp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(patp), patp);
            }
        }
        ಹಿಂತಿರುಗಿ newp;
    }

    AstPatMember* default_Patp_ನಮೂನೆ_Struct(Ast_ನಮೂನೆ* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ಸ್ಥಿರಾಂಕ DTypeMap& dtypemap) {
        ಸ್ಥಿರಾಂಕ string memp_DType = memp->virtRefDTypep()->prettyDTypeName(ನಿಜ);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = dtypemap.ಕಂಡುಹಿಡಿ(memp_DType);
        ಆದರೆ (it != dtypemap.ಕೊನೆ()) {
            // default_value for data_type
            ಹಿಂತಿರುಗಿ it->ಎರಡನೇ->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }
        ಆದರೆ (defaultp) {
            // default_value for any unmatched member yet
            ಹಿಂತಿರುಗಿ defaultp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }
        ಆದರೆ (memp->valuep()) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ),
                                    ಹೊಸ Ast_ಪಠ್ಯ{nodep->fileline(), memp->name()}, ಶೂನ್ಯಸೂಚಕ};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮೌಲ್ಯ_Concat_ನಮೂನೆ_Struct(AstPatMember* patp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp,
                                           AstMemberDType* memp, Ast_ನಮೂನೆ* nodep) {
        ಆದರೆ (patp) {
            patp->dtypep(memp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = ನಮೂನೆ_Member_ಮೌಲ್ಯ_Iterate(patp);
            ಆದರೆ (!newp) {
                newp = valuep;
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstConcat* ಸ್ಥಿರಾಂಕ concatp = ಹೊಸ AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::WIDTHCONCAT, ನಿಜ);
                valuep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::WIDTHCONCAT, ನಿಜ);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        ಹಿಂತಿರುಗಿ newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_ಗಂಟು_DType* elemDTypep,
                                       ಸ್ಥಿರಾಂಕ DTypeMap& dtypemap = DTypeMap{}) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = defaultp->lhssp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDTypeSkipRefp = elemDTypep->skipRefp();
        ಸ್ಥಿರಾಂಕ AstStructDType* ಸ್ಥಿರಾಂಕ structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ unpackedStruct = structp && !structp->packed();

        ಆದರೆ (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            ಹಿಂತಿರುಗಿ defaultp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }
        ಆದರೆ (VN_IS(valuep, ನಮೂನೆ)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            ಹಿಂತಿರುಗಿ defaultp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }
        ಆದರೆ (!valuep->dtypep()) userIterate(valuep, ಅಗಲ_VP{SELF, BOTH}.p());
        ತಾರ್ಕಿಕ ಸಂಪೂರ್ಣ_Element = unpackedStruct;
        ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            ಸಂಪೂರ್ಣ_Element = unpackedStruct ? !ಮೌಲ್ಯ_DTypep->skipRefp()->isIntegralOrPacked()
                                          : ಮೌಲ್ಯ_DTypep->similarDType(elemDTypeSkipRefp);
        }
        ಆದರೆ (ಸಂಪೂರ್ಣ_Element) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            ಹಿಂತಿರುಗಿ defaultp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }

        AstPatMember* ಸ್ಥಿರಾಂಕ nestedDefaultp
            = ಹೊಸ AstPatMember{defaultp->fileline(), valuep, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        nestedDefaultp->isDefault(ನಿಜ);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : dtypemap) nestedDefaultp->ಕೂಡಿಸು_ಮುಂದಿನ(ನಮೂದು.ಎರಡನೇ->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        Ast_ನಮೂನೆ* ಸ್ಥಿರಾಂಕ recursivePatternp = ಹೊಸ Ast_ನಮೂನೆ{defaultp->fileline(), nestedDefaultp};
        ಹಿಂತಿರುಗಿ ಹೊಸ AstPatMember{defaultp->fileline(), recursivePatternp, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ನಮೂನೆ_Default_Valuep(AstPatMember* defaultp, Ast_ಗಂಟು_DType* elemDTypep) {
        ಆದರೆ (!defaultp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstPatMember* ಸ್ಥಿರಾಂಕ elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = ನಮೂನೆ_Member_ಮೌಲ್ಯ_Iterate(elemDefaultp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(elemDefaultp), elemDefaultp);
        ಹಿಂತಿರುಗಿ valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    ಶೂನ್ಯ ನಮೂನೆ_Concat_Array(Ast_ನಮೂನೆ* nodep, AstUnpackArrayDType* arrayDtp) {
        ಸ್ಥಿರಾಂಕ VNumRange range = arrayDtp->declRange();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elements = range.elements();
        AstConsArray* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConsArray{nodep->fileline(), arrayDtp};
        ಪೂರ್ಣಾಂಕ entn = 0;  // Number of elements given so far, from the left
        ತಾರ್ಕಿಕ error = ಸುಳ್ಳು;  // Reported an error, so the value is not complete
        ಗಾಗಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = ನಮೂನೆ_Member_ಮೌಲ್ಯ_Iterate(patp);
            ಸ್ಥಿರಾಂಕ uint64_t ಸೂಚ್ಯಂಕ = range.ascending() ? entn : elements - 1 - entn;
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ಆದರೆ (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_ಮೌಲ್ಯಗಳು(ಸೂಚ್ಯಂಕ, valuep);
                ಮುಂದುವರಿ;
            }
            ಸ್ಥಿರಾಂಕ VCastable castable = Ast_ಗಂಟು::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            ಆದರೆ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = ನಿಜ;
            }
            newp->valuep(ಸೂಚ್ಯಂಕ, valuep);
            ++entn;
        }
        ಆದರೆ (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = ನಿಜ;
        }
        ಆದರೆ (error) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(newp), newp);
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->replaceWith(newp);
        }
    }
    ಶೂನ್ಯ ನಮೂನೆ_Array(Ast_ನಮೂನೆ* nodep, Ast_ಗಂಟು_Array_DType* arrayDtp, AstPatMember* defaultp) {
        ಆದರೆ (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            ನಮೂನೆ_Concat_Array(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = pat_ಸದಿಶ_Map(nodep, range);
        UINFO(9, "ent " << range.ಎಡ() << " to " << range.ಬಲ());
        Ast_ಗಂಟು* newp = ಶೂನ್ಯಸೂಚಕ;
        ತಾರ್ಕಿಕ missed = ಸುಳ್ಳು;  // Some element missed, so not a complete value
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ entn = 0, ent = range.ಎಡ(); entn < range.elements();
             ++entn, ent += range.ಎಡ_ಗಮ್ಯ_ಬಲ_Inc()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = patmap.ಕಂಡುಹಿಡಿ(ent);
            ಆದರೆ (it == patmap.ಕೊನೆ()) {
                ಆದರೆ (defaultp) {
                    valuep = ನಮೂನೆ_Default_Valuep(defaultp, arrayDtp->subDTypep());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = ನಿಜ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstPatMember* ಸ್ಥಿರಾಂಕ patp = it->ಎರಡನೇ;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = ನಮೂನೆ_Member_ಮೌಲ್ಯ_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                ಆದರೆ (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warn_ಇನ್ನಷ್ಟು()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                    valuep = ಹೊಸ AstConst{patp->fileline(), AstConst::ಬಿಟ್_False_Erroring{}};
                }
            }

            ಆದರೆ (valuep) {
                ಆದರೆ (VN_IS(arrayDtp, UnpackArrayDType)) {
                    ಆದರೆ (!newp) newp = ಹೊಸ AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } ಇಲ್ಲದಿದ್ದರೆ {  // Packed. Convert to concat for now.
                    ಆದರೆ (!newp) {
                        newp = valuep;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        AstConcat* ಸ್ಥಿರಾಂಕ concatp
                            = ಹೊಸ AstConcat{valuep->fileline(), VN_AS(newp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        ಆದರೆ (!patmap.ಖಾಲಿಗಳು()) nodep->v3error("Assignment pattern with too many elements");
        ಆದರೆ (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(newp), newp);
            newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}};
        }
        ಆದರೆ (newp) {
            nodep->replaceWith(newp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* ನಮೂನೆ_Const_Keyp(AstPatMember* patp) {
        ಆದರೆ (VN_IS(patp->keyp(), ಗಂಟು_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (patp->ಕೀಲಿ_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->ಕೀಲಿ_Exprp());  // May change
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(patp->ಕೀಲಿ_Exprp(), Const);
        ಆದರೆ (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        ಹಿಂತಿರುಗಿ constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* ನಮೂನೆ_Scalar_Keyp(AstPatMember* patp, Ast_ಗಂಟು_DType* ಕೀಲಿ_DTypep) {
        ಆದರೆ (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        ಆದರೆ (ಕೀಲಿ_DTypep->skipRefp()->isIntegralOrPacked() && patp->ಕೀಲಿ_Exprp()) {
            userIterate(patp->ಕೀಲಿ_Exprp(), ಅಗಲ_VP{ಕೀಲಿ_DTypep, PRELIM}.p());
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = std::max(patp->ಕೀಲಿ_Exprp()->ಅಗಲ(), ಕೀಲಿ_DTypep->ಅಗಲ());
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ contextDTypep
                = ಕೀಲಿ_DTypep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(ಅಗಲ, ಅಗಲ, ಕೀಲಿ_DTypep->numeric(), ನಿಜ);
            iterateCheck(patp, "key", patp->ಕೀಲಿ_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, ಸುಳ್ಳು);
        }
        AstConst* ಸ್ಥಿರಾಂಕ constp = ನಮೂನೆ_Const_Keyp(patp);
        ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (constp->ಅಂಕೆ().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, ಕೀಲಿ_DTypep, EXTEND_EXP, ಸುಳ್ಳು);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ convp = patp->ಕೀಲಿ_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        ಹಿಂತಿರುಗಿ VN_AS(keyp->unlink_Fr_ಹಿಂಭಾಗ(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ನಮೂನೆ_Keyp(AstConsAssoc*, AstPatMember* patp, Ast_ಗಂಟು_DType* arrayDtp) {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಕೀಲಿ_DTypep = VN_AS(arrayDtp, AssocArrayDType)->ಕೀಲಿ_DTypep();
        ಆದರೆ (!ಕೀಲಿ_DTypep->skipRefp()->isAggregateType() || !patp->ಕೀಲಿ_Exprp()) {
            ಹಿಂತಿರುಗಿ ನಮೂನೆ_Scalar_Keyp(patp, ಕೀಲಿ_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->ಕೀಲಿ_Exprp(), ಕೀಲಿ_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->ಕೀಲಿ_Exprp());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ foldedp = patp->ಕೀಲಿ_Exprp();
        ಆದರೆ (!foldedp->is_Constant_ವೃಕ್ಷ()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (foldedp->exists([](ಸ್ಥಿರಾಂಕ AstConst* constp) { ಹಿಂತಿರುಗಿ constp->ಅಂಕೆ().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ foldedp->unlink_Fr_ಹಿಂಭಾಗ();
    }
    AstConst* ನಮೂನೆ_Keyp(AstConsWildcard*, AstPatMember* patp, Ast_ಗಂಟು_DType* arrayDtp) {
        ಹಿಂತಿರುಗಿ ನಮೂನೆ_Scalar_Keyp(patp, arrayDtp->ಕಂಡುಹಿಡಿ_String_DType());
    }
    AstConst* ನಮೂನೆ_Keyp(AstConsDynArray*, AstPatMember* patp, Ast_ಗಂಟು_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        ಹಿಂತಿರುಗಿ ನಮೂನೆ_Scalar_Keyp(patp, arrayDtp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಅಂಶ, ಬಗೆ_ಹೆಸರು T_Cons>
    ಶೂನ್ಯ ವಿಂಗಡಿಸು_Cons_ವಸ್ತುಗಳು(T_Cons* nodep) {
        std::ಸದಿಶ<T_ಅಂಶ*> itemps;
        ಆಗ (T_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = nodep->itemsp()) {
            itemp->unlink_Fr_ಹಿಂಭಾಗ();
            itemps.ತಳ್ಳು_ಹಿಂಭಾಗ(itemp);
        }
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೀಲಿ_ದ = [](ಸ್ಥಿರಾಂಕ T_ಅಂಶ* itemp) -> ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ keyp = itemp->keyp();
            ಹಿಂತಿರುಗಿ VN_AS(keyp, Const)->ಅಂಕೆ();
        };
        std::stable_ವಿಂಗಡಿಸು(itemps.begin(), itemps.ಕೊನೆ(), [&](ಸ್ಥಿರಾಂಕ T_ಅಂಶ* ap, ಸ್ಥಿರಾಂಕ T_ಅಂಶ* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            ಹಿಂತಿರುಗಿ ಕೀಲಿ_ದ(ap).isLtXZ(ಕೀಲಿ_ದ(bp));
        });
        ಸ್ಥಿರಾಂಕ T_ಅಂಶ* firstp = ಶೂನ್ಯಸೂಚಕ;  // First item with the current key
        ಗಾಗಿ (T_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp : itemps) {
            ಆದರೆ (firstp && ಕೀಲಿ_ದ(firstp).isCaseEq(ಕೀಲಿ_ದ(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } ಇಲ್ಲದಿದ್ದರೆ {
                firstp = itemp;
            }
            nodep->ಕೂಡಿಸು_Itemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಅಂಶ, ಬಗೆ_ಹೆಸರು T_Cons>
    ಶೂನ್ಯ ನಮೂನೆ_Keyed(Ast_ನಮೂನೆ* nodep, T_Cons* consp, Ast_ಗಂಟು_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(ನಮೂನೆ_Default_Valuep(defaultp, arrayDtp->subDTypep()));
        ಗಾಗಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ keyp = ನಮೂನೆ_Keyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = ನಮೂನೆ_Member_ಮೌಲ್ಯ_Iterate(patp);
            ಆದರೆ (keyp) {
                consp->ಕೂಡಿಸು_Itemsp(ಹೊಸ T_ಅಂಶ{patp->fileline(), keyp, valuep});
            } ಇಲ್ಲದಿದ್ದರೆ {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ಸ್ಥಿರಾಂಕ AstAssocArrayDType* ಸ್ಥಿರಾಂಕ assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        ಆದರೆ (!assocDtp || !assocDtp->ಕೀಲಿ_DTypep()->skipRefp()->isAggregateType()) {
            ವಿಂಗಡಿಸು_Cons_ವಸ್ತುಗಳು<T_ಅಂಶ>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_Cons_Dyn_Array_Or_ಸರತಿ(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        ಆದರೆ (VN_IS(dtypep, DynArrayDType)) ಹಿಂತಿರುಗಿ ಹೊಸ AstConsDynArray{fileline};
        ಆದರೆ (VN_IS(dtypep, ಸರತಿ_DType)) ಹಿಂತಿರುಗಿ ಹೊಸ Ast_Cons_ಸರತಿ{fileline};
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಅಂಶ, ಬಗೆ_ಹೆಸರು T_Cons>
    ಶೂನ್ಯ ಕೂಡಿಸು_Cons_ಅಂಶ(T_Cons* consp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isElement
            = Ast_ಗಂಟು::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), ಶೂನ್ಯಸೂಚಕ)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ sub_ಸರತಿ_Dtp = VN_CAST(valuep->dtypep()->skipRefp(), ಸರತಿ_DType);
        ಆದರೆ (!isElement && !(sub_ಸರತಿ_Dtp && sub_ಸರತಿ_Dtp->boundp())) {
            T_Cons* ಸ್ಥಿರಾಂಕ subp = Ast_ಗಂಟು::cast<T_Cons>(valuep);
            ಆದರೆ (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                ಆದರೆ (subp->itemsp()) consp->ಕೂಡಿಸು_Itemsp(subp->itemsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(subp), subp);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (Ast_Cons_ಸರತಿ* ಸ್ಥಿರಾಂಕ queuep = VN_CAST(valuep, Cons_ಸರತಿ)) {
                ಕೂಡಿಸು_Cons_ವಸ್ತುಗಳು(consp, queuep);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (AstConsDynArray* ಸ್ಥಿರಾಂಕ dynp = VN_CAST(valuep, ConsDynArray)) {
                ಕೂಡಿಸು_Cons_ವಸ್ತುಗಳು(consp, dynp);
                ಹಿಂತಿರುಗಿ;
            }
        }
        T_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = ಹೊಸ T_ಅಂಶ{valuep->fileline(), valuep};
        consp->ಕೂಡಿಸು_Itemsp(itemp);
        // Width the value now, as constructors are widthed when created
        ಆದರೆ (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } ಇಲ್ಲದಿದ್ದರೆ {
            user_Iterate_And_ಮುಂದಿನ(valuep, ಅಗಲ_VP{arrayDtp, FINAL}.p());
        }
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Cons_ಅಂಶ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* consp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಆದರೆ (AstConsDynArray* ಸ್ಥಿರಾಂಕ dynp = VN_CAST(consp, ConsDynArray)) {
            ಕೂಡಿಸು_Cons_ಅಂಶ<Ast_Cons_Dyn_Array_ಅಂಶ>(dynp, valuep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_Cons_ಅಂಶ<Ast_Cons_ಸರತಿ_ಅಂಶ>(VN_AS(consp, Cons_ಸರತಿ), valuep);
        }
    }
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Sub>
    ಶೂನ್ಯ ಕೂಡಿಸು_Cons_ವಸ್ತುಗಳು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* consp, T_Sub* subp) {
        ಆಗ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ itemp = subp->itemsp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = itemp->valuep()->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(itemp->unlink_Fr_ಹಿಂಭಾಗ()), itemp);
            ಕೂಡಿಸು_Cons_ಅಂಶ(consp, valuep);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(subp), subp);
    }
    ಶೂನ್ಯ ನಮೂನೆ_Dyn_Array_Or_ಸರತಿ(Ast_ನಮೂನೆ* nodep, Ast_ಗಂಟು_DType* arrayp, AstPatMember* defaultp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = new_Cons_Dyn_Array_Or_ಸರತಿ(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ಸ್ಥಿರಾಂಕ AstPatMember* ಸ್ಥಿರಾಂಕ firstp = VN_AS(nodep->itemsp(), PatMember);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ keyed = defaultp || (firstp && firstp->keyp());
        ಆದರೆ (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            ನಮೂನೆ_Keyed<Ast_Cons_Dyn_Array_ಅಂಶ>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        ಆದರೆ (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        ಗಾಗಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ಆದರೆ (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            ಕೂಡಿಸು_Cons_ಅಂಶ(newp, ನಮೂನೆ_Member_ಮೌಲ್ಯ_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    ಶೂನ್ಯ ನಮೂನೆ_Basic(Ast_ನಮೂನೆ* nodep, Ast_ಗಂಟು_DType* vdtypep, AstPatMember* defaultp) {
        ಸ್ಥಿರಾಂಕ AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ಸ್ಥಿರಾಂಕ VNumRange range = bdtypep->declRange();
        PatVecMap patmap = pat_ಸದಿಶ_Map(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ent : range.seq_Hi_ಗಮ್ಯ_Lo()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = patmap.ಕಂಡುಹಿಡಿ(ent);
            ಆದರೆ (it == patmap.ಕೊನೆ()) {
                ಆದರೆ (defaultp) {
                    valuep = ನಮೂನೆ_Default_Valuep(defaultp, nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstPatMember* ಸ್ಥಿರಾಂಕ patp = it->ಎರಡನೇ;
                patmap.erase(it);
                patp->dtypep(nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType());
                valuep = ನಮೂನೆ_Member_ಮೌಲ್ಯ_Iterate(patp);
            }
            ಆದರೆ (valuep) {  // Packed. Convert to concat for now.
                ಆದರೆ (!newp) {
                    newp = valuep;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    AstConcat* ಸ್ಥಿರಾಂಕ concatp = ಹೊಸ AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        ಆದರೆ (!patmap.ಖಾಲಿಗಳು()) nodep->v3error("Assignment pattern with too many elements");
        ಆದರೆ (newp) {
            nodep->replaceWith(newp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ನಮೂನೆ_Member_ಮೌಲ್ಯ_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, ಅಗಲ_VP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = patp->lhssp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ newp = V3_ಅಗಲ_Commit::new_If_Const_Commit_ಗಾತ್ರ(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        ಹಿಂತಿರುಗಿ valuep;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ checkEventAssignment(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asgnp) {
        string unsupEvtAsgn;
        ಆದರೆ (!ಬಳಸುತ್ತದೆ_Dynamic_Scheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        ಆದರೆ (VN_IS(asgnp->rhsp(), CReset)) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (asgnp->rhsp()->dtypep()->isEvent() && !ಬಳಸುತ್ತದೆ_Dynamic_Scheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.ಖಾಲಿಗಳು() ? "from" : " and from");
        }
        ಆದರೆ (!unsupEvtAsgn.ಖಾಲಿಗಳು()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warn_ಇನ್ನಷ್ಟು()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warn_ಇನ್ನಷ್ಟು()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಬಳಸುತ್ತದೆ_Dynamic_Scheduler(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        ಆಗ (ನಿಜ) {
            AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(nodep, VarRef);
            ಆದರೆ (vrefp) ಹಿಂತಿರುಗಿ ಬಳಸುತ್ತದೆ_Dynamic_Scheduler(vrefp);
            ಆದರೆ (VN_IS(nodep, MemberSel)) {
                ಹಿಂತಿರುಗಿ ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_Sel* selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
                nodep = selp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಬಳಸುತ್ತದೆ_Dynamic_Scheduler(AstVarRef* vrefp) {
        ಹಿಂತಿರುಗಿ VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstPatMember* nodep) override {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, ಅಗಲ_VP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ನಿಯೋಜಿಸು_3, FINAL, vdtypep, EXTEND_LHS);
    }
    ಪೂರ್ಣಾಂಕ ಭೇಟಿ_Pat_Member_Rep(AstPatMember* nodep) {
        uint32_t times = 1;
        ಆದರೆ (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->repp(), Const);
            ಆದರೆ (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } ಇಲ್ಲದಿದ್ದರೆ {
                times = constp->ಗಮ್ಯ_UInt();
            }
            ಆದರೆ (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()
                ->delete_ವೃಕ್ಷ();  // Done with replicate before cloning
        }
        ಹಿಂತಿರುಗಿ times;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_ಗಂಟು* propp = nodep->propp();
            ಆಗ (AstLogNot* ಸ್ಥಿರಾಂಕ notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ಸ್ಥಿರಾಂಕ untilp = VN_CAST(propp, Until);
            ಆದರೆ (untilp && VN_IS(nodep->backp(), ಗಂಟು_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().is_ಗಣ_False() || !v3Global.opt.timing().is_ಗಣ_True())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(ಹೊಸ AstConst{untilp->fileline(), AstConst::ಬಿಟ್_False{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            user_Iterate_And_ಮುಂದಿನ(nodep->sensesp(), ಶೂನ್ಯಸೂಚಕ);
            ಆದರೆ (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }

    ತಾರ್ಕಿಕ ಮೊದಲ_New_Statement_Ok_Recurse(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
            ಆದರೆ (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ aitemp = VN_CAST(nodep, ನಿಯೋಜಿಸು)) {
            ಆದರೆ (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (AstInitialStaticStmt* ಸ್ಥಿರಾಂಕ aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಆದರೆ (!ಮೊದಲ_New_Statement_Ok_Recurse(stmtp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (AstInitialAutomaticStmt* ಸ್ಥಿರಾಂಕ aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಆದರೆ (!ಮೊದಲ_New_Statement_Ok_Recurse(stmtp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    //--------------------
    // Top levels

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Case_ಅಂಶ>
    ಶೂನ್ಯ handleCaseType(Ast_ಗಂಟು* casep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Case_ಅಂಶ* itemsp) {
        Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ exprap = VN_CAST(exprp, Attr_ದ);
        ಆದರೆ (!exprap) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (exprap->attrType() != VAttrType::TYPEID) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << ಅಭಿವ್ಯಕ್ತಿ_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ newfl = casep->fileline();
        newfl->warnOff(V3_Error_ಸಂಕೇತ::CASEINCOMPLETE, ನಿಜ);  // Side effect of transform
        newfl->warnOff(V3_Error_ಸಂಕೇತ::CASEOVERLAP, ನಿಜ);  // Side effect of transform
        casep->fileline(newfl);
        ಗಾಗಿ (Case_ಅಂಶ* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<Case_ಅಂಶ>(itemp->nextp())) {
            ಆದರೆ (itemp->isDefault()) ಮುಂದುವರಿ;
            ತಾರ್ಕಿಕ ದೊರಕಿತು = ಸುಳ್ಳು;
            ಗಾಗಿ (Ast_ಗಂಟು* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ಸ್ಥಿರಾಂಕ Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ condAttrp = VN_CAST(condp, Attr_ದ);
                ಆದರೆ (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ condDtp = condAttrp->dtypep();
                    ಆದರೆ (Ast_ಗಂಟು::computeCastable(ಅಭಿವ್ಯಕ್ತಿ_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        ದೊರಕಿತು = ನಿಜ;
                        ನಿಲ್ಲಿಸು;
                    }
                }
            }
            ತಳ್ಳು_Deletep(itemp->condsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            // Item condition becomes constant 1 if hits else 0
            itemp->ಕೂಡಿಸು_Condsp(ಹೊಸ AstConst{newfl, AstConst::ಬಿಟ್_True{}, ದೊರಕಿತು});
        }
        exprap->replaceWith(ಹೊಸ AstConst{newfl, AstConst::ಬಿಟ್_True{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(exprap), exprap);
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Case_ಅಂಶ>
    ಶೂನ್ಯ handleCase(Ast_ಗಂಟು* casep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Case_ಅಂಶ* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_ಗಂಟು_DType* subDTypep = exprp->dtypep();
        ಗಾಗಿ (Case_ಅಂಶ* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<Case_ಅಂಶ>(itemp->nextp())) {
            ಗಾಗಿ (Ast_ಗಂಟು* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ಆದರೆ (condp->dtypep() == subDTypep) ಮುಂದುವರಿ;

                ಆದರೆ (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->ಕಂಡುಹಿಡಿ_Double_DType();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->ಕಂಡುಹಿಡಿ_String_DType();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = std::max(subDTypep->ಅಗಲ(), condp->ಅಗಲ());
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ mwidth = std::max(subDTypep->ಅಗಲ_Min(), condp->ಅಗಲ_Min());
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                        ಅಗಲ, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        ಗಾಗಿ (Case_ಅಂಶ* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<Case_ಅಂಶ>(itemp->nextp())) {
            ಗಾಗಿ (Ast_ಗಂಟು *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
        ಗಾಗಿ (Ast_Gen_Case_ಅಂಶ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), Gen_Case_ಅಂಶ);
            ಗಾಗಿ (Ast_ಗಂಟು *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_ಮುಂದಿನ(nodep->initsp(), ಶೂನ್ಯಸೂಚಕ);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        user_Iterate_And_ಮುಂದಿನ(nodep->incsp(), ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
        ಗಾಗಿ (Ast_Case_ಅಂಶ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_ಅಂಶ);  // Prelim may cause the node to get replaced
            user_Iterate_And_ಮುಂದಿನ(itemp->stmtsp(), ಶೂನ್ಯಸೂಚಕ);
            ಗಾಗಿ (Ast_ಗಂಟು *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಅಂಶ_DTypep = nodep->ಕಂಡುಹಿಡಿ_UInt32_DType();
        ಗಾಗಿ (Ast_Case_ಅಂಶ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_ಅಂಶ);  // Prelim may cause the node to get replaced
            user_Iterate_And_ಮುಂದಿನ(itemp->stmtsp(), ಶೂನ್ಯಸೂಚಕ);
            ಗಾಗಿ (Ast_ಗಂಟು *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, ಅಂಶ_DTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        user_Iterate_And_ಮುಂದಿನ(nodep->stmtsp(), ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        user_Iterate_And_ಮುಂದಿನ(nodep->thensp(), ಶೂನ್ಯಸೂಚಕ);
        user_Iterate_And_ಮುಂದಿನ(nodep->elsesp(), ಶೂನ್ಯಸೂಚಕ);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->stmtsp(), ಶೂನ್ಯಸೂಚಕ);
        // expected result is same as parent's expected result
        user_Iterate_And_ಮುಂದಿನ(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Foreach* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        nodep->did_ಅಗಲ(ನಿಜ);
        ಸ್ಥಿರಾಂಕ Ast_Foreach_ಶೀರ್ಷಿಕೆ* ಸ್ಥಿರಾಂಕ headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        user_Iterate_And_ಮುಂದಿನ(headerp->fromp(), ಅಗಲ_VP{SELF, BOTH}.p());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_ಗಂಟು_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        ಗಾಗಿ (Ast_ಗಂಟು *argsp = headerp->elementsp(), *ಮುಂದಿನ_argsp; argsp; argsp = ಮುಂದಿನ_argsp) {
            ಮುಂದಿನ_argsp = argsp->nextp();
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಖಾಲಿಗಳು = VN_IS(argsp, ಖಾಲಿಗಳು_2);
            AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || ಖಾಲಿಗಳು, argsp, "Missing foreach loop variable");
            ಆದರೆ (varp) varp->ಬಳಸಿದ_Loop_Idx(ನಿಜ);
            ಆದರೆ (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            ಆದರೆ (VN_IS(fromDtp, ಗಂಟು_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, ಸರತಿ_DType)) {
                // Nothing special here
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, BasicDType)) {
                ಆದರೆ (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAssocArrayDType* ಸ್ಥಿರಾಂಕ adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->ಕೀಲಿ_DTypep());
            } ಇಲ್ಲದಿದ್ದರೆ {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodyp = nodep->bodyp();
        user_Iterate_And_ಮುಂದಿನ(bodyp, ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (AstForeach* ಸ್ಥಿರಾಂಕ loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convert_ಗಮ್ಯ_While(loopp), loopp, nodep);
            ಹಿಂತಿರುಗಿ;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
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
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->ಅಗಲ_Sized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                ಆದರೆ (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            ಹರಿವು_ನಿಯೋಜಿಸು_ಅಧೋ(nodep);
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            ಆದರೆ (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDtp = lhsDTypep->skipRefp();
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                ಆದರೆ (lhsIsUnpackArray && rhsIsUnpackArray) {
                    ಆದರೆ (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_ಗಂಟು_Var_Ref, Ast_ಗಂಟು_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), ಗಂಟು_Var_Ref), VN_CAST(nodep->rhsp(), ಗಂಟು_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                ಆದರೆ (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), ಗಂಟು_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterate_Check_ನಿಯೋಜಿಸು(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        ಆದರೆ (VN_IS(nodep, ನಿಯೋಜಿಸು_Force)) checkForceReleaseLhs(nodep, nodep->lhsp());
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ controlp = nodep->timingControlp()) {
            ಆದರೆ (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ(), controlp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->fileline()->timingOn() && v3Global.opt.timing().is_ಗಣ_True()) {
                iterateNull(controlp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (nodep->fileline()->timingOn()) {
                    ಆದರೆ (v3Global.opt.timing().is_ಗಣ_False()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ(), controlp);
            }
        }
        ಆದರೆ (VN_IS(nodep->rhsp(), ಖಾಲಿಗಳು_ಸರತಿ)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            ಆದರೆ (!VN_IS(lhsDtp, ಸರತಿ_DType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ Ast_ವಿಧಾನ_Call{nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), "delete"};
            newp->dtype_ಗಣ_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // Need to now convert it
            ಭೇಟಿ(newp);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstNewDynamic* ಸ್ಥಿರಾಂಕ dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            ಆದರೆ (!dynp->rhsp()) {
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlink_Fr_ಹಿಂಭಾಗ()};
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlink_Fr_ಹಿಂಭಾಗ()};
                newp->ಕೂಡಿಸು_Pinsp(dynp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ());
            }
            newp->did_ಅಗಲ(ನಿಜ);
            newp->protect(ಸುಳ್ಳು);
            newp->dtype_ಗಣ_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        ಆದರೆ (AstCMethodHard* ಸ್ಥಿರಾಂಕ slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod ನಿಯೋಜಿಸು_ವಿಧಾನ;
            ಆದರೆ (slicep->ವಿಧಾನ() == VCMethod::DYN_ತುಂಡು) {
                ನಿಯೋಜಿಸು_ವಿಧಾನ = VCMethod::DYN_ತುಂಡು_ನಿಯೋಜಿಸು;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (slicep->ವಿಧಾನ() == VCMethod::DYN_ತುಂಡು_ಹಿಂಭಾಗ_ಹಿಂಭಾಗ) {
                ನಿಯೋಜಿಸು_ವಿಧಾನ = VCMethod::DYN_ತುಂಡು_ನಿಯೋಜಿಸು_ಹಿಂಭಾಗ_ಹಿಂಭಾಗ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (slicep->ವಿಧಾನ() == VCMethod::DYN_ತುಂಡು_FRONT_ಹಿಂಭಾಗ) {
                ನಿಯೋಜಿಸು_ವಿಧಾನ = VCMethod::DYN_ತುಂಡು_ನಿಯೋಜಿಸು_FRONT_ಹಿಂಭಾಗ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ನಿಯೋಜಿಸು_ವಿಧಾನ = VCMethod::_ENUM_MAX;  // not a slice
            }
            ಆದರೆ (ನಿಯೋಜಿಸು_ವಿಧಾನ != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = slicep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
                // Collect existing slice index pins (lsb, msb)
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lsbp = slicep->pinsp()->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ msbp = slicep->pinsp()->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                AstCMethodHard* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstCMethodHard{nodep->fileline(), fromp, ನಿಯೋಜಿಸು_ವಿಧಾನ};
                newp->ಕೂಡಿಸು_Pinsp(lsbp);
                newp->ಕೂಡಿಸು_Pinsp(msbp);
                newp->ಕೂಡಿಸು_Pinsp(rhsp);
                newp->did_ಅಗಲ(ನಿಜ);
                newp->protect(ಸುಳ್ಳು);
                newp->dtype_ಗಣ_Void();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.ಗಣ_Assigns_Events();
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRSRule* nodep) override {
        ಆದರೆ (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        user_Iterate_And_ಮುಂದಿನ(nodep->prodlistsp(), ಶೂನ್ಯಸೂಚಕ);
        user_Iterate_And_ಮುಂದಿನ(nodep->ತೂಕ_Stmtsp(), ಶೂನ್ಯಸೂಚಕ);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRelease* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->ಅಗಲ_Sized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDeassign* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->ಅಗಲ_Sized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isFormatNonNumericArg(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಹಿಂತಿರುಗಿ dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, ಸರತಿ_DType)  //
               || (VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType)
                   && !VN_AS(dtypep, ಗಂಟು_UOr_Struct_DType)->packed());
    }

    ಶೂನ್ಯ checkFormatNumericArg(Ast_ಗಂಟು* argp, ಅಕ್ಷರ ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        ಆದರೆ (std::tolower(ch) == 'd') ಹಿಂತಿರುಗಿ;
        // In contrast, %x of a class reference, causes errors on several simulators.
        ಆದರೆ (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warn_ಇನ್ನಷ್ಟು() << "... Suggest use '%s'");
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_ಮುಂದಿನ(nodep->fmtp(), ಅಗಲ_VP{SELF, BOTH}.p());
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಮ್ಯ_String_N* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtype_ಗಣ_String();
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        ಆದರೆ (m_vup && !m_vup->prelim()) ಹಿಂತಿರುಗಿ;  // Can be called as statement or function
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;
        //
        UINFO(9, "  Display in " << nodep->ಪಠ್ಯ());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
        ಆದರೆ (!nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {  // Non-constant format, can't check format string types
            ಭೇಟಿ_sformatf_format(nodep);
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* oldExprsp = nodep->exprsp();
        ಆದರೆ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        }
        ಆದರೆ (oldExprsp) oldExprsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        ಆಗ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಆದರೆ (oldExprsp) oldExprsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            ಆದರೆ (VN_IS(argp, SFormatArg)) {
                nodep->ಕೂಡಿಸು_Exprsp(argp);
                ಮುಂದುವರಿ;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = argp->dtypep()->skipRefp();
            ಆದರೆ (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isFormatNonNumericArg(dtypep)) {
                ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ಆದರೆ (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(ನಿಜ);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Class handles inside containers print type names, not their members.
                    Ast_ಗಂಟು_DType* ಗಂಟು_Dtypep = dtypep;
                    ಆಗ (ಗಂಟು_Dtypep) {
                        ಗಂಟು_Dtypep = ಗಂಟು_Dtypep->skipRefp();
                        ಆದರೆ (Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ uOrStructDTypep
                            = VN_CAST(ಗಂಟು_Dtypep, ಗಂಟು_UOr_Struct_DType)) {
                            uOrStructDTypep->ಗಣ_Emit_ಗಮ್ಯ_String();
                            v3Global.hasPrintedObjects(ನಿಜ);
                        }
                        ಗಂಟು_Dtypep = ಗಂಟು_Dtypep->subDTypep();
                    }
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ಗಮ್ಯ_String_N{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
                ಆದರೆ (AstEnumDType* ಸ್ಥಿರಾಂಕ enumDtp = formatEnumDType(argp)) {
                    nodep->ಕೂಡಿಸು_Exprsp(newFormatEnumArg(argp, enumDtp));
                    ಮುಂದುವರಿ;
                }
            }
            ಆದರೆ (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            ಆದರೆ (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->ಕೂಡಿಸು_Exprsp(argp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->ಕೂಡಿಸು_Exprsp(ಹೊಸ AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->ಪಠ್ಯ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCReset* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintRef* nodep) override { userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ); }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        ಆದರೆ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
        ಆದರೆ (!m_paramsOnly) {
            nodep->foreach([ಇದು](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    ಹೊಸ AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string ಪಠ್ಯ = VString::dequotePercent(nodep->fmtp()->ಪಠ್ಯ());
            ಆದರೆ (ಪಠ್ಯ.ಖಾಲಿಗಳು()) ಪಠ್ಯ = "Elaboration system task message (IEEE 1800-2023 20.11)";
            ಆಯ್ಕೆ (nodep->displayType()) {
            ಸ್ಥಿತಿ VDisplayType::DT_INFO: nodep->v3warn(USERINFO, ಪಠ್ಯ); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, ಪಠ್ಯ); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, ಪಠ್ಯ); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, ಪಠ್ಯ); ನಿಲ್ಲಿಸು;
            ಪೂರ್ವನಿಯೋಜಿತ: UASSERT_OBJ(ಸುಳ್ಳು, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        ಆದರೆ (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFError* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            user_Iterate_And_ಮುಂದಿನ(nodep->strp(), ಅಗಲ_VP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFEof* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        ಆದರೆ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ಶೂನ್ಯ ಭೇಟಿ(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ಶೂನ್ಯ ಭೇಟಿ(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->ಸ್ಥಾನಾಂತರ(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFGetC* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFGetS* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_ಮುಂದಿನ(nodep->strgp(), ಅಗಲ_VP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFUngetC* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFRead* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->memp(), ಅಗಲ_VP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            ಆದರೆ (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            ಆದರೆ (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFScanF* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_ಮುಂದಿನ(nodep->exprsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSScanF* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->fromp(), ಅಗಲ_VP{SELF, BOTH}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->exprsp(), ಅಗಲ_VP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ರಾಶಿ_Trace_F* nodep) override { nodep->dtype_ಗಣ_String(); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSysIgnore* nodep) override {
        user_Iterate_And_ಮುಂದಿನ(nodep->exprsp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSystemF* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        user_Iterate_And_ಮುಂದಿನ(nodep->memp(), ಅಗಲ_VP{SELF, BOTH}.p());
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* subp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            ಆದರೆ (!adtypep->ಕೀಲಿ_DTypep()->skipRefp()->basicp()
                || !adtypep->ಕೀಲಿ_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDynArrayDType* ಸ್ಥಿರಾಂಕ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        ಆದರೆ (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        user_Iterate_And_ಮುಂದಿನ(nodep->lsbp(), ಅಗಲ_VP{SELF, BOTH}.p());
        user_Iterate_And_ಮುಂದಿನ(nodep->msbp(), ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Test_Plus_ವಾದಗಳು* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮೌಲ್ಯ_Plus_ವಾದಗಳು* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            user_Iterate_And_ಮುಂದಿನ(nodep->outp(), ಅಗಲ_VP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_Format* nodep) override {
        assertAtStatement(nodep);
        ಆದರೆ (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        ಆದರೆ (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        ಆದರೆ (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        ಆದರೆ (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_ಮುಂದಿನ(nodep->passsp(), ಶೂನ್ಯಸೂಚಕ);
        user_Iterate_And_ಮುಂದಿನ(nodep->failsp(), ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_ಮುಂದಿನ(nodep->passsp(), ಶೂನ್ಯಸೂಚಕ);
        user_Iterate_And_ಮುಂದಿನ(nodep->failsp(), ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_ಮುಂದಿನ(nodep->passsp(), ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    ತಾರ್ಕಿಕ pinInstArrayElement(AstPin* nodep, ಸ್ಥಿರಾಂಕ AstRange* rangep, ಪೂರ್ಣಾಂಕ idx) {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        ಆದರೆ (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Types don't match, but all dimensions done, still need the usual checks
        ಆದರೆ (!rangep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಸ್ಥಿರಾಂಕ AstRange* ಸ್ಥಿರಾಂಕ subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        ಪೂರ್ಣಾಂಕ subElems = 1;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elements = rangep->elementsConst();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ from_ಎಡ = idx / subElems;

        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ಸ್ಥಿರಾಂಕ uint32_t conDims = conDTypep->dimensions(ಸುಳ್ಳು).ಎರಡನೇ;
        ಸ್ಥಿರಾಂಕ uint32_t modDims = modDTypep->dimensions(ಸುಳ್ಳು).ಎರಡನೇ;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        ಆದರೆ (arrp && conDims != modDims) {
            ಆದರೆ (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                user_Iterate_And_ಮುಂದಿನ(nodep->exprp(),
                                   ಅಗಲ_VP{conDTypep, FINAL, ಹರಿವು_USE_ನಿಯೋಜಿಸು}.p());
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            // ArraySel index is counted from lo
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = arrp->declRange().ascending() ? from_ಎಡ : elements - 1 - from_ಎಡ;
            // Connection is self-determined
            user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{conDTypep, FINAL, ಹರಿವು_USE_ನಿಯೋಜಿಸು}.p());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elemp = newIfaceArrayElement(fromp, ಬಿಟ್_2)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstArraySel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstArraySel{fromp->fileline(), fromp, ಬಿಟ್_2};
                selp->did_ಅಗಲ(ನಿಜ);
                nodep->exprp(selp);
            }
            ಹಿಂತಿರುಗಿ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        ಆದರೆ (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Packed connection
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ modwidth = modDTypep->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ conwidth = conDTypep->ಅಗಲ();

        // Connection has the width of the port, connect it to every element
        ಆದರೆ (conwidth == modwidth) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        ಆದರೆ (conwidth == modwidth * elements * subElems) {
            ಆದರೆ (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->ಎಡ_Const() << ":"
                                                     << rangep->ಬಲ_Const() << "]");
            }
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ selwidth = modwidth * subElems;
            // Connection is self-determined
            user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{conDTypep, FINAL, ಹರಿವು_USE_ನಿಯೋಜಿಸು}.p());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->unlink_Fr_ಹಿಂಭಾಗ();
            AstSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_ಎಡ), selwidth};
            selp->did_ಅಗಲ(ನಿಜ);
            nodep->exprp(selp);
            ಹಿಂತಿರುಗಿ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{conDTypep, FINAL, ಹರಿವು_USE_ನಿಯೋಜಿಸು}.p());
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        ಆದರೆ (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ಸ್ಥಿರಾಂಕ concatp = VN_CAST(nodep->exprp(), Replicate);
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ countp = concatp ? VN_CAST(concatp->countp(), Const) : ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            ಆದರೆ (countp && countp->ಗಮ್ಯ_UInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                Ast_ನಮೂನೆ* ಸ್ಥಿರಾಂಕ patternp = ಹೊಸ Ast_ನಮೂನೆ{concatp->fileline(), ಶೂನ್ಯಸೂಚಕ};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_ಒಳ_Param_Override);
            m_ಒಳ_Param_Override = is_Override_ನಮೂನೆ(nodep->exprp());
            userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_paramsOnly) {
            ಆದರೆ (!nodep->modVarp()->did_ಅಗಲ()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), ಶೂನ್ಯಸೂಚಕ);
            }
            ಆದರೆ (!nodep->exprp()) {  // No-connect
                ಆದರೆ (nodep->modVarp()->is_Iface_Array_ವಿಭಜಿಸು()) expandIfaceArrayPin(nodep);
                ಹಿಂತಿರುಗಿ;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ಸ್ಥಿರಾಂಕ VDirection pinDirection = nodep->modVarp()->direction();
            ಆದರೆ (VN_IS(nodep->exprp(), ಗಂಟು_ಹರಿವು) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_ಗಂಟು_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_ಗಂಟು_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                ಆದರೆ (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_ಗಂಟು_DType* subDTypep = modDTypep;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ modwidth = modDTypep->ಅಗಲ();
            ಪೂರ್ಣಾಂಕ conwidth = conDTypep->ಅಗಲ();
            ಆದರೆ (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                user_Iterate_And_ಮುಂದಿನ(nodep->exprp(),
                                   ಅಗಲ_VP{subDTypep, FINAL, ಹರಿವು_USE_ನಿಯೋಜಿಸು}.p());
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    ಆದರೆ (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        ಆದರೆ (nodep->modVarp()->is_Iface_Array_ವಿಭಜಿಸು()) expandIfaceArrayPin(nodep);
                        ಹಿಂತಿರುಗಿ;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->ಅಗಲ();
                }
                ಆದರೆ (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->modVarp()->isTristate()) {
                    ಆದರೆ (modwidth != conwidth) {
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
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                ಆದರೆ ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpacked_Array_ಗಾತ್ರ(conDTypep) != unpacked_Array_ಗಾತ್ರ(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpacked_Array_ಗಾತ್ರ(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpacked_Array_ಗಾತ್ರ(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((ಅಭಿವ್ಯಕ್ತಿ_Arrayp && !modArrayp) || (!ಅಭಿವ್ಯಕ್ತಿ_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (ಅಭಿವ್ಯಕ್ತಿ_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_ಗಂಟು_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), ಗಂಟು_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterate_Check_ನಿಯೋಜಿಸು(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            ಆದರೆ (nodep->modVarp()->is_Iface_Array_ವಿಭಜಿಸು()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        ಆದರೆ (!m_paramsOnly) {
            ಆದರೆ (VN_IS(nodep->modp(), Not_ಸಿಕ್ಕಿತು_Module)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_ಮಾರ್ಗ_Looked_ಸಂದೇಶ(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            ಆದರೆ (nodep->rangep()) user_Iterate_And_ಮುಂದಿನ(nodep->rangep(), ಅಗಲ_VP{SELF, BOTH}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->pinsp(), ಶೂನ್ಯಸೂಚಕ);
            // If element of an instance array, pins are now connected, no longer need the ranges
            ಆದರೆ (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ತಳ್ಳು_Deletep(nodep->rangep()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            }
        }
        user_Iterate_And_ಮುಂದಿನ(nodep->paramsp(), ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGatePin* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->rangep(), ಅಗಲ_VP{SELF, BOTH}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ pinDTypep = nodep->exprp()->dtypep();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಂಕೆ_Insts = nodep->rangep()->elementsConst();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ modwidth = ಅಂಕೆ_Insts;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ pinwidth = pinDTypep->ಅಗಲ();
            ಆದರೆ (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = nodep->ಕಂಡುಹಿಡಿ_Logic_DType(1, 1, pinDTypep->numeric());
                user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{subDTypep, FINAL}.p());
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstReplicate{nodep->fileline(), nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                       static_cast<uint32_t>(ಅಂಕೆ_Insts)};
                nodep->replaceWith(newp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->doing_ಅಗಲ()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(ನಿಜ);
            nodep->did_ಅಗಲ(ನಿಜ);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->class_ವಿಧಾನ() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->class_ವಿಧಾನ() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        ಆದರೆ (nodep->class_ವಿಧಾನ() && nodep->name() == "new") {
            ಆದರೆ (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            ಆದರೆ (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_ಗಂಟು* firstp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ಆದರೆ (Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sep = VN_CAST(itemp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                    ಆದರೆ (AstNew* ಸ್ಥಿರಾಂಕ newp = VN_CAST(sep->exprp(), New)) {
                        ಆದರೆ (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            ನಿಲ್ಲಿಸು;
                        }
                    }
                    ಮುಂದುವರಿ;
                }
                ಆದರೆ (ಮೊದಲ_New_Statement_Ok_Recurse(itemp)) ಮುಂದುವರಿ;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doing_ಅಗಲ(ನಿಜ);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        ಆದರೆ (nodep->fvarp()) userIterate(nodep->fvarp(), ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Property)) {
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
        ಅಗಲ_VP* vup = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (VN_IS(nodep, Property)) vup = ಅಗಲ_VP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->did_ಅಗಲ(ನಿಜ);
        nodep->doing_ಅಗಲ(ಸುಳ್ಳು);
        ಆದರೆ (nodep->dpiImport() && !nodep->dpi_Open_ಪೋಷಕ() && markHasOpenArray(nodep)) {
            nodep->dpi_Open_ಪೋಷಕ_Inc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        ಆದರೆ (nodep->fvarp() && !nodep->dpiImport() && !nodep->ಶುದ್ಧ_Virtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->exists_And_ಮುಂದಿನ([&](ಸ್ಥಿರಾಂಕ AstVarRef* varrefp) {
                   ಹಿಂತಿರುಗಿ varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::ಸದಿಶ<AstVar*> ports;
        std::ಸದಿಶ<Ast_Attr_ದ*> protos;
        Ast_Attr_ದ* protop = ಶೂನ್ಯಸೂಚಕ;  // Base prototype
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_ಹಿಂಭಾಗ(portp); }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ attrp = VN_CAST(stmtp, Attr_ದ)) {
                ಆದರೆ (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_ಹಿಂಭಾಗ(attrp); }
                ಆದರೆ (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        ಆದರೆ (protop) {
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < std::max(ports.ಗಾತ್ರ_2(), protos.ಗಾತ್ರ_2()); ++i) {
                ಆದರೆ (i >= ports.ಗಾತ್ರ_2() || i >= protos.ಗಾತ್ರ_2()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    ನಿಲ್ಲಿಸು;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    AstVar* ಸ್ಥಿರಾಂಕ portp = ports[i];
                    Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ rprotop = protos[i];
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ declDtp = portp->dtypep();
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ protoDtp = rprotop->fromp()->dtypep();
                    ಆದರೆ (portp->name() != rprotop->name()) {
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
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!similarDTypeRecurse(protoDtp, declDtp)) {
                        protoDtp->v3warn(
                            PROTOTYPEMIS,
                            "In prototype for "
                                << nodep->prettyNameQ() << ", argument " << portp->prettyNameQ()
                                << " data-type does not match out-of-block"
                                   " declaration's data-type (IEEE 1800-2023 8.24)\n"
                                << protoDtp->warn_ಇನ್ನಷ್ಟು() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warn_ಇನ್ನಷ್ಟು() << "... Declaration data type: "
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
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraint* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
        nodep->did_ಅಗಲ(ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstProperty* nodep) override {
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->doing_ಅಗಲ()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(ನಿಜ);
            nodep->did_ಅಗಲ(ನಿಜ);
            ಹಿಂತಿರುಗಿ;
        }
        nodep->doing_ಅಗಲ(ನಿಜ);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtype_ಗಣ_ಬಿಟ್();
        ಗಾಗಿ (Ast_ಗಂಟು* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            ಆದರೆ (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->did_ಅಗಲ(ನಿಜ);
        nodep->doing_ಅಗಲ(ಸುಳ್ಳು);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        nodep->dtype_ಗಣ_ಬಿಟ್();
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (VN_IS(stmtp, Var)) {
                userIterate(stmtp, ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(stmtp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } ಇಲ್ಲದಿದ್ದರೆ {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->did_ಅಗಲ(ನಿಜ);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* get_ಅಂಶ_Package(Ast_ಗಂಟು* pkgItemp) {
        ಆಗ (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        ಹಿಂತಿರುಗಿ VN_CAST(pkgItemp->backp(), Package);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFuncRef* nodep) override {
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_FTask_Ref*>(nodep));
        ಆದರೆ (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            ಆದರೆ (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->dtypeFrom(nodep->taskp());
        }
        ಆದರೆ (nodep->fileline()->timingOn()) {
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classp = nodep->classOrPackagep();
            ಆದರೆ (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ಸ್ಥಿರಾಂಕ packagep = get_ಅಂಶ_Package(classp);
                ಆದರೆ (packagep && packagep->name() == "std") {
                    ವಿಧಾನ_Call_Warn_Timing(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    ಸ್ಥಿರ ತಾರ್ಕಿಕ are_Same_ಗಾತ್ರ(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        ಆದರೆ (dims0.ಗಾತ್ರ_2() != dims1.ಗಾತ್ರ_2()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < dims0.ಗಾತ್ರ_2(); ++i) {
            ಆದರೆ (dims0[i]->elementsConst() != dims1[i]->elementsConst()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    // Makes sure that port and pin have same size and same datatype
    ಶೂನ್ಯ check_Unpacked_Array_ವಾದಗಳು(AstVar* portp, Ast_ಗಂಟು* pinp) {
        ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ ಪೋರ್ಟ್_Dtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                ಆದರೆ (!are_Same_ಗಾತ್ರ(ಪೋರ್ಟ್_Dtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << ಪೋರ್ಟ್_Dtypep->prettyDTypeNameQ() << ")");
                }
                ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ ಪೋರ್ಟ್_Basicp = ಪೋರ್ಟ್_Dtypep->basicp();
                ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ sameElemDType
                    = ಪೋರ್ಟ್_Basicp && pinBasicp
                          ? ಪೋರ್ಟ್_Basicp->ಅಗಲ() == pinBasicp->ಅಗಲ()
                                && ಪೋರ್ಟ್_Basicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(ಪೋರ್ಟ್_Dtypep->elemDTypep());
                ಆದರೆ (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, ಪೋರ್ಟ್_Dtypep, "Argument", "Parameter"));
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    ಶೂನ್ಯ process_FTask_Ref_ವಾದಗಳು(Ast_ಗಂಟು_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        userIterate(nodep->taskp(), ಶೂನ್ಯಸೂಚಕ);
        //
        // And do the arguments to the task/function too
        ಮಾಡು {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            ಆದರೆ (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
                AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp = argp->exprp();
                ಆದರೆ (!pinp) ಮುಂದುವರಿ;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                ಆದರೆ (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ(&handle);  // Format + additional args, if any
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = ಶೂನ್ಯಸೂಚಕ;
                    ಆಗ (AstArg* ಸ್ಥಿರಾಂಕ nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->ಕೂಡಿಸು_ಮುಂದಿನ(nextargp->exprp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                        nextargp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();  // Remove the call's Arg wrapper
                    }
                    string format;
                    ಆದರೆ (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->ಅಂಕೆ().ಗಮ್ಯ_String();
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(argp), argp);
                    AstSFormatF* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstSFormatF{nodep->fileline(), format, ಸುಳ್ಳು, argsp};
                    ಆದರೆ (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(ಹೊಸ AstScopeName{newp->fileline(), ನಿಜ});
                    }
                    handle.relink(ಹೊಸ AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    ಹೋಗು reloop;  // so exit early; next loop will correct it
                }  //
                ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlink_Fr_ಹಿಂಭಾಗ(&handle);  // No next, that's the next pin
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                ಆದರೆ (portp->isWritable()) {
                    V3LinkLValue::link_LValue_ಗಣ(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, ಅಗಲ_VP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } ಆಗ (ಸುಳ್ಳು);
        // Stage 2
        {
            ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
                AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
                ಸ್ಥಿರಾಂಕ AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ pinp = argp->exprp();
                ಆದರೆ (!pinp) ಮುಂದುವರಿ;  // Argument error we'll find later
                // Change data types based on above accept completion
                ಆದರೆ (nodep->taskp()->dpiImport()) check_Unpacked_Array_ವಾದಗಳು(portp, pinp);
                ಆದರೆ (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
                ಸ್ಥಿರಾಂಕ AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ pinp = argp->exprp();
                ಆದರೆ (!pinp) ಮುಂದುವರಿ;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, ಅಗಲ_VP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        ಆದರೆ (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
                ಸ್ಥಿರಾಂಕ AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp = argp->exprp();
                ಆದರೆ (!pinp) ಮುಂದುವರಿ;  // Argument error we'll find later
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಪೋರ್ಟ್_DTypep = portp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
                ಆದರೆ (VN_IS(pinp, ಖಾಲಿಗಳು_ಸರತಿ)) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = new_Cons_Dyn_Array_Or_ಸರತಿ(pinp->fileline(), ಪೋರ್ಟ್_DTypep);
                    ಆದರೆ (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = ಹೊಸ AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(ಪೋರ್ಟ್_DTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ pinDTypep = pinp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
                ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ಪೋರ್ಟ್_Ifacep
                    = VN_CAST(ಪೋರ್ಟ್_DTypep->elemDTypep(ನಿಜ), IfaceRefDType);
                ಸ್ಥಿರಾಂಕ VCastable ifaceCastable
                    = ಪೋರ್ಟ್_Ifacep ? Ast_ಗಂಟು::computeCastable(ಪೋರ್ಟ್_DTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ ನಿಜ)
                                 : VCastable{VCastable::UNSUPPORTED};
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ matchingRefDTypes = ಪೋರ್ಟ್_Ifacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(ಪೋರ್ಟ್_DTypep, pinDTypep);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->ಅಂಕೆ().isNull();
                ಆದರೆ ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    ಆದರೆ (ಪೋರ್ಟ್_Ifacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << ಪೋರ್ಟ್_DTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಪೋರ್ಟ್_Ifacep && ಪೋರ್ಟ್_Ifacep->isVirtual() && portp->is_ಒಳಹರಿವು() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (portp->isWritable() && pinp->ಅಗಲ() != portp->ಅಗಲ()) {
                    pinp->v3widthWarn(portp->ಅಗಲ(), pinp->ಅಗಲ(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->ಅಗಲ()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->ಅಗಲ() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                ಆದರೆ (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(ಪೋರ್ಟ್_DTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && ಪೋರ್ಟ್_DTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << ಪೋರ್ಟ್_DTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    ಮುಂದುವರಿ;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                ಆದರೆ (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    check_Class_ನಿಯೋಜಿಸು(nodep, "Function Argument", pinp, ಪೋರ್ಟ್_DTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, ಅಗಲ_VP{ಪೋರ್ಟ್_DTypep, FINAL, ಹರಿವು_USE_ನಿಯೋಜಿಸು}.p());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    iterate_Check_ನಿಯೋಜಿಸು(nodep, "Function Argument", pinp, FINAL, ಪೋರ್ಟ್_DTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    ಶೂನ್ಯ handle_Std_Randomize_ವಾದಗಳು(Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ nodep) {
        AstConst* nullp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = argp->exprp();
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(exprp, Const)) {
                ಆದರೆ (constp->ಅಂಕೆ().isNull()) {
                    nullp = constp;
                    ಮುಂದುವರಿ;
                }
            }
            ಆದರೆ (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp = VN_CAST(exprp, CMethodHard)) {
                ಆದರೆ (methodp->ವಿಧಾನ() == VCMethod::ARRAY_AT
                    || methodp->ವಿಧಾನ() == VCMethod::ARRAY_AT_WRITE) {
                    ಮುಂದುವರಿ;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        ಆದರೆ (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(ನಿಜ);
            nodep->dtypep(nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::INT));
            ಹಿಂತಿರುಗಿ;  // Handled in V3Randomize
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep->classOrPackagep(), Class);
            ಆದರೆ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(ನಿಜ);
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ adtypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType();
                withp = ವಿಧಾನ_With_Clause(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, adtypep->ಕಂಡುಹಿಡಿ_Void_DType(),
                                         adtypep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(), adtypep);
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_ಮುಂದಿನ(argp->exprp(), ಅಗಲ_VP{SELF, BOTH}.p());
                }
                handle_Std_Randomize_ವಾದಗಳು(nodep);  // Provided args should be in current scope
                process_FTask_Ref_ವಾದಗಳು(nodep);
                ಆದರೆ (withp) nodep->withp(withp);
                nodep->did_ಅಗಲ(ನಿಜ);
                ಹಿಂತಿರುಗಿ;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            ಆದರೆ (nodep->name() == "randomize") {
                AstClassRefDType* ಸ್ಥಿರಾಂಕ adtypep
                    = ಹೊಸ AstClassRefDType{nodep->fileline(), classp, ಶೂನ್ಯಸೂಚಕ};
                v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(adtypep);
                withp = ವಿಧಾನ_With_Clause(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, adtypep->ಕಂಡುಹಿಡಿ_Void_DType(),
                                         adtypep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(), adtypep);
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_ಮುಂದಿನ(argp->exprp(), ಅಗಲ_VP{SELF, BOTH}.p());
                }
                handle_Randomize_ವಾದಗಳು(nodep, classp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "get_randstate") {
                ವಿಧಾನ_Ok_Arguments(nodep, 0, 0);
                classp->ತಳ_Most_Classp()->needRNG(ನಿಜ);
                v3Global.useRandomizeMethods(ನಿಜ);
                AstCExpr* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtype_ಗಣ_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "set_randstate") {
                ವಿಧಾನ_Ok_Arguments(nodep, 1, 1);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->argsp()->exprp();
                classp->ತಳ_Most_Classp()->needRNG(ನಿಜ);
                v3Global.useRandomizeMethods(ನಿಜ);
                AstCExpr* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->ಕೂಡಿಸು(exprp->unlink_Fr_ಹಿಂಭಾಗ());
                newp->ಕೂಡಿಸು(")");
                newp->dtype_ಗಣ_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(ಸುಳ್ಳು, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ಆದರೆ (nodep->did_ಅಗಲ()) {
            ಆದರೆ (withp) nodep->withp(withp);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ ((nodep->taskp()->class_ವಿಧಾನ() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            ತಾರ್ಕಿಕ allow = ಸುಳ್ಳು;
            ಆದರೆ (m_ftaskp && m_ftaskp->class_ವಿಧಾನ() && !m_ftaskp->isStatic()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstFuncRef* ಸ್ಥಿರಾಂಕ funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstTaskRef* ಸ್ಥಿರಾಂಕ taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                ಆದರೆ (!allow) {
                    ಸ್ಥಿರಾಂಕ AstClass* callerClassp = m_containingClassFinder.ಕಂಡುಹಿಡಿ(m_ftaskp);
                    ಆದರೆ (!callerClassp) {
                        callerClassp = m_containingClassFinder.ಕಂಡುಹಿಡಿ(m_ftaskp->classOrPackagep());
                    }
                    ಸ್ಥಿರಾಂಕ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ಆದರೆ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.ಕಂಡುಹಿಡಿ(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            ಆದರೆ (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ cgClassp = m_containingClassFinder.ಕಂಡುಹಿಡಿ(nodep);
                ಆದರೆ (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ಸ್ಥಿರಾಂಕ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ಆದರೆ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.ಕಂಡುಹಿಡಿ(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            ಆದರೆ (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        ಆದರೆ (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(ಹೊಸ AstScopeName{nodep->fileline(), ಸುಳ್ಳು});
        }
        // And do the arguments to the task/function too
        process_FTask_Ref_ವಾದಗಳು(nodep);
        ಆದರೆ (withp) nodep->withp(withp);
        nodep->did_ಅಗಲ(ನಿಜ);
        // See steps that follow in visit(AstFuncRef*)
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        ಆದರೆ (nodep->is_Combo_ನಕ್ಷತ್ರ()) ಹಿಂತಿರುಗಿ;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        ಆದರೆ (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ concatOrReplp = VN_CAST(nodep->sensp(), ಗಂಟು_Biop);
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ rhsp = concatOrReplp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (nodep->ಅಂಚು_Type() == VEdgeType::ET_ಬದಲಾಯಿತು) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ lhsp = concatOrReplp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ Ast_Sen_ಅಂಶ{lhsp->fileline(), nodep->ಅಂಚು_Type(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(ಹೊಸ Ast_Sen_ಅಂಶ{rhsp->fileline(), nodep->ಅಂಚು_Type(), rhsp});
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            userIterateChildren(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
            ಆದರೆ (nodep->ಅಂಚು_Type().an_ಅಂಚು()) {
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ sensDtp = nodep->sensp()->dtypep()->skipRefp();
                ಆದರೆ (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Clocking_ಅಂಶ* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ಸ್ಥಿರಾಂಕ refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, ಅಗಲ_VP{SELF, PRELIM}.p());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWait* nodep) override {
        ಆದರೆ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->fileline()->timingOn()) {
            ಆದರೆ (v3Global.opt.timing().is_ಗಣ_True()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->condp(), ಗಂಟು_Var_Ref)) {
                    ಆದರೆ (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warn_ಇನ್ನಷ್ಟು() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.timing().is_ಗಣ_False()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            nodep->replaceWith(stmtsp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->ಸೂಚ್ಯಂಕ_Arg_Refp(), ಶೂನ್ಯಸೂಚಕ);
        userIterateChildren(nodep->ಮೌಲ್ಯ_Arg_Refp(), ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (!nodep->exprp()) {
            nodep->dtype_ಗಣ_Void();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (!nodep->exprp()->hasDType()) {
                user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (vdtypep) {
                    user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{nodep->dtypep(), PRELIM}.p());
                } ಇಲ್ಲದಿದ್ದರೆ {  // 'sort with' allows arbitrary type
                    user_Iterate_And_ಮುಂದಿನ(nodep->exprp(), ಅಗಲ_VP{SELF, PRELIM}.p());
                }
            }

            ಆದರೆ (!nodep->exprp()->hasDType()) {
                nodep->dtype_ಗಣ_Void();
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->dtypeFrom(nodep->exprp());
                iterate_Check_ನಿಯೋಜಿಸು(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        ಆದರೆ (nodep->ಸೂಚ್ಯಂಕ()) {
            nodep->dtypeFrom(m_withp->ಸೂಚ್ಯಂಕ_Arg_Refp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->dtypeFrom(m_withp->ಮೌಲ್ಯ_Arg_Refp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, ಶೂನ್ಯಸೂಚಕ);
    }

    //--------------------
    // Default
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (!nodep->did_ಅಗಲ()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << ಸಾಲು_ಅಂತ್ಯ);
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->ಅಗಲ() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->ಅಗಲ() << " bits -> " << nodep->ಅಗಲ()
                                    << " bits" << ಸಾಲು_ಅಂತ್ಯ);

        // Final processing
        user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, FINAL}.p());
        nodep->did_ಅಗಲ(ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ_Class(AstClass* nodep) {
        ಆದರೆ (nodep->did_ಅಗಲ_And_ಗಣ()) ಹಿಂತಿರುಗಿ;

        // If the class is std::process
        ಆದರೆ (nodep->name() == "process") {
            AstPackage* ಸ್ಥಿರಾಂಕ packagep = get_ಅಂಶ_Package(nodep);
            ಆದರೆ (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp
                    = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, "m_process"), Var)) {
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->get_ಮಗು_DTypep();
                    ಆದರೆ (!varp->dtypep()) {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(dtypep->unlink_Fr_ಹಿಂಭಾಗ()), dtypep);
                    }
                    AstBasicDType* ಸ್ಥಿರಾಂಕ newdtypep = ಹೊಸ AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp
                    = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, "self"), ಗಂಟು_FTask)) {
                    ftaskp->ಗಣ_Need_Process();
                    v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        user_Iterate_And_ಮುಂದಿನ(nodep->extendsp(), ಶೂನ್ಯಸೂಚಕ);
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);  // First size all members
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enum_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು);
        VL_RESTORER_COPY(m_inside_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು);
        ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep, Class)) {
            ಭೇಟಿ_Class(classp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([ಇದು](ಸ್ಥಿರಾಂಕ AstVar* varp) { m_curModVars.ಸೇರಿಸು(varp); });
            userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, ಶೂನ್ಯಸೂಚಕ);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    ಶೂನ್ಯ ಭೇಟಿ_Or_Lu64(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_ಗಣ_Double();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_ಗಣ_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, PRELIM}.p());
            ಆದರೆ (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    ಹೊಸ AstISToRD{nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ()});
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_ಗಣ_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, PRELIM}.p());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_Os32_Lr(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ಗಣ_Integer2_State();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_Ou64_Lr(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ಗಣ_UInt64();
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ_log_not(AstLogNot* nodep) {
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
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            ಆದರೆ (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_log_and_or(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic(
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_Abort_Prop(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_red_and_or(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_red_unknown(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    ತಾರ್ಕಿಕ isEquivalentDType(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* lhs, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* rhs) {
        // a) If two types match, they are equivalent.
        ಆದರೆ (!lhs || !rhs) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        lhs = lhs->skip_Ref_ಗಮ್ಯ_Enump();
        rhs = rhs->skip_Ref_ಗಮ್ಯ_Enump();
        ಆದರೆ (lhs == rhs) ಹಿಂತಿರುಗಿ ನಿಜ;
        // If both are basic types, check if they are the same type
        ಆದರೆ (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* lb = VN_CAST(lhs, BasicDType);
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* rb = VN_CAST(rhs, BasicDType);
            ಆದರೆ (lb->isString() != rb->isString()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        ಆದರೆ (lhsIsUnpackArray || rhsIsUnpackArray) {
            ಆದರೆ (lhsIsUnpackArray && rhsIsUnpackArray) {
                ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(lhs, UnpackArrayDType);
                ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(rhs, UnpackArrayDType);
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsz = lhsp->elementsConst();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rsz = rhsp->elementsConst();
                ಆದರೆ (lsz >= 0 && rsz >= 0 && lsz != rsz) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                ಹಿಂತಿರುಗಿ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lhs_Is_ಸರತಿ = VN_IS(lhs, ಸರತಿ_DType);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhs_Is_ಸರತಿ = VN_IS(rhs, ಸರತಿ_DType);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        ಆದರೆ (lhsIsDynArray || rhsIsDynArray || lhs_Is_ಸರತಿ || rhs_Is_ಸರತಿ || lhsIsAssocArray
            || rhsIsAssocArray) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDynArrayDType* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(lhs, DynArrayDType)) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDynArrayDType* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(rhs, DynArrayDType)) {
                    ಹಿಂತಿರುಗಿ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(lhs, ಸರತಿ_DType)) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(rhs, ಸರತಿ_DType)) {
                    ಹಿಂತಿರುಗಿ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAssocArrayDType* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(lhs, AssocArrayDType)) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAssocArrayDType* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    ಹಿಂತಿರುಗಿ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->ಕೀಲಿ_DTypep(), rhsp->ಕೀಲಿ_DTypep());
                }
            }

            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        ಆದರೆ (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            ಆದರೆ (lhs->ಅಗಲ() != rhs->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (lhs->isFourstate() != rhs->isFourstate()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (lhs->isSigned() != rhs->isSigned()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        ಆದರೆ (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ !(VN_IS(lhs, ಗಂಟು_UOr_Struct_DType) || VN_IS(rhs, ಗಂಟು_UOr_Struct_DType));
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isAggregateType(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep = nodep->dtypep();
        ಆದರೆ (!dtypep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        dtypep = dtypep->skipRefp();
        ಆದರೆ (!dtypep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ dtypep->isAggregateType();
    }

    ಶೂನ್ಯ ಭೇಟಿ_cmp_eq_gt(Ast_ಗಂಟು_Biop* nodep, ಸ್ಥಿರಾಂಕ ComparisonType cmpType) {
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
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lhsIsPat = VN_IS(nodep->lhsp(), ನಮೂನೆ);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsIsPat = VN_IS(nodep->rhsp(), ನಮೂನೆ);
            ಆದರೆ (lhsIsPat && !rhsIsPat) {
                user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rhsIsPat && !lhsIsPat) {
                user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } ಇಲ್ಲದಿದ್ದರೆ {
                user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            }

            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isAggrLhs = isAggregateType(nodep->lhsp());
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isAggrRhs = isAggregateType(nodep->rhsp());

            ಆದರೆ ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::ಸಾಮಾನ್ಯ_EQ || cmpType == ComparisonType::CASE)) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDType = nodep->lhsp()->dtypep();
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ rhsDType = nodep->rhsp()->dtypep();

                ಆದರೆ (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convert_Unpacked_ಕ್ರಮ(nodep->rhsp(), nodep->lhsp()->dtypep());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                ಆದರೆ (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
                ಆದರೆ (Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                ಆದರೆ (Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                ಆದರೆ (Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = std::max(nodep->lhsp()->ಅಗಲ(), nodep->rhsp()->ಅಗಲ());
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ewidth = std::max(nodep->lhsp()->ಅಗಲ_Min(), nodep->rhsp()->ಅಗಲ_Min());
                ತಾರ್ಕಿಕ warnOn = ನಿಜ;
                ಆದರೆ (!signedFl && ಅಗಲ == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    ಆದರೆ ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->ಅಗಲ() >= nodep->rhsp()->ಅಗಲ_Min())) {
                        warnOn = ಸುಳ್ಳು;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    ಆದರೆ ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->ಅಗಲ_Min() >= nodep->rhsp()->ಅಗಲ())) {
                        warnOn = ಸುಳ್ಳು;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(ಅಗಲ, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(ಅಗಲ, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::ಸಾಮಾನ್ಯ_EQ
                                               || cmpType == ComparisonType::ಸಾಮಾನ್ಯ_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_cmp_real(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_cmp_string(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ಗಣ_ಬಿಟ್();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_cmp_type(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{SELF, BOTH}.p());
            ಸ್ಥಿರಾಂಕ Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ lhsap = VN_AS(nodep->lhsp(), Attr_ದ);
            ಸ್ಥಿರಾಂಕ Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ rhsap = VN_AS(nodep->rhsp(), Attr_ದ);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDtp = lhsap->dtypep();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ invert = VN_IS(nodep, NeqT);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ identical
                = Ast_ಗಂಟು::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ eq = invert ^ identical;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_True{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ_negate_not(Ast_ಗಂಟು_Uniop* nodep, ತಾರ್ಕಿಕ real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            ಆದರೆ (!real_ok) checkCvtUS(nodep->lhsp(), ಸುಳ್ಳು);
        }
        ಆದರೆ (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            ಆದರೆ (Ast_ಗಂಟು_Uniop* ಸ್ಥಿರಾಂಕ newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtype_ಗಣ_Double();
                ಹಿಂತಿರುಗಿ;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        ಆದರೆ (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        ಆದರೆ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = expDTypep;
            // Some warning suppressions
            ತಾರ್ಕಿಕ lhsWarn = ನಿಜ;
            ಆದರೆ (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                ಆದರೆ (subDTypep->ಅಗಲ_Min() == (nodep->lhsp()->ಅಗಲ_Min() + 1)) lhsWarn = ಸುಳ್ಳು;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ_signed_unsigned(Ast_ಗಂಟು_Uniop* nodep, VSigning rs_ಹೊರಹೋಗು) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ನಿಜ);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->lhsp()->ಅಗಲ();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                ಅಗಲ, ಅಗಲ, rs_ಹೊರಹೋಗು, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ_ಸರಿಸು_2(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_ಸರಿಸು_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ newp = iterate_ಸರಿಸು_final(nodep);
        VL_DANGLING(nodep);
        (ಶೂನ್ಯ)newp;  // Ununused
    }
    ಶೂನ್ಯ iterate_ಸರಿಸು_prelim(Ast_ಗಂಟು_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ಸುಳ್ಳು);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ಆದರೆ (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtype_ಗಣ_Logic_Unsized(nodep->ಅಗಲ(), nodep->ಅಗಲ_Min(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_ಗಂಟು_Biop* iterate_ಸರಿಸು_final(Ast_ಗಂಟು_Biop* nodep) {
        // Nodep maybe edited
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            ಆದರೆ (VN_IS(nodep, ಸರಿಸು_RS)) {
                ಆದರೆ (Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            ತಾರ್ಕಿಕ warnOn = ನಿಜ;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            ಆದರೆ (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), ಗಂಟು_ನಿಯೋಜಿಸು)) warnOn = ಸುಳ್ಳು;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ಸ್ಥಿರಾಂಕ shiftp = VN_CAST(nodep->rhsp(), Const);
            ಆದರೆ (shiftp && !shiftp->ಅಂಕೆ().isFourState() && shiftp->ಅಗಲ() <= 32) {
                ಸ್ಥಿರಾಂಕ int64_t ಸರಿಸು_Val = shiftp->ಅಂಕೆ().ಗಮ್ಯ_SQuad();
                ಆದರೆ (VN_IS(nodep, ಸರಿಸು_L)) {
                    ಆದರೆ (ಸರಿಸು_Val > 0 && nodep->ಅಗಲ() == nodep->lhsp()->ಅಗಲ() + ಸರಿಸು_Val)
                        warnOn = ಸುಳ್ಳು;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            ಆದರೆ (nodep->rhsp()->ಅಗಲ() > 32) {
                ಆದರೆ (shiftp && shiftp->ಅಂಕೆ().most_ಗಣ_ಬಿಟ್_P1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_ಸಂಖ್ಯೆ ಅಂಕೆ{shiftp, 32, shiftp->ಅಂಕೆ()};
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(ಹೊಸ AstConst{shiftrhsp->fileline(), ಅಂಕೆ});
                    VL_DO_DANGLING(shiftrhsp->delete_ವೃಕ್ಷ(), shiftrhsp);
                }
            }
        }
        ಹಿಂತಿರುಗಿ nodep;  // May edit
    }

    ಶೂನ್ಯ ಭೇಟಿ_boolexpr_and_or(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ಸುಳ್ಳು);
            checkCvtUS(nodep->rhsp(), ಸುಳ್ಳು);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = std::max(nodep->lhsp()->ಅಗಲ(), nodep->rhsp()->ಅಗಲ());
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ mwidth = std::max(nodep->lhsp()->ಅಗಲ_Min(), nodep->rhsp()->ಅಗಲ_Min());
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(
                ಅಗಲ, mwidth, VSigning::fromBool(expSigned),
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        ಆದರೆ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ_ಕೂಡಿಸು_sub_replace(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ real_ok, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ modOrDiv) {
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
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_ಮುಂದಿನ(nodep->lhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ಮುಂದಿನ(nodep->rhsp(), ಅಗಲ_VP{CONTEXT_DET, PRELIM}.p());
            ಆದರೆ (!real_ok) {
                checkCvtUS(nodep->lhsp(), ಸುಳ್ಳು);
                checkCvtUS(nodep->rhsp(), ಸುಳ್ಳು);
            }
            ಆದರೆ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                ಆದರೆ (Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtype_ಗಣ_Double();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, ಕೂಡಿಸು_2)
                            ? (nodep->warn_ಇನ್ನಷ್ಟು()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = std::max(nodep->lhsp()->ಅಗಲ(), nodep->rhsp()->ಅಗಲ());
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ mwidth = std::max(nodep->lhsp()->ಅಗಲ_Min(), nodep->rhsp()->ಅಗಲ_Min());
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(
                    ಅಗಲ, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ಆದರೆ (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            ಆದರೆ (Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            ತಾರ್ಕಿಕ lhsWarn = ನಿಜ;
            ತಾರ್ಕಿಕ rhsWarn = ನಿಜ;
            ಆದರೆ (VN_IS(nodep, ಕೂಡಿಸು_2) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                ಆದರೆ (subDTypep->ಅಗಲ_Min() == (nodep->lhsp()->ಅಗಲ_Min() + 1)) lhsWarn = ಸುಳ್ಳು;
                ಆದರೆ (subDTypep->ಅಗಲ_Min() == (nodep->rhsp()->ಅಗಲ_Min() + 1)) rhsWarn = ಸುಳ್ಳು;
                ಆದರೆ (VN_IS(nodep, ಕೂಡಿಸು_2) && nodep->lhsp()->ಅಗಲ() == 1
                    && nodep->rhsp()->ಅಗಲ() != 1)
                    lhsWarn = ಸುಳ್ಳು;  // do_increment + ...
                ಆದರೆ (nodep->rhsp()->ಅಗಲ() == 1 && nodep->lhsp()->ಅಗಲ() != 1)
                    rhsWarn = ಸುಳ್ಳು;  // ... + do_increment
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                ಆದರೆ (subDTypep->ಅಗಲ_Min() >= (nodep->lhsp()->ಅಗಲ_Min())) lhsWarn = ಸುಳ್ಳು;
                ಆದರೆ (subDTypep->ಅಗಲ_Min() >= (nodep->rhsp()->ಅಗಲ_Min())) rhsWarn = ಸುಳ್ಳು;
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
    ಶೂನ್ಯ ಭೇಟಿ_real_ಕೂಡಿಸು_sub(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ಗಣ_Double();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_real_neg_ceil(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಆದರೆ (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ಗಣ_Double();
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2(), nodep, "Assumes constant format");
        ತಾರ್ಕಿಕ ಒಳ_Pct = ಸುಳ್ಳು;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argp = nodep->exprsp();
        string newFormat;
        ಗಾಗಿ (ಅಕ್ಷರ ch : nodep->ಪಠ್ಯ()) {
            ಆದರೆ (!ಒಳ_Pct && ch == '%') {
                ಒಳ_Pct = ನಿಜ;
                newFormat += ch;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಒಳ_Pct && (std::ಅಂಕಿಯೇ(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಒಳ_Pct) {  // Normal text
                newFormat += ch;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಒಳ_Pct = ಸುಳ್ಳು;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nextp = argp ? VN_AS(argp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ) : ಶೂನ್ಯಸೂಚಕ;
                AstSFormatArg* ಸ್ಥಿರಾಂಕ fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subargp = fargp ? fargp->exprp() : argp;
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : ಶೂನ್ಯಸೂಚಕ;
                ch = std::tolower(ch);
                ಆಯ್ಕೆ (ch) {
                ಸ್ಥಿತಿ '?':  // Unspecified by user, guess
                    ಆದರೆ (dtypep->isDouble()) {
                        ch = 'g';
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dtypep->isString()) {
                        ch = 's';
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ch = 'p';
                    }
                    argp = nextp;
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '%': ನಿಲ್ಲಿಸು;  // %% - just output a %
                ಸ್ಥಿತಿ 'm': ನಿಲ್ಲಿಸು;  // %m - auto insert "name"
                ಸ್ಥಿತಿ 'l': ನಿಲ್ಲಿಸು;  // %m - auto insert "library"
                ಸ್ಥಿತಿ 'd':
                    ಆದರೆ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ಆದರೆ (subargp->isDouble()) spliceCvtS(subargp, ನಿಜ, 64);
                        argp = nextp;
                    }
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'b':  // FALLTHRU
                ಸ್ಥಿತಿ 'o':  // FALLTHRU
                ಸ್ಥಿತಿ 'x':
                    ಆದರೆ (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 't':  // Decimal/float time
                    ಆದರೆ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'e':  // FALLTHRU
                ಸ್ಥಿತಿ 'f':  // FALLTHRU
                ಸ್ಥಿತಿ 'g':
                    ಆದರೆ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ಆದರೆ (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'p':  // FALLTHRU
                ಸ್ಥಿತಿ 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    ಆದರೆ (subargp && !fargp) {
                        ಆದರೆ (AstEnumDType* ಸ್ಥಿರಾಂಕ enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    ನಿಲ್ಲಿಸು;
                ಪೂರ್ವನಿಯೋಜಿತ:  // Most operators, just move to next argument
                    argp = nextp;
                    ನಿಲ್ಲಿಸು;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->ಪಠ್ಯ(newFormat);
    }

    ಸ್ಥಿರ AstEnumDType* formatEnumDType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType);
        ಆದರೆ (!enumDtp) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType);
            }
        }
        ಹಿಂತಿರುಗಿ enumDtp;
    }

    Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* newEnumCapture(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, AstEnumDType* dtypep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = valuep->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{flp, VVarType::XTEMP, m_enum_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು.get(valuep), dtypep};
        varp->noSubst(ನಿಜ);
        ಆದರೆ (m_ftaskp) {
            varp->funcLocal(ನಿಜ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->ಕೂಡಿಸು_Stmtsp(varp);
        }
        iterate(varp);
        Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ exprp = ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
            flp, ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            ಹೊಸ AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        ಹಿಂತಿರುಗಿ exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, AstEnumDType* dtypep) {
        ಆದರೆ (valuep->is_ಶುದ್ಧ()) valuep = V3Const::constifyEdit(valuep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* keyp;
        ಆದರೆ (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        }
        ಸ್ಥಿರಾಂಕ VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ಸ್ಥಿರಾಂಕ argp = ಹೊಸ AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        ಹಿಂತಿರುಗಿ argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    ತಾರ್ಕಿಕ ಅಗಲ_Bad(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_DType* expDTypep) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ exp_ಅಗಲ = expDTypep->ಅಗಲ();
        ಪೂರ್ಣಾಂಕ exp_ಅಗಲ_Min = expDTypep->ಅಗಲ_Min();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        ಆದರೆ (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->ಅಗಲ() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(exp_ಅಗಲ != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        ಆದರೆ (exp_ಅಗಲ_Min == 0) exp_ಅಗಲ_Min = exp_ಅಗಲ;
        ಆದರೆ (nodep->dtypep()->ಅಗಲ() == exp_ಅಗಲ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nodep->dtypep()->ಅಗಲ_Sized() && nodep->ಅಗಲ() != exp_ಅಗಲ_Min) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (!nodep->dtypep()->ಅಗಲ_Sized() && nodep->ಅಗಲ_Min() > exp_ಅಗಲ_Min) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ fix_ಅಗಲ_Extend(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Ast_ಗಂಟು_DType* expDTypep, Extend_ನಿಯಮ extend_ನಿಯಮ) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<ಪೂರ್ಣಾಂಕ>(extend_ನಿಯಮ) << ") old: " << nodep);
        ಆದರೆ (extend_ನಿಯಮ == EXTEND_OFF) ಹಿಂತಿರುಗಿ;
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ exp_ಅಗಲ = expDTypep->ಅಗಲ();
        ಆದರೆ (constp && !constp->ಅಂಕೆ().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, exp_ಅಗಲ, constp->ಅಂಕೆ()};
            ಅಂಕೆ.isSigned(ಸುಳ್ಳು);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), ಅಂಕೆ};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (exp_ಅಗಲ < nodep->ಅಗಲ()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&linker);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSel{nodep->fileline(), nodep, 0, exp_ಅಗಲ};
            newp->did_ಅಗಲ(ನಿಜ);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Extend
            VNRelinker linker;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&linker);
            ತಾರ್ಕಿಕ doSigned = ಸುಳ್ಳು;
            ಆಯ್ಕೆ (extend_ನಿಯಮ) {
            ಸ್ಥಿತಿ EXTEND_ZERO: doSigned = ಸುಳ್ಳು; ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ EXTEND_LHS: doSigned = nodep->isSigned(); ನಿಲ್ಲಿಸು;
            ಪೂರ್ವನಿಯೋಜಿತ: nodep->v3fatalSrc("bad case");
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp
                = (doSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstExtend{nodep->fileline(), nodep}));
            newp->did_ಅಗಲ(ನಿಜ);
            linker.relink(newp);
            nodep = newp;
        }
        ಆದರೆ (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = spliceCvtD(nodep);
            nodep = newp;
        }
        ಆದರೆ (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtype_Chg_ಅಗಲ_Signed(exp_ಅಗಲ, expDTypep->ಅಗಲ_Min(), expDTypep->numeric());
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    ಶೂನ್ಯ fix_ಅಗಲ_Reduce(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ exp_ಅಗಲ = 1;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ expSigned = ಸುಳ್ಳು;
        ಆದರೆ (nodep->ಅಗಲ() == exp_ಅಗಲ) ಹಿಂತಿರುಗಿ;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const);
        ಆದರೆ (constp) {
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ(nodep, exp_ಅಗಲ);
            ಅಂಕೆ.opRedOr(constp->ಅಂಕೆ());
            ಅಂಕೆ.isSigned(expSigned);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), ಅಂಕೆ};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            VNRelinker linker;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&linker);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypeSkipp = nodep->dtypep()->skipRefp();
            ಆದರೆ (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = ಹೊಸ AstNeq{nodep->fileline(),
                                  ಹೊಸ AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } ಇಲ್ಲದಿದ್ದರೆ {
                // But, AstRedOr can be faster on wide
                newp = ಹೊಸ AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtype_Chg_ಅಗಲ_Signed(exp_ಅಗಲ, exp_ಅಗಲ, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    ತಾರ್ಕಿಕ fixAutoExtend(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& nodepr, ಪೂರ್ಣಾಂಕ exp_ಅಗಲ) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodepr, Const)) {
            ಆದರೆ (constp->ಅಂಕೆ().autoExtend() && !constp->ಅಂಕೆ().sized() && constp->ಅಗಲ() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_ಸಂಖ್ಯೆ ಅಂಕೆ(constp, exp_ಅಗಲ);
                ಅಂಕೆ.opRepl(constp->ಅಂಕೆ(), exp_ಅಗಲ);  // {width{'1}}
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{constp->fileline(), ಅಂಕೆ};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!constp->ಅಂಕೆ().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && exp_ಅಗಲ > 32 && constp->ಅಂಕೆ().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << exp_ಅಗಲ
                                                  << " bits: " << constp->prettyName());
                V3_ಸಂಖ್ಯೆ ಅಂಕೆ(constp, exp_ಅಗಲ);
                ಅಂಕೆ.opExtendXZ(constp->ಅಂಕೆ(), constp->ಅಗಲ());
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{constp->fileline(), ಅಂಕೆ};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // No change
    }
    ತಾರ್ಕಿಕ is_ತಳ_Class_Recurse(ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ cls1p, ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        ಆದರೆ (cls1p == cls2p) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            ಆದರೆ (is_ತಳ_Class_Recurse(cls1p, cextp->classp())) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // Lines to end a message on nodep with the data types of its operands
    ಸ್ಥಿರ string warnDTypes(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* lhsDtp,
                             ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* rhsDtp, ಸ್ಥಿರಾಂಕ string& lhsName = "Left-hand",
                             ಸ್ಥಿರಾಂಕ string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        ಆದರೆ (lhsState == rhsState) lhsState = rhsState = "";
        ಹಿಂತಿರುಗಿ "\n" + nodep->warn_ಇನ್ನಷ್ಟು() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warn_ಇನ್ನಷ್ಟು()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    ಸ್ಥಿರ ತಾರ್ಕಿಕ unpacked_Array_Sizes_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* lhsDtp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* rhsDtp) {
        ಆಗ (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            ಆದರೆ (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T, ಬಗೆ_ಹೆಸರು N>
    ಶೂನ್ಯ checkUnpackedArrayAssignmentCompatible(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ T* ಸ್ಥಿರಾಂಕ lhsRefp,
                                                ಸ್ಥಿರಾಂಕ N* ಸ್ಥಿರಾಂಕ rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::ಮೌಲ್ಯ || std::is_same<T, Ast_ಗಂಟು_Var_Ref>::ಮೌಲ್ಯ)
                && (std::is_same<N, AstVar>::ಮೌಲ್ಯ || std::is_same<N, Ast_ಗಂಟು_Var_Ref>::ಮೌಲ್ಯ),
            "Unsupported types provided.");
        ಆದರೆ (!lhsRefp || !rhsRefp) ಹಿಂತಿರುಗಿ;
        std::string lhsName;
        std::string rhsName;
        ಆದರೆ (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } ಇಲ್ಲದಿದ್ದರೆ {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDtp = lhsRefp->dtypep()->skipRefp();
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ rhsDtp = rhsRefp->dtypep()->skipRefp();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isLhsAggregate = lhsDtp->isAggregateType();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isRhsAggregate = rhsDtp->isAggregateType();
        ಆದರೆ (!isLhsAggregate && !isRhsAggregate) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(lhsDtp, ಸರತಿ_DType) && VN_IS(rhsDtp, ಖಾಲಿಗಳು_ಸರತಿ_DType)) {
            ಹಿಂತಿರುಗಿ;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(ಸುಳ್ಳು),
                                      rhsDim = rhsDtp->dimensions(ಸುಳ್ಳು);
        // Check if unpacked array dimensions are matching
        ಆದರೆ (lhsDim.ಎರಡನೇ != rhsDim.ಎರಡನೇ) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ಹಿಂತಿರುಗಿ;
        }

        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* lhsDtpIterp = lhsDtp;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* rhsDtpIterp = rhsDtp;
        ಗಾಗಿ (uint32_t dim = 0; dim < rhsDim.ಎರಡನೇ; dim++) {
            // Associative arrays are compatible only with each other
            ಆದರೆ (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                ಹಿಂತಿರುಗಿ;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        ಆದರೆ (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    ಶೂನ್ಯ check_Class_ನಿಯೋಜಿಸು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* rhsp,
                          Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDTypep, ತಾರ್ಕಿಕ isOutputArg = ಸುಳ್ಳು) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsRawDTypep = lhsDTypep->skipRefp();
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ rhsRawDTypep = rhsp->dtypep()->skipRefp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                ಆದರೆ (is_ತಳ_Class_Recurse(dstp, srcp)) ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rhsp->isNull()) {
                ಹಿಂತಿರುಗಿ;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        ಆದರೆ (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
            relinker.relink(
                ಹೊಸ Ast_Cvt_Unpack_ಗಮ್ಯ_ಸರತಿ{rhsp->fileline(), VN_AS(rhsp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), lhsDTypep});
        }
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ similarDTypeRecurse(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ node1p,
                                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ node2p) {
        ಹಿಂತಿರುಗಿ node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    ಶೂನ್ಯ iterateCheckFileDesc(Ast_ಗಂಟು* parentp, Ast_ಗಂಟು* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, ಅಗಲ_VP{SELF, PRELIM}.p());
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = underp->ಕಂಡುಹಿಡಿ_UInt32_DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckReal(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ಕಂಡುಹಿಡಿ_Double_DType(), stage);
    }
    ಶೂನ್ಯ iterateCheckSigned8(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ಕಂಡುಹಿಡಿ_Signed8_DType(), stage);
    }
    ಶೂನ್ಯ iterateCheckSigned32(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ಕಂಡುಹಿಡಿ_Int_DType(), stage);
    }
    ಶೂನ್ಯ iterateCheckUInt32(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ಕಂಡುಹಿಡಿ_UInt32_DType(), stage);
    }
    ಶೂನ್ಯ iterateCheckDelay(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        ಆದರೆ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ಅಗಲ_VP{SELF, PRELIM}.p());
        }
        ಆದರೆ (stage & FINAL) {
            Ast_ಗಂಟು_DType* expDTypep;
            ಆದರೆ (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->ಕಂಡುಹಿಡಿ_Double_DType();
            } ಇಲ್ಲದಿದ್ದರೆ {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ newFl = ಹೊಸ File_ಸಾಲು{underp->fileline()};
                newFl->warnOff(V3_Error_ಸಂಕೇತ::WIDTHEXPAND, ನಿಜ);
                underp->fileline(newFl);
                expDTypep = parentp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, ಸುಳ್ಳು);
        }
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckString(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->ಕಂಡುಹಿಡಿ_String_DType(), stage);
    }
    ಶೂನ್ಯ iterateCheckTyped(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp,
                           Ast_ಗಂಟು_DType* expDTypep, Stage stage) {
        ಆದರೆ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ಅಗಲ_VP{expDTypep, PRELIM}.p());
        }
        ಆದರೆ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckTypedSelfPrelim(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp,
                                     Ast_ಗಂಟು_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        ಆದರೆ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ಅಗಲ_VP{SELF, PRELIM}.p());
        }
        ಆದರೆ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckIntegralSelf(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, ನಿಜ);
    }
    ಶೂನ್ಯ iterateCheckSelf(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Determ determ,
                          Stage stage, ತಾರ್ಕಿಕ integralOnly = ಸುಳ್ಳು) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ಆದರೆ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ಅಗಲ_VP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ) ? checkCvtUS(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), integralOnly) : underp;
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckSizedSelf(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Determ determ,
                               Stage stage, ತಾರ್ಕಿಕ deferUntyped = ಸುಳ್ಳು) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ಆದರೆ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ಅಗಲ_VP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ) ? checkCvtUS(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), ಸುಳ್ಳು) : underp;
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ checkDtp = expDTypep->skip_Ref_ಗಮ್ಯ_Enump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        ಆದರೆ (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterate_Check_ನಿಯೋಜಿಸು(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* rhsp, Stage stage,
                            Ast_ಗಂಟು_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        ಆದರೆ (AstConst* constp = VN_CAST(rhsp, Const)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                ಆದರೆ (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    ಆದರೆ (basicp->ಅಗಲ() == 8 && constp->ಅಂಕೆ().isFromString()) {
                        AstConsArray* newp = ಹೊಸ AstConsArray{
                            constp->fileline(), lhsDTypep,
                            ಹೊಸ AstConst{constp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}};
                        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ aindex : arrayp->declRange().seq_Lo_ಗಮ್ಯ_Hi()) {
                            ಪೂರ್ಣಾಂಕ cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_ಸಂಖ್ಯೆ selected{constp, 8};
                            selected.opSel(constp->ಅಂಕೆ(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            ಆದರೆ (!selected.isFourState()) {
                                ಆದರೆ (ಸ್ಥಿರಾಂಕ uint32_t c = selected.ಗಮ್ಯ_UInt()) {
                                    newp->valuep(aindex,
                                                 ಹೊಸ AstConst{constp->fileline(),
                                                              AstConst::Widthed_ಮೌಲ್ಯ{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        check_Class_ನಿಯೋಜಿಸು(parentp, side, rhsp, lhsDTypep);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lhs_ಹರಿವು = (VN_IS(parentp, ಗಂಟು_ನಿಯೋಜಿಸು)
                                && VN_IS(VN_AS(parentp, ಗಂಟು_ನಿಯೋಜಿಸು)->lhsp(), ಗಂಟು_ಹರಿವು));
        rhsp = iterateCheck(parentp, side, rhsp, ನಿಯೋಜಿಸು_3, FINAL, lhsDTypep,
                            lhs_ಹರಿವು ? EXTEND_OFF : EXTEND_LHS, ನಿಜ, ಹರಿವು_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (ಶೂನ್ಯ)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* convert_Unpacked_ಕ್ರಮ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_DType* dtypep) {
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ಆದರೆ (!arrayp || !arrayp->is_ಹಿಮ್ಮುಖ_ದ(exprp->dtypep())) ಹಿಂತಿರುಗಿ exprp;
        VNRelinker relinker;
        exprp->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        ಹಿಂತಿರುಗಿ newp;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isLogicalOperator(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        ಹಿಂತಿರುಗಿ VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    ಶೂನ್ಯ iterateCheckBool(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, ಅಗಲ_VP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ underVDTypep = underp ? underp->dtypep()->skipRefp() : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_ಹಿಂಭಾಗ(&linker);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstNeqD{parentp->fileline(), VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ),
                              ಹೊಸ AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_ಹಿಂಭಾಗ(&linker);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstNeqN{parentp->fileline(), VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ),
                              ಹೊಸ AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fix_ಅಗಲ_Reduce(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)), underp);  //ಬದಲಾಯಿತು
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(ಹೊಸ AstConst{parentp->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(underp), underp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ bad = ಅಗಲ_Bad(underp, parentp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType());
            ಆದರೆ (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->ಅಗಲ(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->ಅಗಲ()
                                             << (underp->ಅಗಲ() != underp->ಅಗಲ_Min()
                                                     ? " or " + cvt_ಗಮ್ಯ_Str(underp->ಅಗಲ_Min())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            ಆದರೆ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Stmtp = VN_CAST(underp, ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fix_ಅಗಲ_Reduce(ಅಭಿವ್ಯಕ್ತಿ_Stmtp->resultp());
                ಅಭಿವ್ಯಕ್ತಿ_Stmtp->dtypeFrom(ಅಭಿವ್ಯಕ್ತಿ_Stmtp->resultp());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (underp->ಅಗಲ() != 1) {
                VL_DO_DANGLING(fix_ಅಗಲ_Reduce(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)), underp);  //ಬದಲಾಯಿತು
            }
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ checkEnum(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಗುರಿ_DTypep,
                          ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಮೂಲ_DTypep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮೂಲ_Exprp) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ ಗುರಿ_Enump
            = VN_CAST(ಗುರಿ_DTypep->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType)) {
            ಸ್ಥಿರಾಂಕ VCastable castable
                = Ast_ಗಂಟು::computeCastable(ಗುರಿ_Enump, ಮೂಲ_DTypep, ಮೂಲ_Exprp);
            ಆದರೆ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << ಗುರಿ_DTypep->prettyDTypeNameQ() << " from "
                                             << ಮೂಲ_DTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warn_ಇನ್ನಷ್ಟು()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_ಗಂಟು* iterateCheck(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side, Ast_ಗಂಟು* underp, Determ determ,
                          Stage stage, Ast_ಗಂಟು_DType* expDTypep, Extend_ನಿಯಮ extend_ನಿಯಮ,
                          ತಾರ್ಕಿಕ warnOn = ನಿಜ, ಹರಿವು_Use ಹರಿವು_Use_2 = ಹರಿವು_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ಸ್ಥಿರಾಂಕ ಹರಿವು_Use ಮಗು_ಹರಿವು_Use
            = ಹರಿವು_Use_2 != ಹರಿವು_USE_NONE ? ಹರಿವು_Use_2 : ಹರಿವು_Use_For_Determ(determ);
        ಆದರೆ (rhsAccess.isReadOnly()) {
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                // A value only read is converted to a reversed expected type.
                underp = convert_Unpacked_ಕ್ರಮ(exprp, expDTypep);
            }
        }
        ಆದರೆ (VN_IS(underp, ಗಂಟು_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   ಅಗಲ_VP{expDTypep, FINAL, ಮಗು_ಹರಿವು_Use}.p());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   ಅಗಲ_VP{expDTypep, FINAL, ಮಗು_ಹರಿವು_Use}.p());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            underp = userIterateSubtreeReturnEdits(oldp, ಅಗಲ_VP{SELF, FINAL, ಮಗು_ಹರಿವು_Use}.p());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), ನಿಜ, expDTypep->ಅಗಲ());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, ಅಗಲ_VP{SELF, FINAL, ಮಗು_ಹರಿವು_Use}.p());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            underp = userIterateSubtreeReturnEdits(oldp, ಅಗಲ_VP{SELF, FINAL, ಮಗು_ಹರಿವು_Use}.p());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ underDtp = underp->dtypep()->skipRefp();
            ಆದರೆ (determ != ನಿಯೋಜಿಸು_3 && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstConst{underp->fileline(), AstConst::ಬಿಟ್_False_Erroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(underp), underp);
                underp = newp;
            }
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ expDtp = expDTypep->skipRefp();
            ಆದರೆ (determ == ನಿಯೋಜಿಸು_3 && VN_IS(expDtp, ಗಂಟು_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            ಆದರೆ (determ == ನಿಯೋಜಿಸು_3 && !isIfaceArrayDType(expDtp)
                && !unpacked_Array_Sizes_ಹೊಂದಿಕೆ(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ expBasicp = expDTypep->basicp();
            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ underBasicp = underp->dtypep()->basicp();
            ಆದರೆ (expBasicp && underBasicp) {
                ಆದರೆ (!m_enumItemp && !parentp->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::ENUMVALUE)
                    && warnOn) {
                    ಆದರೆ (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
                    }
                    ಆದರೆ (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, ಶೂನ್ಯಸೂಚಕ);
                    }
                }
                Ast_ಗಂಟು_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                ಆದರೆ (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, ಅಗಲ_VP{SELF, FINAL, ಮಗು_ಹರಿವು_Use}.p());
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (determ == ನಿಯೋಜಿಸು_3) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    ಆದರೆ (underp->isSigned() != subDTypep->isSigned()
                        || underp->ಅಗಲ() != subDTypep->ಅಗಲ()) {
                        subDTypep = parentp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(
                            std::max(subDTypep->ಅಗಲ(), underp->ಅಗಲ()),
                            std::max(subDTypep->ಅಗಲ_Min(), underp->ಅಗಲ_Min()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, ಅಗಲ_VP{subDTypep, FINAL, ಮಗು_ಹರಿವು_Use}.p());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, ಅಗಲ_VP{subDTypep, FINAL, ಮಗು_ಹರಿವು_Use}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                ಅಗಲ_Check_Sized(parentp, side, VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), expDTypep, extend_ನಿಯಮ,
                                warnOn);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ಸ್ಥಿರಾಂಕ AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                ಆದರೆ (VN_IS(underp, Const) && VN_AS(underp, Const)->ಅಂಕೆ().isNull()) {
                    // '= null' is ok
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_ಮೂಲಕ_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (expIfaceRefp->iface_ಮೂಲಕ_Cellp() != underIfaceRefp->iface_ಮೂಲಕ_Cellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_ಮೂಲಕ_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->iface_ಮೂಲಕ_Cellp()->prettyNameQ() << ").");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, ಅಗಲ_VP{expDTypep, FINAL, ಮಗು_ಹರಿವು_Use}.p());
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, ಅಗಲ_VP{expDTypep, FINAL, ಮಗು_ಹರಿವು_Use}.p());
            }
        }
        ಹಿಂತಿರುಗಿ underp;
    }

    ಶೂನ್ಯ
    ಅಗಲ_Check_Sized(Ast_ಗಂಟು* parentp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* side,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* underp,  // Node to be checked or have typecast added in front of
                    Ast_ಗಂಟು_DType* expDTypep, Extend_ನಿಯಮ extend_ನಿಯಮ, ತಾರ್ಕಿಕ warnOn = ನಿಜ) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ expBasicp = expDTypep->basicp();
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ underBasicp = underp->dtypep()->basicp();
        ಆದರೆ (expDTypep == underp->dtypep()) {
            ಹಿಂತಿರುಗಿ;  // Same type must match
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#ಆದರೆ VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#ಆದರೆ_ಅಂತ್ಯ
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ exp_ಅಗಲ = expDTypep->ಅಗಲ();
            ಪೂರ್ಣಾಂಕ exp_ಅಗಲ_Min = expDTypep->ಅಗಲ_Min();
            ಆದರೆ (exp_ಅಗಲ_Min == 0) exp_ಅಗಲ_Min = exp_ಅಗಲ;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ bad = ಅಗಲ_Bad(underp, expDTypep);
            ಆದರೆ ((bad || underp->ಅಗಲ() != exp_ಅಗಲ) && fixAutoExtend(underp /*ref*/, exp_ಅಗಲ)) {
                underp = ಶೂನ್ಯಸೂಚಕ;  // Changes underp
                ಹಿಂತಿರುಗಿ;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            ಆದರೆ (VN_IS(parentp->backp(), Cast_ಗಾತ್ರ)) warnOn = ಸುಳ್ಳು;
            ಆದರೆ (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = ಸುಳ್ಳು;
            ಆದರೆ (VN_IS(underp, Const) && VN_AS(underp, Const)->ಅಂಕೆ().isFromString()
                && exp_ಅಗಲ > underp->ಅಗಲ()
                && (((exp_ಅಗಲ - underp->ಅಗಲ()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = ಸುಳ್ಳು;
            }
            ಆದರೆ ((VN_IS(parentp, ಕೂಡಿಸು_2) && underp->ಅಗಲ() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->ಅಗಲ() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = ಸುಳ್ಳು;
            }
            ಆದರೆ (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    exp_ಅಗಲ, underp->ಅಗಲ(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << exp_ಅಗಲ
                        << (exp_ಅಗಲ != exp_ಅಗಲ_Min ? " or " + cvt_ಗಮ್ಯ_Str(exp_ಅಗಲ_Min) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->ಅಗಲ()
                        << (underp->ಅಗಲ() != underp->ಅಗಲ_Min()
                                ? " or " + cvt_ಗಮ್ಯ_Str(underp->ಅಗಲ_Min())
                                : "")
                        << " bits.");
            }
            ಆದರೆ (bad || underp->ಅಗಲ() != exp_ಅಗಲ) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* assignp = VN_CAST(parentp, ಗಂಟು_ನಿಯೋಜಿಸು);
                ಸ್ಥಿರಾಂಕ AstPin* pinp = VN_CAST(parentp, Pin);
                ಆದರೆ (assignp && VN_IS(assignp->lhsp(), ಗಂಟು_ಹರಿವು)) {
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pinp && pinp->modVarp()->direction() != VDirection::ಒಳಹರಿವು) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (assignp && VN_IS(underp, ಗಂಟು_ಹರಿವು) && exp_ಅಗಲ > underp->ಅಗಲ()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಸರಿಸು_2 = exp_ಅಗಲ - underp->ಅಗಲ();
                    underp->unlink_Fr_ಹಿಂಭಾಗ(&linker);
                    AstExtend* ಸ್ಥಿರಾಂಕ widenedp = ಹೊಸ AstExtend{underp->fileline(), underp};
                    widenedp->did_ಅಗಲ(ನಿಜ);
                    // Shift left so zeros fill on the right
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ shiftedp = ಹೊಸ Ast_ಸರಿಸು_L{
                        underp->fileline(), widenedp,
                        ಹೊಸ AstConst{underp->fileline(), static_cast<uint32_t>(ಸರಿಸು_2)}, exp_ಅಗಲ};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    VL_DO_DANGLING(fix_ಅಗಲ_Extend(underp, expDTypep, extend_ನಿಯಮ), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* checkCvtUS(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ತಾರ್ಕಿಕ fatal) {
        ಆದರೆ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            ಆದರೆ (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, ಸುಳ್ಳು, 32);
        }
        ಹಿಂತಿರುಗಿ nodep;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* spliceCvtD(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        ಆದರೆ (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&linker);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            ಆದರೆ (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = ಹೊಸ AstISToRD{nodep->fileline(), nodep};
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp = ಹೊಸ AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            ಹಿಂತಿರುಗಿ newp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ nodep;
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* spliceCvtS(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ತಾರ್ಕಿಕ warnOn, ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ಆದರೆ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&linker);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                ಆದರೆ (constp->isDouble() && V3_ಸಂಖ್ಯೆ::epsilonIntegral(constp->ಅಂಕೆ().ಗಮ್ಯ_Double())) {
                    warnOn = ಸುಳ್ಳು;
                }
            }
            ಆದರೆ (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtype_ಗಣ_ಬಿಟ್_Sized(ಅಗಲ, VSigning::SIGNED);
            ಹಿಂತಿರುಗಿ newp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ nodep;
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* spliceCvtString(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ಆದರೆ (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&linker);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            ಹಿಂತಿರುಗಿ newp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ nodep;
        }
    }
    Ast_ಗಂಟು_Biop* replaceWithUOrSVersion(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ಆದರೆ (signedFlavorNeeded == nodep->signedFlavor()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        ಆಯ್ಕೆ (nodep->type()) {
        ಸ್ಥಿತಿ VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿತಿ VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿತಿ VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿತಿ VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿತಿ VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿತಿ VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿತಿ VNType::ಕೂಡಿಸು_2: nodep->dtypeChgSigned(signedFlavorNeeded); ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿತಿ VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿತಿ VNType::ಸರಿಸು_L: nodep->dtypeChgSigned(signedFlavorNeeded); ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಪೂರ್ವನಿಯೋಜಿತ: ನಿಲ್ಲಿಸು;
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_Biop* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆಯ್ಕೆ (nodep->type()) {
        ಸ್ಥಿತಿ VNType::Gt: newp = ಹೊಸ AstGtS{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::GtS: newp = ಹೊಸ AstGt{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Gte: newp = ಹೊಸ AstGteS{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::GteS: newp = ಹೊಸ AstGte{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Lt: newp = ಹೊಸ AstLtS{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::LtS: newp = ಹೊಸ AstLt{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Lte: newp = ಹೊಸ AstLteS{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::LteS: newp = ಹೊಸ AstLte{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Div: newp = ಹೊಸ AstDivS{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::DivS: newp = ಹೊಸ AstDiv{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::ModDiv: newp = ಹೊಸ AstModDivS{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::ModDivS: newp = ಹೊಸ AstModDiv{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Mul: newp = ಹೊಸ AstMulS{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::MulS: newp = ಹೊಸ AstMul{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::ಸರಿಸು_R: newp = ಹೊಸ Ast_ಸರಿಸು_RS{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::ಸರಿಸು_RS: newp = ಹೊಸ Ast_ಸರಿಸು_R{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            ನಿಲ್ಲಿಸು;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ newp;
    }
    Ast_ಗಂಟು_Biop* replaceWithDVersion(Ast_ಗಂಟು_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ಆದರೆ (nodep->doubleFlavor()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_Biop* newp = ಶೂನ್ಯಸೂಚಕ;
        // No width change on output;...                // All below have bool or double outputs
        ಆಯ್ಕೆ (nodep->type()) {
        ಸ್ಥಿತಿ VNType::ಕೂಡಿಸು_2: newp = ಹೊಸ Ast_ಕೂಡಿಸು_D{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Sub: newp = ಹೊಸ AstSubD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Pow: newp = ಹೊಸ AstPowD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Eq:
        ಸ್ಥಿತಿ VNType::EqCase: newp = ಹೊಸ AstEqD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Neq:
        ಸ್ಥಿತಿ VNType::NeqCase: newp = ಹೊಸ AstNeqD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Gt:
        ಸ್ಥಿತಿ VNType::GtS: newp = ಹೊಸ AstGtD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Gte:
        ಸ್ಥಿತಿ VNType::GteS: newp = ಹೊಸ AstGteD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Lt:
        ಸ್ಥಿತಿ VNType::LtS: newp = ಹೊಸ AstLtD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Lte:
        ಸ್ಥಿತಿ VNType::LteS: newp = ಹೊಸ AstLteD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Div:
        ಸ್ಥಿತಿ VNType::DivS: newp = ಹೊಸ AstDivD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Mul:
        ಸ್ಥಿತಿ VNType::MulS: newp = ಹೊಸ AstMulD{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ನಿಲ್ಲಿಸು;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ newp;
    }
    Ast_ಗಂಟು_Biop* replaceWithNVersion(Ast_ಗಂಟು_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        ಆದರೆ (nodep->stringFlavor()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_Biop* newp = ಶೂನ್ಯಸೂಚಕ;
        // No width change on output;...                // All below have bool or double outputs
        ಆಯ್ಕೆ (nodep->type()) {
        ಸ್ಥಿತಿ VNType::Eq:
        ಸ್ಥಿತಿ VNType::EqCase:
        ಸ್ಥಿತಿ VNType::EqWild: newp = ಹೊಸ AstEqN{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Neq:
        ಸ್ಥಿತಿ VNType::NeqCase:
        ಸ್ಥಿತಿ VNType::NeqWild: newp = ಹೊಸ AstNeqN{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Gt:
        ಸ್ಥಿತಿ VNType::GtS: newp = ಹೊಸ AstGtN{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Gte:
        ಸ್ಥಿತಿ VNType::GteS: newp = ಹೊಸ AstGteN{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Lt:
        ಸ್ಥಿತಿ VNType::LtS: newp = ಹೊಸ AstLtN{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNType::Lte:
        ಸ್ಥಿತಿ VNType::LteS: newp = ಹೊಸ AstLteN{fl, lhsp, rhsp}; ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            ನಿಲ್ಲಿಸು;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ newp;
    }
    Ast_ಗಂಟು_Uniop* replaceWithDVersion(Ast_ಗಂಟು_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ಆದರೆ (nodep->doubleFlavor()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_Uniop* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆಯ್ಕೆ (nodep->type()) {
        ಸ್ಥಿತಿ VNType::Negate: newp = ಹೊಸ AstNegateD{fl, lhsp}; ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ನಿಲ್ಲಿಸು;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    ಶೂನ್ಯ replaceWithSFormat(Ast_ವಿಧಾನ_Call* nodep, ಸ್ಥಿರಾಂಕ string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ಸ್ಥಿರಾಂಕ argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), VarRef);
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            nodep->fileline(), fromp,
            ಹೊಸ AstSFormatF{nodep->fileline(), format, ಸುಳ್ಳು, argp->exprp()->unlink_Fr_ಹಿಂಭಾಗ()}};
        fromp->access(VAccess::WRITE);
        ತಳ್ಳು_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_ಗಂಟು_DType* iterateEditMoveDTypep(Ast_ಗಂಟು* parentp, Ast_ಗಂಟು_DType* dtnodep) {
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
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ child1 = (parentp->get_ಮಗು_DTypep() == dtnodep);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ child2 = (parentp->getChild2DTypep() == dtnodep);
        ಆದರೆ (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, ಶೂನ್ಯಸೂಚಕ), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->get_ಮಗು_DTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->did_ಅಗಲ(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlink_Fr_ಹಿಂಭಾಗ();
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(dtnodep);
        }
        ಆದರೆ (!dtnodep->did_ಅಗಲ()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, ಶೂನ್ಯಸೂಚಕ);
            UASSERT_OBJ(dtnodep->did_ಅಗಲ(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        ಹಿಂತಿರುಗಿ dtnodep;
    }

    AstConst* dimension_ಮೌಲ್ಯ(File_ಸಾಲು* fileline, Ast_ಗಂಟು_DType* nodep, VAttrType attrType,
                             ಪೂರ್ಣಾಂಕ dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_ಗಂಟು_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                declRange = adtypep->declRange();
                ಆದರೆ (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                ಮುಂದುವರಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ adtypep
                       = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                ನಿಲ್ಲಿಸು;  // Sub elements don't look like arrays and can't iterate into
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, BasicDType)) {
                ಆದರೆ (adtypep->isRanged()) declRange = adtypep->declRange();
                ನಿಲ್ಲಿಸು;
            }
            ನಿಲ್ಲಿಸು;  // LCOV_EXCL_LINE
        }
        AstConst* valp = ಶೂನ್ಯಸೂಚಕ;  // If nullptr, construct from val
        ಪೂರ್ಣಾಂಕ val = 0;
        ಆಯ್ಕೆ (attrType) {
        ಸ್ಥಿತಿ VAttrType::DIM_ಬಿಟ್ಟುಗಳು: {
            ಪೂರ್ಣಾಂಕ ಬಿಟ್ಟುಗಳು = 1;
            ಆಗ (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                    ಬಿಟ್ಟುಗಳು *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    ಮುಂದುವರಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ adtypep
                           = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
                    ಬಿಟ್ಟುಗಳು *= adtypep->isStreamableFixedAggregate() ? adtypep->ಅಗಲ_ಹರಿವು()
                                                                  : adtypep->ಅಗಲ();
                    ನಿಲ್ಲಿಸು;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, BasicDType)) {
                    ಬಿಟ್ಟುಗಳು *= adtypep->ಅಗಲ();
                    ನಿಲ್ಲಿಸು;
                }
                ನಿಲ್ಲಿಸು;
            }
            ಆದರೆ (dim == 0) {
                val = 0;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dim == 1 && !declRange.ranged()
                       && ಬಿಟ್ಟುಗಳು == 1) {  // $bits should be sane for non-arrays
                val = nodep->ಅಗಲ();
            } ಇಲ್ಲದಿದ್ದರೆ {
                val = ಬಿಟ್ಟುಗಳು;
            }
            ನಿಲ್ಲಿಸು;  // LCOV_EXCL_LINE
        }
        ಸ್ಥಿತಿ VAttrType::DIM_ಮೇಲಿನ: val = !declRange.ranged() ? 0 : declRange.hi(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VAttrType::DIM_ಎಡ: val = !declRange.ranged() ? 0 : declRange.ಎಡ(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VAttrType::DIM_ಕೆಳಗಿನ: val = !declRange.ranged() ? 0 : declRange.lo(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VAttrType::DIM_ಬಲ: val = !declRange.ranged() ? 0 : declRange.ಬಲ(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VAttrType::DIM_ಗಾತ್ರ: val = !declRange.ranged() ? 0 : declRange.elements(); ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: nodep->v3fatalSrc("Missing DIM ATTR type case"); ನಿಲ್ಲಿಸು;
        }
        ಆದರೆ (!valp) valp = ಹೊಸ AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvt_ಗಮ್ಯ_Hex(dtypep) << "," << dim
                                << ")=" << valp);
        ಹಿಂತಿರುಗಿ valp;
    }
    AstVarRef* dimensionVarRefp(Ast_ಗಂಟು_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_ಕೋಷ್ಟಕ_Map[std::make_pair(nodep, attrType)];
        ಆದರೆ (cachedpr) ಹಿಂತಿರುಗಿ cachedpr->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);

        Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ vardtypep = ಹೊಸ AstUnpackArrayDType{
            nodep->fileline(), nodep->ಕಂಡುಹಿಡಿ_Int_DType(), ಹೊಸ AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(vardtypep);
        userIterate(vardtypep, ಅಗಲ_VP{SELF, BOTH}.p());
        AstConsArray* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsArray{nodep->fileline(), vardtypep};
        ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_ಮೌಲ್ಯ(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::ಕಂಡುಹಿಡಿ(consp);
        VL_DO_DANGLING(consp->delete_ವೃಕ್ಷ(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->did_ಅಗಲ(ನಿಜ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_ಅಗಲ(ನಿಜ);
        ತಳ್ಳು_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ಹಿಂತಿರುಗಿ cachedpr->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
    }
    ಸ್ಥಿರ uint64_t enum_Max_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* errNodep, ಸ್ಥಿರಾಂಕ AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            ಆದರೆ (!vconstp->ಅಂಕೆ().isAnyXZ() && vconstp->ಗಮ್ಯ_UQuad() >= maxval)
                maxval = vconstp->ಗಮ್ಯ_UQuad();
        }
        ಆದರೆ (adtypep->itemsp()->ಅಗಲ() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            ಹಿಂತಿರುಗಿ 64;
        }
        ಹಿಂತಿರುಗಿ maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ಸ್ಥಿರಾಂಕ nodep, VAttrType attrType, ತಾರ್ಕಿಕ assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_ಕೋಷ್ಟಕ_Map[std::make_pair(nodep, attrType)];
        ಆದರೆ (cachedpr) ಹಿಂತಿರುಗಿ cachedpr->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು_DType* basep;
        ಆದರೆ (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->ಕಂಡುಹಿಡಿ_String_DType();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType();
        } ಇಲ್ಲದಿದ್ದರೆ {
            basep = nodep->dtypep();
        }
        Ast_ಗಂಟು_DType* vardtypep;
        ಆದರೆ (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = ಹೊಸ AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } ಇಲ್ಲದಿದ್ದರೆ {
            vardtypep = ಹೊಸ AstUnpackArrayDType{flp, basep, ಹೊಸ AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (attrType == VAttrType::ENUM_NAME) {
            defaultp = ಹೊಸ AstConst{flp, AstConst::String{}, ""};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (attrType == VAttrType::ENUM_ಮುಂದಿನ || attrType == VAttrType::ENUM_PREV) {
            defaultp = ಹೊಸ AstConst{flp, V3_ಸಂಖ್ಯೆ{nodep, nodep->ಅಗಲ(), 0}};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (attrType == VAttrType::ENUM_VALID) {
            defaultp = ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}};
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಮೌಲ್ಯಗಳು;
        {
            uint64_t ordinal = 0;
            Ast_Enum_ಅಂಶ* ಸ್ಥಿರಾಂಕ firstp = nodep->itemsp();
            ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* prevp = firstp;  // Prev must start with last item
            ಆಗ (prevp->nextp()) prevp = VN_AS(prevp->nextp(), Enum_ಅಂಶ);
            ಗಾಗಿ (Ast_Enum_ಅಂಶ* itemp = firstp; itemp;) {
                Ast_Enum_ಅಂಶ* ಸ್ಥಿರಾಂಕ nextp = VN_AS(itemp->nextp(), Enum_ಅಂಶ);
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                ಆದರೆ (!vconstp->ಅಂಕೆ().isAnyXZ()) {  // Can 2-state runtime decode
                    ಸ್ಥಿರಾಂಕ uint64_t i = nodep->isWide() ? ++ordinal : vconstp->ಗಮ್ಯ_UQuad();
                    ಆದರೆ (attrType == VAttrType::ENUM_NAME) {
                        ಮೌಲ್ಯಗಳು[i] = ಹೊಸ AstConst{flp, AstConst::String{},
                                                 V3_ಸಂಖ್ಯೆ::displayedEnumName(itemp)};
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (attrType == VAttrType::ENUM_ಮುಂದಿನ) {
                        ಮೌಲ್ಯಗಳು[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);  // A const
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (attrType == VAttrType::ENUM_PREV) {
                        ಮೌಲ್ಯಗಳು[i] = prevp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);  // A const
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (attrType == VAttrType::ENUM_VALID) {
                        ಮೌಲ್ಯಗಳು[i] = ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : ಮೌಲ್ಯಗಳು) {
                V3_ಸಂಖ್ಯೆ ಕೀಲಿ{nodep, nodep->ಅಗಲ()};
                ಕೀಲಿ.ಗಣ_Quad(itr.ಮೊದಲ);
                consp->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Cons_Assoc_ಅಂಶ{flp, ಹೊಸ AstConst{flp, ಕೀಲಿ}, itr.ಎರಡನೇ});
            }
            valuep = consp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            AstConsArray* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsArray{flp, vardtypep, defaultp};
            ಗಾಗಿ (uint64_t i = 0; i < (msbdim + 1); ++i) {
                ಆದರೆ (ಮೌಲ್ಯಗಳು[i]) consp->valuep(i, ಮೌಲ್ಯಗಳು[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::ಕಂಡುಹಿಡಿ(valuep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->did_ಅಗಲ(ನಿಜ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_ಅಗಲ(ನಿಜ);
        ತಳ್ಳು_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ಹಿಂತಿರುಗಿ cachedpr->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enumSelect(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        ಆದರೆ (attrType == VAttrType::ENUM_NAME) {
            ಆದರೆ (nodep->is_ಶುದ್ಧ()) nodep = V3Const::constifyEdit(nodep);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->ಅಂಕೆ().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ newp;
            }
        }
        ಆದರೆ (attrType == VAttrType::ENUM_NAME && !nodep->is_ಶುದ್ಧ() && (m_modep || m_ftaskp)) {
            Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ capturep = newEnumCapture(nodep, adtypep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = capturep->resultp()->unlink_Fr_ಹಿಂಭಾಗ();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtype_ಗಣ_String();
            ಹಿಂತಿರುಗಿ capturep;
        }
        ಆದರೆ (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp = ಹೊಸ AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(itemp->valuep(), Const);
                ಆದರೆ (constp->ಅಂಕೆ().isAnyXZ()) ಮುಂದುವರಿ;
                indexp = ಹೊಸ AstCond{nodep->fileline(),
                                     ಹೊಸ AstEq{nodep->fileline(), nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                               itemp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)},
                                     ಹೊಸ AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ಸ್ಥಿರಾಂಕ tabRefp = enumVarRefp(adtypep, attrType, ಸುಳ್ಳು, ordinal);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtype_ಗಣ_String();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ newp;
        }
        ಸ್ಥಿರಾಂಕ uint64_t msbdim = enum_Max_ಮೌಲ್ಯ(nodep, adtypep);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ assoc = msbdim > ENUM_LOOKUP_ಬಿಟ್ಟುಗಳು;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
        ಆದರೆ (assoc) {
            AstVarRef* ಸ್ಥಿರಾಂಕ tabRefp = enumVarRefp(adtypep, attrType, ನಿಜ, 0);
            newp = ಹೊಸ AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ selwidth = V3_ಸಂಖ್ಯೆ::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ಸ್ಥಿರಾಂಕ tabRefp
                = enumVarRefp(adtypep, attrType, ಸುಳ್ಳು, (1ULL << selwidth) - 1);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->ಅಗಲ() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? ಹೊಸ AstGt{nodep->fileline(), nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                  ಹೊಸ AstConst{nodep->fileline(),
                                               V3_ಸಂಖ್ಯೆ{nodep, adtypep->ಅಗಲ(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : ಶೂನ್ಯಸೂಚಕ;
            newp = ಹೊಸ AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                ಹೊಸ AstSel{nodep->fileline(), nodep, 0, selwidth}};
            ಆದರೆ (boundp) {
                newp = ಹೊಸ AstCond{nodep->fileline(), boundp,
                                   ಹೊಸ AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        ಆದರೆ (attrType == VAttrType::ENUM_NAME) {
            newp->dtype_ಗಣ_String();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (attrType == VAttrType::ENUM_VALID) {
            newp->dtype_ಗಣ_ಬಿಟ್();
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        ಹಿಂತಿರುಗಿ newp;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enumTestValid(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valp, AstEnumDType* enumDtp) {
        ಸ್ಥಿರಾಂಕ uint64_t maxval = enum_Max_ಮೌಲ್ಯ(valp, enumDtp);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ assoc = maxval > ENUM_LOOKUP_ಬಿಟ್ಟುಗಳು;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* testp = ಶೂನ್ಯಸೂಚಕ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl_novalue = ಹೊಸ File_ಸಾಲು{valp->fileline()};
        fl_novalue->warnOff(V3_Error_ಸಂಕೇತ::ENUMVALUE, ನಿಜ);
        fl_novalue->warnOff(V3_Error_ಸಂಕೇತ::CMPCONST, ನಿಜ);
        ಆದರೆ (assoc) {
            AstVarRef* ಸ್ಥಿರಾಂಕ tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ನಿಜ, 0);
            testp = ಹೊಸ AstAssocSel{fl_novalue, tabRefp, valp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ selwidth = V3_ಸಂಖ್ಯೆ::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ಸ್ಥಿರಾಂಕ tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ಸುಳ್ಳು, (1ULL << selwidth) - 1);
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl_nowidth = ಹೊಸ File_ಸಾಲು{fl_novalue};
            fl_nowidth->warnOff(V3_Error_ಸಂಕೇತ::ಅಗಲ_2, ನಿಜ);
            testp = ಹೊಸ AstCond{
                fl_novalue,
                ಹೊಸ AstGt{fl_nowidth, valp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                          ಹೊಸ AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                ಹೊಸ AstConst{fl_novalue, AstConst::ಬಿಟ್_False{}},
                ಹೊಸ AstArraySel{fl_novalue, tabRefp,
                                ಹೊಸ AstSel{fl_novalue, valp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), 0, selwidth}}};
        }
        ಹಿಂತಿರುಗಿ testp;
    }

    PatVecMap pat_ಸದಿಶ_Map(Ast_ನಮೂನೆ* nodep, ಸ್ಥಿರಾಂಕ VNumRange& range) {
        PatVecMap patmap;
        ಪೂರ್ಣಾಂಕ element = range.ಎಡ();
        ಗಾಗಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ಆದರೆ (patp->keyp()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = ನಮೂನೆ_Const_Keyp(patp)) {
                    ಆದರೆ (constp->ಅಂಕೆ().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        element = constp->ಗಮ್ಯ_SInt();
                    }
                }
            }
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ new_ನಮೂದು = patmap.emplace(element, patp).ಎರಡನೇ;
            ಆದರೆ (!new_ನಮೂದು) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.ಎಡ_ಗಮ್ಯ_ಬಲ_Inc();
        }
        ಹಿಂತಿರುಗಿ patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    ಶೂನ್ಯ patConcatConvertRecurse(Ast_ನಮೂನೆ* patternp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            AstPatMember* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstPatMember{nodep->fileline(), nodep->unlink_Fr_ಹಿಂಭಾಗ(), ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
            newp->isConcat(ನಿಜ);
            patternp->ಕೂಡಿಸು_Itemsp(newp);
        }
    }

    ಶೂನ್ಯ makeOpenArrayShell(Ast_ಗಂಟು_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ oldTaskp = nodep->taskp();
        oldTaskp->dpi_Open_ಪೋಷಕ_Inc();
        UASSERT_OBJ(!oldTaskp->dpi_Open_ಮಗು(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ newTaskp = oldTaskp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        newTaskp->dpi_Open_ಮಗು(ನಿಜ);
        newTaskp->dpi_Open_ಪೋಷಕ_Clear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvt_ಗಮ್ಯ_Str(oldTaskp->dpi_Open_ಪೋಷಕ()));
        oldTaskp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
            ಸ್ಥಿರಾಂಕ AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ pinp = argp->exprp();
            ಆದರೆ (!pinp) ಮುಂದುವರಿ;  // Argument error we'll find later
            ಆದರೆ (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_ಗಂಟು* nodep) {
                    ಆದರೆ (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    ತಾರ್ಕಿಕ markHasOpenArray(Ast_ಗಂಟು_FTask* nodep) {
        ತಾರ್ಕಿಕ hasOpen = ಸುಳ್ಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(ನಿಜ);
                    hasOpen = ನಿಜ;
                }
            }
        }
        ಹಿಂತಿರುಗಿ hasOpen;
    }
    ತಾರ್ಕಿಕ hasOpenArrayDTypeRecurse(Ast_ಗಂಟು_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        ಆದರೆ (VN_IS(nodep, UnsizedArrayDType)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (nodep->subDTypep()) ಹಿಂತಿರುಗಿ hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    Ast_ಗಂಟು_DType* dtypeNotIntAtomOrVecRecurse(Ast_ಗಂಟು_DType* nodep, ತಾರ್ಕಿಕ ranged = ಸುಳ್ಳು) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skip_Ref_ಗಮ್ಯ_Enump();
        ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(nodep, BasicDType)) {
            ಆದರೆ (ranged && (!dtp->is_ಬಿಟ್_Logic() || dtp->isRanged()))
                ಹಿಂತಿರುಗಿ dtp;  // Packed when already packed
            ಆದರೆ (dtp->keyword().isIntNumeric()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            ಹಿಂತಿರುಗಿ dtp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstPackArrayDType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(nodep, PackArrayDType)) {
            ಆದರೆ (ranged) ಹಿಂತಿರುಗಿ dtp;  // Packed when already packed
            ಹಿಂತಿರುಗಿ dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), ನಿಜ);
        }
        ಹಿಂತಿರುಗಿ nodep;
    }
    Ast_ಗಂಟು_DType* dtypeNot4StateIntegralRecurse(Ast_ಗಂಟು_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(nodep, BasicDType)) {
            ಆದರೆ (!dtp->keyword().isFourstate()) ಹಿಂತಿರುಗಿ dtp;
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(nodep, ಗಂಟು_Array_DType)) {
            ಹಿಂತಿರುಗಿ dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(nodep, ಗಂಟು_UOr_Struct_DType)) {
            ಗಾಗಿ (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                ಆದರೆ (badDtp) ಹಿಂತಿರುಗಿ badDtp;
            }
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ nodep;
    }
    ಶೂನ್ಯ checkForceReleaseLhs(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ selNodep = V3_ಅಗಲ::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    ಶೂನ್ಯ assert_At_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    ಶೂನ್ಯ assertAtStatement(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    ಶೂನ್ಯ checkConstantOrReplace(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ noFourState, ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const);
        ಆದರೆ (!constp || (noFourState && constp->ಅಂಕೆ().isFourState())) {
            nodep->v3error(ಸಂದೇಶ);
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ತಳ್ಳು_Deletep(nodep), nodep, constp);
            ಹಿಂತಿರುಗಿ;
        }
    }
    Ast_ಗಂಟು* ಗಂಟು_For_Unsized_Warning(Ast_ಗಂಟು* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        ಆದರೆ (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->ಅಗಲ_Sized()) {
            ಹಿಂತಿರುಗಿ nodep->op1p();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->ಅಗಲ_Sized()) {
            ಹಿಂತಿರುಗಿ nodep->op2p();
        }
        ಹಿಂತಿರುಗಿ nodep;  // By default return this
    }
    AstRefDType* check_Ref_ಗಮ್ಯ_Typedef_Recurse(Ast_ಗಂಟು* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        ಆದರೆ (VL_LIKELY(!nodep)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, RefDType)) {
            ಆದರೆ (refp->typedefp() == typedefp) ಹಿಂತಿರುಗಿ refp;
        }
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ refp = check_Ref_ಗಮ್ಯ_Typedef_Recurse(nodep->op1p(), typedefp)) ಹಿಂತಿರುಗಿ refp;
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ refp = check_Ref_ಗಮ್ಯ_Typedef_Recurse(nodep->op2p(), typedefp)) ಹಿಂತಿರುಗಿ refp;
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ refp = check_Ref_ಗಮ್ಯ_Typedef_Recurse(nodep->op3p(), typedefp)) ಹಿಂತಿರುಗಿ refp;
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ refp = check_Ref_ಗಮ್ಯ_Typedef_Recurse(nodep->op4p(), typedefp)) ಹಿಂತಿರುಗಿ refp;
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isIfaceArrayDType(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        ಆದರೆ (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        ಹಿಂತಿರುಗಿ irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    ಸ್ಥಿರ std::string unpacked_Array_ಗಾತ್ರ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        std::string ಗಾತ್ರ_2;
        dtypep = dtypep->skipRefp();
        ಆಗ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            ಆದರೆ (!ಗಾತ್ರ_2.ಖಾಲಿಗಳು()) ಗಾತ್ರ_2 += "x";
            ಗಾತ್ರ_2 += std::ಗಮ್ಯ_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        ಹಿಂತಿರುಗಿ ಗಾತ್ರ_2;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newIfaceArrayElement(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) {
        AstConsArray* ಸ್ಥಿರಾಂಕ consp = VN_CAST(fromp, ConsArray);
        ಆದರೆ (!consp || !isIfaceArrayDType(consp->dtypep())) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        ಆದರೆ (ಬಿಟ್_2 < 0 || ಬಿಟ್_2 >= arrp->elementsConst()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ consp->valuep(ಬಿಟ್_2)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_Iface_Array_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* refp, Ast_ಗಂಟು_DType* dtypep,
                                    ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = refp->fileline();
        AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        ಆದರೆ (!arrp) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_ಗಂಟು_Var_Ref* newp;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(refp, VarXRef)) {
                newp = ಹೊಸ AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp = ಹೊಸ AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, ಅಗಲ_VP{SELF, BOTH}.p());
            ಹಿಂತಿರುಗಿ newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsArray{flp, arrp};
        ಸ್ಥಿರಾಂಕ VNumRange range = arrp->declRange();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ i : range.seq_ಎಡ_ಗಮ್ಯ_ಬಲ()) {
            ಸ್ಥಿರಾಂಕ std::string s = name + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_ಮೌಲ್ಯ(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        ಹಿಂತಿರುಗಿ consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    ಶೂನ್ಯ replace_ವಿಭಜಿಸು_Iface_Array_Ref(Ast_ಗಂಟು_Var_Ref* nodep) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        UASSERT_OBJ(varp->is_Iface_Array_ವಿಭಜಿಸು(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        ಆದರೆ (varp->is_Iface_ಪೋಷಕ()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_ಮೌಲ್ಯ(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    ಶೂನ್ಯ expandIfaceArrayPinDimensions(AstPin* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* conp, ಸ್ಥಿರಾಂಕ std::string& suffix,
                                       AstVar*& elemVarpr) {
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        ಆದರೆ (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ಸ್ಥಿರಾಂಕ newp = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            ಆದರೆ (conp) newp->exprp(conp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newp);
            ಹಿಂತಿರುಗಿ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ಸ್ಥಿರಾಂಕ VNumRange range = arrp->declRange();
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ n = range.elements() - 1; n >= 0; --n) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ i = range.ಎಡ() + n * range.ಎಡ_ಗಮ್ಯ_ಬಲ_Inc();
            ಸ್ಥಿರಾಂಕ std::string s = suffix + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elemConnp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                ಆದರೆ (!conp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elements = conArrp->elementsConst();
                // Counted from lo
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                ಹಿಂತಿರುಗಿ resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    ಶೂನ್ಯ expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ಸ್ಥಿರಾಂಕ portp = nodep->modVarp();
        UASSERT_OBJ(portp->is_Iface_Array_ವಿಭಜಿಸು(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        ಗಾಗಿ (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        ಆದರೆ (exprp) ತಳ್ಳು_Deletep(exprp->unlink_Fr_ಹಿಂಭಾಗ());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpacked_Array_ಗಾತ್ರ(exprp->dtypep()) == unpacked_Array_ಗಾತ್ರ(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : ಶೂನ್ಯಸೂಚಕ, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_ಗಂಟು* userIterateSubtreeReturnEdits(Ast_ಗಂಟು* nodep, ಅಗಲ_VP* vup) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        ಹಿಂತಿರುಗಿ ret;
    }
    ಶೂನ್ಯ userIterate(Ast_ಗಂಟು* nodep, ಅಗಲ_VP* vup) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    ಶೂನ್ಯ user_Iterate_And_ಮುಂದಿನ(Ast_ಗಂಟು* nodep, ಅಗಲ_VP* vup) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->did_ಅಗಲ()) ಹಿಂತಿರುಗಿ;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate_And_ಮುಂದಿನ_Null(nodep);
    }
    ಶೂನ್ಯ userIterateChildren(Ast_ಗಂಟು* nodep, ಅಗಲ_VP* vup) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ userIterateChildrenBackwardsConst(Ast_ಗಂಟು* nodep, ಅಗಲ_VP* vup) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಅಗಲ_Visitor(ತಾರ್ಕಿಕ paramsOnly,  // [in] TRUE if we are considering parameters only.
                 ತಾರ್ಕಿಕ doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enum_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು{"__Venum"}
        , m_inside_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_ಗಂಟು* mainAcceptEdit(Ast_ಗಂಟು* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, ಅಗಲ_VP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ varp : m_ವಿಭಜಿಸು_Iface_Array_Varps) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(varp->unlink_Fr_ಹಿಂಭಾಗ()), varp);
        }
        m_ವಿಭಜಿಸು_Iface_Array_Varps.clear();
        ಹಿಂತಿರುಗಿ nodep;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಅಧೋ_As_Fixed_Aggregate(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        ಹಿಂತಿರುಗಿ dtypep->isStreamableFixedAggregate() && dtypep->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct();
    }
    ಶೂನ್ಯ ಹರಿವು_ನಿಯೋಜಿಸು_ಅಧೋ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        Ast_ಗಂಟು_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        ಆದರೆ (Ast_ಗಂಟು_ಹರಿವು* ಸ್ಥಿರಾಂಕ streamp = VN_CAST(nodep->rhsp(), ಗಂಟು_ಹರಿವು)) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lwidth = lhsDTypeSkippedRefp->ಅಗಲ_ಹರಿವು();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rwidth = streamp->lhsp()->dtypep()->skipRefp()->ಅಗಲ_ಹರಿವು();
            ಆದರೆ (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            ಆದರೆ (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), ಸರತಿ_DType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ಸರತಿ_DType)) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಸರತಿ_Element_ಗಾತ್ರ = streamp->lhsp()->dtypep()->subDTypep()->ಅಗಲ();
                UASSERT_OBJ(ಸರತಿ_Element_ಗಾತ್ರ <= lwidth, nodep, "LHS < RHS");
            }
            ಆದರೆ (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || ಅಧೋ_As_Fixed_Aggregate(lhsDTypeSkippedRefp)) {
                streamp->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->rhsp(
                    ಹೊಸ Ast_Cvt_Packed_ಗಮ್ಯ_Array{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        ಆದರೆ (Ast_ಗಂಟು_ಹರಿವು* ಸ್ಥಿರಾಂಕ streamp = VN_CAST(nodep->lhsp(), ಗಂಟು_ಹರಿವು)) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhs_ಹರಿವು_DTypep = streamp->lhsp()->dtypep()->skipRefp();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lwidth = lhs_ಹರಿವು_DTypep->ಅಗಲ_ಹರಿವು();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rwidth = rhsDTypep->ಅಗಲ_ಹರಿವು();
            ಆದರೆ (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            ಆದರೆ (ಅಧೋ_As_Fixed_Aggregate(lhs_ಹರಿವು_DTypep)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹರಿವು_Exprp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ dstp = streamp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                ಆದರೆ (VN_IS(streamp, ಹರಿವು_L)) {
                    streamp->lhsp(srcp);
                    streamp->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(
                        srcp->ಅಗಲ(), srcp->ಅಗಲ_Min(), VSigning::UNSIGNED,
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(ಹರಿವು_Exprp, srcp));
                    srcp = ಹರಿವು_Exprp;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (srcp->ಅಗಲ() > lwidth) {
                        srcp = ಹೊಸ AstSel{streamp->fileline(), srcp, srcp->ಅಗಲ() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(ಹರಿವು_Exprp), ಹರಿವು_Exprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(ಹೊಸ Ast_Cvt_Packed_ಗಮ್ಯ_Array{srcp->fileline(), srcp, lhs_ಹರಿವು_DTypep});
                nodep->dtypeFrom(dstp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->rhsp(ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Packed{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~ಅಗಲ_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Width class functions

ಶೂನ್ಯ V3_ಅಗಲ::ಅಗಲ(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ಸ್ಥಿರಾಂಕ ಅಗಲ_Clear_Visitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ಸ್ಥಿರಾಂಕ VDTypeNameScopes scopes{nodep};
        ಅಗಲ_Visitor visitor{ಸುಳ್ಳು, ಸುಳ್ಳು};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
        ಅಗಲ_Remove_Visitor rvisitor;
        (ಶೂನ್ಯ)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("width", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_ಗಂಟು* V3_ಅಗಲ::ಅಗಲ_Params_Edit(Ast_ಗಂಟು* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    ಅಗಲ_Visitor visitor{ನಿಜ, ಸುಳ್ಳು};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ಹಿಂತಿರುಗಿ nodep;
}

ಶೂನ್ಯ V3_ಅಗಲ::ಹರಿವು_ನಿಯೋಜಿಸು_ಅಧೋ_Edit(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    ಅಗಲ_Visitor visitor{ಸುಳ್ಳು, ಸುಳ್ಳು};
    visitor.ಹರಿವು_ನಿಯೋಜಿಸು_ಅಧೋ(nodep);
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
Ast_ಗಂಟು* V3_ಅಗಲ::ಅಗಲ_Generate_Params_Edit(
    Ast_ಗಂಟು* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    ಅಗಲ_Visitor visitor{ನಿಜ, ನಿಜ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ಹಿಂತಿರುಗಿ nodep;
}

Ast_ಗಂಟು* V3_ಅಗಲ::selectNonConstantRecurse(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ ಒಳ_Sel) {
    // If node has a non-constant select, return that select
    Ast_ಗಂಟು* resultp = ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, ಗಂಟು_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), ಒಳ_Sel);
        ಆದರೆ (resultp) ಹಿಂತಿರುಗಿ resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), ನಿಜ);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), ಒಳ_Sel);
        ಆದರೆ (resultp) ಹಿಂತಿರುಗಿ resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), ನಿಜ);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
        ಆದರೆ (ಒಳ_Sel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) ಹಿಂತಿರುಗಿ anodep;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, ಒಳ_Sel);
        ಆದರೆ (resultp) ಹಿಂತಿರುಗಿ resultp;
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, ಒಳ_Sel);
        ಆದರೆ (resultp) ಹಿಂತಿರುಗಿ resultp;
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, ಒಳ_Sel);
        ಆದರೆ (resultp) ಹಿಂತಿರುಗಿ resultp;
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, ಒಳ_Sel);
    }
    ಹಿಂತಿರುಗಿ resultp;
}
