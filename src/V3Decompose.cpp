// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Split arrays and structs into separate variables
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
// V3Decompose replaces variables of aggregate type (unpacked array, unpacked
// struct, multidimensional packed array, or packed struct) with one variable
// per element or member, called a component. This avoids false combinational
// loops (UNOPTFLAT) through the parts of a variable, and lets downstream passes
// optimize each component separately. Note specifically that one dimensional
// packed arrays (vectors, e.g. logic [31:0]) are not split into individual
// bits by this pass.
//
// Components that are themselves aggregates can be split further. The pass
// decides for the variable, and for each of its components at any depth,
// whether to split it.
//
// The pass proceeds in three steps:
//   - DecomposeRecord walks the netlist and records necessary information about
//     accesses to aggregate variables, without changing the tree.
//   - DecomposeDecision decides which variables and components to split, in one
//     iterative analysis of the data structures recorded by DecomposeRecord.
//   - DecomposeRewrite replaces references to, and expands assignments between
//     split variables and components, in a single rewrite across all constructs
//     involved.
// This structure ensures there is only a single traversal of the netlist, the
// complete splitting decision is made globally, independent of iteration order,
// and the rewriting is done after all decisions are made, so there is no need
// to re-analyze, or keep track of stale references anywhere.
//
// The core concept and data structure that enables the decomposition of the
// three steps is the 'place'. A 'place' represents a possible storage location:
// - either an original variable,
// - or one of its components, recursively.
// 'places' form a tree, with the original variable as the root. For example,
// the tree of 'places' involved for the aggregate variable 's' are:
//
//   struct packed {
//     logic [1:0][3:0] m;
//     logic [2:0] f;
//   } s [2];
//
//   s                      'place' for the unpacked array of 2 packed structs
//   +-- s[0]               'place' for element 0 of 's', a packed struct
//   |   +-- s[0].m         'place' for member 'm' of 's[0]', a packed array of 2 elements
//   |   |   +-- s[0].m[0]  'place' logic [3:0], a non splittable leaf
//   |   |   +-- s[0].m[1]  'place' logic [3:0], a non splittable leaf
//   |   +-- s[0].f         'place' logic [2:0], a non splittable leaf
//   +-- s[1]               Same subtree structure as 's[0]'
//       +-- s[1].m
//       |   +-- s[1].m[0]
//       |   +-- s[1].m[1]
//       +-- s[1].f
//
// A place is automatically split if it can be, and it wants to be.
// It can be split if all of the following are true:
//   - it is an aggregate variable
//   - the original variable is eligible for splitting (not public, etc.)
//   - its parent is split (if it is a component)
//   - it is not blocked, by being referenced other than in splittable constructs
// It wants to be split if any of the following are true:
//   - one of its components is addressed by a constant select
//   - the original variable is marked with split_var
//   - a copy assignment between it and another split place exists
// Note specifically that a reference outside a constant indexed select, or
// a copy assignment will block splitting, so effectively the variable is split
// mostly if it is only accessed via constant indexed selects, and it's only
// assigned whole (recursively for its components).
//
// E.g. this will split 'arr' into 'arr[0]' and 'arr[1]':
//
//   logic [6:0] arr [2];
//   assign arr[0] = in;
//   assign arr[1] = arr[0] + 7'd1;
//   assign out = arr[1];  // All indices are constant, so 'arr' is split
//
// This will not split 'arr', as it is indexed with a non-constant index:
//
//   logic [6:0] arr [2];
//   assign arr[0] = in;
//   assign arr[1] = arr[0] + 7'd1;
//   assign out = arr[i];  // 'i' is not a constant, so 'arr' is not split
//
// Copy assignments propagate splitting. An assignment between two places is
// recorded as a copy. When a place is split, its copies are split at the
// boundaries of its components. For example:
//
//   typedef struct packed {
//     logic [3:0] hi;
//     logic [3:0] lo;
//   } pair_t;
//   pair_t a, b;
//   assign b = {a.lo + 4'd1, in};
//   assign a = b;
//   assign out = a.hi;
//
// 'a' is split, as its components are selected. This splits the 'a = b' copy
// into copies between 'a.hi' and 'b[7:4]', and 'a.lo' and 'b[3:0]', each covering
// only one component of 'b', so 'b' is split too. The result is a list of
// assignments between the components of 'a' and 'b':
//
//   assign b__DOT__hi = a__DOT__lo + 4'd1;
//   assign b__DOT__lo = in;
//   assign a__DOT__hi = b__DOT__hi;
//   assign a__DOT__lo = b__DOT__lo;
//   assign out = a__DOT__hi;
//
// Which later passes can simplify into 'out = in + 4'd1', with intermediate
// variables discarded when possible.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Decompose.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3MemberMap.h"
#ضمن "V3SharedTmps.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

صنف Decompose_بنسټ VL_NOT_FINAL {
محمي:
    // TYPES

    // A component (element or member) of an aggregate type
    بنية Component final {
        Ast_غوټه_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        صحيح declIdx;  // The declared index of the element, if an array
        صحيح lsb;  // LSB in the whole value, if packed
        صحيح msb;  // MSB in the whole value, if packed
    };

    بنية ځای_پرځای_کړه;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    بنية Copy final {
        ځای_پرځای_کړه* otherp;  // The Place at the other end
        // Following only for packed places
        صحيح lsb;  // The first bit covered of this end
        صحيح otherLsb;  // The first bit covered of the other end
        صحيح پلنوالی;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    بنية ځای_پرځای_کړه final {
        ځای_پرځای_کړه* parentp = تش_اشاره;  // The Place this is a component of, nullptr if original
        ځای_پرځای_کړه* rootp = تش_اشاره;  // The original variable (root of the tree), root points to itself
        Ast_غوټه_DType* dtypep = تش_اشاره;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = تش_اشاره;
        // The component Places created so far, by index, sized on the first one created
        std::ویکټور<std::unique_ptr<ځای_پرځای_کړه>> childrenp;
        std::ویکټور<Copy> copies;  // The copies covering it, until split
        منطقي splittable = ریښتیا;  // Can be split (there is no reason not to)
        منطقي غواړي_وېشل = خطأ;  // Should be split (if possible, do split)
        منطقي وېشل = خطأ;  // Place was split
        منطقي queued = خطأ;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    بنية Select final {
        Ast_غوټه_بیان* exprp;  // The select
        ځای_پرځای_کړه* placep;  // The component
        صحيح lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    بنية Assignment final {
        Ast_غوټه_ټاکل* assp;  // The assignment
        ځای_پرځای_کړه* lPlacep;  // The Place the LHS addresses exactly, if any
        ځای_پرځای_کړه* rPlacep;  // The Place the RHS addresses exactly, if any
    };

عام:
    // The state of the pass, shared by the steps
    صنف State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ثابت VNUser4_دننه_Use m_user4_دننه_Use;

    عام:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_غوټه_DType, std::ویکټور<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, ځای_پرځای_کړه> places;
        std::ویکټور<ځای_پرځای_کړه*> rootps;  // The original Places, in creation order
        std::ویکټور<std::ویکټور<Select>> chains;  // Select chains addressing components
        std::ویکټور<Assignment> assignments;  // Assignments to expand if a side is split
    };

محمي:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    څرګند Decompose_بنسټ(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    ساكن منطقي isUnpacked(ثابت Ast_غوټه_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (VN_IS(dtypep, UnpackArrayDType)) أرجع ریښتیا;
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        أرجع structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    ساكن منطقي isPacked(ثابت Ast_غوټه_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            أرجع arrayp->subDTypep()->پلنوالی() > 1;
        }
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        أرجع structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    ساكن منطقي isSplittableType(ثابت Ast_غوټه_DType* dtypep) {
        إذا (isPacked(dtypep)) أرجع v3Global.opt.fDecomposePacked();
        إذا (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            إذا (ثابت AstStructDType* ثابت structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                لكل (ثابت AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    إذا (mp->valuep()) أرجع خطأ;
                }
            }
            أرجع v3Global.opt.fDecomposeUnpacked();
        }
        أرجع خطأ;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    ساكن Ast_غوټه_DType* dtype_د(ثابت Ast_غوټه_بیان* nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) أرجع refp->varp()->dtypep();
        أرجع nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ثابت std::ویکټور<Component>& dtypeComponents(Ast_غوټه_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::ویکټور<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        إذا (compsr.تشه()) {
            إذا (ثابت Ast_غوټه_Array_DType* ثابت arrayp = VN_CAST(dtypep, غوټه_Array_DType)) {
                ثابت منطقي packed = isPacked(dtypep);
                ثابت VNumRange range = arrayp->declRange();
                ثابت منطقي rev = packed && range.ascending();  // Only for naming components
                Ast_غوټه_DType* ثابت subp = arrayp->subDTypep();
                لكل (صحيح i = 0; i < range.elements(); ++i) {
                    ثابت صحيح lsb = packed ? i * subp->پلنوالی() : 0;
                    ثابت صحيح msb = packed ? lsb + subp->پلنوالی() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ثابت صحيح declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ټېل_شا({subp, تش_اشاره, declIdx, lsb, msb});
                }
            } وإلا {
                ثابت AstStructDType* ثابت structp = VN_AS(dtypep, StructDType);
                ثابت منطقي packed = isPacked(dtypep);
                لكل (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ثابت صحيح lsb = packed ? mp->lsb() : 0;
                    ثابت صحيح msb = packed ? lsb + mp->پلنوالی() - 1 : 0;
                    compsr.ټېل_شا({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::سرچپه(compsr.begin(), compsr.پای_ټکی());
            }
        }

        // The components of this data type
        أرجع compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    اندازه_t component_شاخص(Ast_غوټه_DType* dtypep, صحيح بټ_2) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ثابت std::ویکټور<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ثابت خپلکاری it = std::ښکتنی_bound(compsr.begin(), compsr.پای_ټکی(), بټ_2,
                                         [](ثابت Component& comp, صحيح b) {  //
                                             أرجع b > comp.msb;
                                         });
        أرجع it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    ځای_پرځای_کړه* component_د(ځای_پرځای_کړه* placep, اندازه_t idx) {
        ثابت std::ویکټور<Component>& comps = dtypeComponents(placep->dtypep);
        إذا (placep->childrenp.تشه()) placep->childrenp.resize(comps.اندازه());
        std::unique_ptr<ځای_پرځای_کړه>& childpr = placep->childrenp.at(idx);
        إذا (!childpr) {
            childpr = std::make_unique<ځای_پرځای_کړه>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->غواړي_وېشل = placep->rootp->vscp->varp()->attr_وېشل_Var();
        }
        أرجع childpr.get();
    }

    // Is 'nodep' cheap to clone
    ساكن منطقي isCheap(ثابت Ast_غوټه_بیان* nodep) {
        // Constants are cheap
        إذا (VN_IS(nodep, Const)) أرجع ریښتیا;
        // So are resets
        إذا (VN_IS(nodep, CReset)) أرجع ریښتیا;
        // Variable references are cheap for the most part
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            إذا (refp->varp()->isForced()) أرجع خطأ;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            إذا (refp->varp()->isSc()) أرجع خطأ;
            // Otherwise the path rooted here is cheap
            أرجع ریښتیا;
        }
        // Bit selects only with a constant LSB
        إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            أرجع VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        إذا (ثابت AstArraySel* ثابت selp = VN_CAST(nodep, ArraySel)) {
            أرجع isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {  //
            أرجع isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        إذا (ثابت AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {  //
            أرجع isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        أرجع خطأ;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

صنف Decompose_ریکارډ final : عام VNVisitorConst, عام Decompose_بنسټ {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_غوټه_ټاکل* m_assignp = تش_اشاره;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ساكن فراغ warn_No_وېشل(AstVar* varp, ثابت Ast_غوټه* wherep, ثابت حرف* reasonp) {
        // Only warn if user requested splitting
        إذا (!varp->attr_وېشل_Var()) أرجع;
        // A packed variable might still be split by V3BitBlast, which warns if not
        إذا (isPacked(varp->dtypep())) أرجع;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3_Error_کوډ::SPLITVAR, ریښتیا);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    ساكن منطقي isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        إذا (!varp->user1()) {
            ثابت منطقي eligible = [&]() {
                // Check data type
                إذا (!isSplittableType(varp->dtypep())) أرجع خطأ;
                // Check properties
                ثابت حرف* ثابت reasonp = varp->cannot_وېشل_ډول_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                إذا (reasonp) warn_No_وېشل(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                أرجع !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        أرجع varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    ځای_پرځای_کړه* ځای_پرځای_کړه_د(ثابت AstVarRef* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        إذا (!isEligible(vscp->varp())) أرجع تش_اشاره;
        ځای_پرځای_کړه& ځای_پرځای_کړه_2 = m_state.places(vscp);
        إذا (!ځای_پرځای_کړه_2.rootp) {
            ځای_پرځای_کړه_2.rootp = &ځای_پرځای_کړه_2;
            ځای_پرځای_کړه_2.dtypep = vscp->varp()->dtypep();
            ځای_پرځای_کړه_2.vscp = vscp;
            ځای_پرځای_کړه_2.غواړي_وېشل = vscp->varp()->attr_وېشل_Var();
            m_state.rootps.ټېل_شا(&ځای_پرځای_کړه_2);
        }
        أرجع &ځای_پرځای_کړه_2;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    ساكن فراغ بلاک(ځای_پرځای_کړه* placep, ثابت Ast_غوټه* wherep) {
        placep->splittable = خطأ;
        // Warn on the variable only, a component is split as far as possible, as requested
        إذا (!placep->parentp) {
            warn_No_وېشل(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    ځای_پرځای_کړه* resolveChain(Ast_غوټه_بیان* exprp, std::ویکټور<Select>& chain) {
        // Array selects correspond one to one with a component select
        إذا (AstArraySel* ثابت selp = VN_CAST(exprp, ArraySel)) {
            إذا (ځای_پرځای_کړه* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ثابت AstConst* ثابت bitp = VN_CAST(selp->bitp(), Const);
                إذا (bitp && bitp->منزل_UQuad() < dtypeComponents(fromp->dtypep).اندازه()) {
                    ځای_پرځای_کړه* ثابت childp = component_د(fromp, bitp->منزل_UQuad());
                    fromp->غواړي_وېشل = ریښتیا;
                    chain.ټېل_شا({selp, childp, 0});
                    أرجع childp;
                }
                // Otherwise block splitting of the Place
                بلاک(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            أرجع تش_اشاره;
        }
        // Struct selects correspond one to one with a component select
        إذا (AstStructSel* ثابت selp = VN_CAST(exprp, StructSel)) {
            إذا (ځای_پرځای_کړه* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                إذا (!isUnpacked(fromp->dtypep)) أرجع تش_اشاره;
                AstStructDType* ثابت structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                إذا (!structp->user1_سېټ_Once()) {
                    ثابت std::ویکټور<Component>& compsr = dtypeComponents(structp);
                    لكل (اندازه_t i = 0; i < compsr.اندازه(); ++i) compsr[i].memberp->user1(i);
                }
                ثابت Ast_غوټه* ثابت memberp = m_memberMap.پیدا_کړه_Member(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                ځای_پرځای_کړه* ثابت childp = component_د(fromp, memberp->user1());
                fromp->غواړي_وېشل = ریښتیا;
                chain.ټېل_شا({selp, childp, 0});
                أرجع childp;
            }
            أرجع تش_اشاره;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        إذا (AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            ځای_پرځای_کړه* placep = resolveChain(selp->fromp(), chain);
            إذا (!placep) {
                iterateConst(selp->lsbp());
                أرجع تش_اشاره;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
            إذا (!lsbp || lsbp->منزل_SInt() + selp->پلنوالی_Const() > placep->dtypep->پلنوالی()) {
                بلاک(placep, selp->fromp());
                iterateConst(selp->lsbp());
                أرجع تش_اشاره;
            }
            // Descend into the Place containing the bits, until exact
            صحيح lsb = lsbp->منزل_SInt();
            صحيح msb = lsb + selp->پلنوالی_Const() - 1;
            طالما (lsb != 0 || msb != placep->dtypep->پلنوالی() - 1) {
                // Within a component of a type not split
                إذا (!isPacked(placep->dtypep)) أرجع تش_اشاره;
                ثابت اندازه_t idx = component_شاخص(placep->dtypep, lsb);
                // Crossing components prevents splitting
                إذا (idx != component_شاخص(placep->dtypep, msb)) {
                    بلاک(placep, selp->fromp());
                    أرجع تش_اشاره;
                }
                placep->غواړي_وېشل = ریښتیا;
                ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = component_د(placep, idx);
                chain.ټېل_شا({selp, placep, lsb});
            }
            أرجع placep;
        }
        // Base case
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) أرجع ځای_پرځای_کړه_د(refp);
        // Not a select chain
        iterateConst(exprp);
        أرجع تش_اشاره;
    }

    // 'nodep' references the Place 'placep' whole
    فراغ referenced_ټولیز(Ast_غوټه_بیان* nodep, ځای_پرځای_کړه* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        إذا (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            أرجع;
        }

        // Otherwise must block splitting of this Place
        بلاک(placep, nodep);
    }

    // Visit a select expression
    فراغ لیدنه_Select(Ast_غوټه_بیان* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::ویکټور<Select> chain;
        ځای_پرځای_کړه* ثابت placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        إذا (placep) referenced_ټولیز(nodep, placep);
        // Record the chain for the rewriting phase
        إذا (!chain.تشه()) m_state.chains.ټېل_شا(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    ساكن منطقي hasReversedRange(ثابت Ast_غوټه_DType* aDTypep, ثابت Ast_غوټه_DType* bDTypep) {
        ثابت AstUnpackArrayDType* ثابت aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        إذا (!aArrayp || !bArrayp) أرجع خطأ;
        إذا (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) أرجع ریښتیا;
        أرجع hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    فراغ لیدنه_Assignment(Ast_غوټه_ټاکل* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ثابت منطقي reversed = hasReversedRange(dtype_د(nodep->lhsp()), dtype_د(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? تش_اشاره : nodep;
        iterateChildrenConst(nodep);
        إذا (!m_assignp) أرجع;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp();
        ځای_پرځای_کړه* ثابت lPlacep = lhsp->user1u().منزل<ځای_پرځای_کړه*>();
        ځای_پرځای_کړه* ثابت rPlacep = rhsp->user1u().منزل<ځای_پرځای_کړه*>();

        // Is it a splittable assignment
        ثابت منطقي splittable = [&]() {
            // Assignment between two Places is splittable
            إذا (lPlacep && rPlacep) أرجع ریښتیا;
            // If the LHS is a Place, the RHS must be a splittable expression
            إذا (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                إذا (isPacked(lPlacep->dtypep)) أرجع ریښتیا;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                أرجع isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            إذا (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                إذا (isPacked(rPlacep->dtypep)) أرجع ریښتیا;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                أرجع isCheap(lhsp);
            }
            // Otherwise not splittable
            أرجع خطأ;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        إذا (!splittable) {
            إذا (rPlacep) بلاک(rPlacep, rhsp);
            إذا (lPlacep) بلاک(lPlacep, lhsp);
            أرجع;
        }

        // If both sides are places, record the whole copies for the Decision phase
        إذا (lPlacep && rPlacep) {
            ثابت صحيح پلنوالی = lPlacep->dtypep->پلنوالی();  // Unused when unpacked
            lPlacep->copies.ټېل_شا({rPlacep, 0, 0, پلنوالی});
            rPlacep->copies.ټېل_شا({lPlacep, 0, 0, پلنوالی});
        }
        // Record for the rewriting phase
        m_state.assignments.ټېل_شا({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_DType*) override {}  // No references in data types
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    فراغ لیدنه(AstArraySel* nodep) override { لیدنه_Select(nodep); }
    فراغ لیدنه(AstStructSel* nodep) override { لیدنه_Select(nodep); }
    فراغ لیدنه(AstSel* nodep) override { لیدنه_Select(nodep); }
    // VISITORS - Assignments recordable as Copy
    فراغ لیدنه(Ast_ټاکل* nodep) override { لیدنه_Assignment(nodep); }
    فراغ لیدنه(Ast_ټاکل_W* nodep) override { لیدنه_Assignment(nodep); }
    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override { لیدنه_Assignment(nodep); }
    // VISITORS - References to Places
    فراغ لیدنه(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        إذا (ځای_پرځای_کړه* ثابت placep = ځای_پرځای_کړه_د(nodep)) referenced_ټولیز(nodep, placep);
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ثابت varp = nodep->varp();
        إذا (!isEligible(varp)) أرجع;
        varp->user1(2);  // Mark not eligible
        warn_No_وېشل(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    Decompose_ریکارډ(State& state, AstNetlist* netlistp)
        : Decompose_بنسټ{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        لكل (ځای_پرځای_کړه* ثابت placep : m_state.rootps) {
            إذا (!isEligible(placep->vscp->varp())) placep->splittable = خطأ;
        }
    }

عام:
    ساكن فراغ apply(State& state, AstNetlist* netlistp) { Decompose_ریکارډ{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

صنف DecomposeDecision final : عام Decompose_بنسټ {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;

    // STATE
    std::ویکټور<ځای_پرځای_کړه*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::ویکټور<AstVar*>> m_وېشل_Varps;
    VDouble0 m_stat_وېشل_Unpacked_Orig;  // Original AstVars of unpacked type split
    VDouble0 m_stat_وېشل_Packed_Orig;  // Original AstVars of packed type split
    VDouble0 m_stat_وېشل_Unpacked_Comp;  // Component AstVars of unpacked type split
    VDouble0 m_stat_وېشل_Packed_Comp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    ساكن منطقي can_وېشل(ثابت ځای_پرځای_کړه* placep) {
        // Not if not splittable
        إذا (!placep->splittable) أرجع خطأ;
        // Not if already split
        إذا (placep->وېشل) أرجع خطأ;
        // Original variable or its parent was already split
        أرجع !placep->parentp || placep->parentp->وېشل;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    فراغ کتار_ته_ورزیاتول(ځای_پرځای_کړه* placep) {
        إذا (placep->queued || !placep->غواړي_وېشل || !can_وېشل(placep)) أرجع;
        placep->queued = ریښتیا;
        m_worklist.ټېل_شا(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    فراغ createComponents(ځای_پرځای_کړه* placep) {
        AstVarScope* ثابت vscp = placep->vscp;
        AstVar* ثابت varp = vscp->varp();
        std::ویکټور<AstVar*>& varps = m_وېشل_Varps(varp);
        // Create the component AstVars on first encounter
        إذا (varps.تشه()) {
            ثابت منطقي packed = isPacked(varp->dtypep());
            إذا (placep->parentp) {
                ++(packed ? m_stat_وېشل_Packed_Comp : m_stat_وېشل_Unpacked_Comp);
            } وإلا {
                ++(packed ? m_stat_وېشل_Packed_Orig : m_stat_وېشل_Unpacked_Orig);
            }
            File_کرښه* ثابت flp = varp->fileline();
            ثابت VVarType varType = varp->varType();
            ثابت std::string name = varp->name();
            لكل (ثابت Component& comp : dtypeComponents(varp->dtypep())) {
                ثابت std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_غوټه::encode_شمېره(comp.declIdx) + "__KET__";
                AstVar* ثابت newp = جديد AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagate_وېشل_Attr_From(varp);
                varps.ټېل_شا(newp);
                varp->جمع_کړه_دلته_This_As_بل(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ثابت scopep = vscp->scopep();
        File_کرښه* ثابت flp = vscp->fileline();
        لكل (اندازه_t i = 0; i < varps.اندازه(); ++i) {
            AstVarScope* ثابت newp = جديد AstVarScope{flp, scopep, varps[i]};
            component_د(placep, i)->vscp = newp;
            vscp->جمع_کړه_دلته_This_As_بل(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    فراغ جمع_کړه_Copy(ځای_پرځای_کړه* placep, ثابت Copy& copy) {
        UASSERT(!placep->وېشل, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        إذا (!placep->splittable) أرجع;
        // Add the copy to the Place
        placep->copies.ټېل_شا(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        إذا (!isPacked(placep->dtypep)) أرجع;
        إذا (copy.lsb != 0 || copy.پلنوالی != placep->dtypep->پلنوالی()) {
            placep->غواړي_وېشل = ریښتیا;
            کتار_ته_ورزیاتول(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    فراغ newCopy(ځای_پرځای_کړه* ap, صحيح aLsb, ځای_پرځای_کړه* bp, صحيح bLsb, صحيح پلنوالی) {
        UASSERT(ap != bp, "Copy within a Place");
        جمع_کړه_Copy(ap, {bp, aLsb, bLsb, پلنوالی});
        جمع_کړه_Copy(bp, {ap, bLsb, aLsb, پلنوالی});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    فراغ وېشل_Copy(ځای_پرځای_کړه* placep, ثابت Copy& copy) {
        ځای_پرځای_کړه* ثابت otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        إذا (isPacked(placep->dtypep)) {
            ثابت std::ویکټور<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ثابت صحيح lsb = copy.lsb;
            ثابت صحيح msb = copy.lsb + copy.پلنوالی - 1;
            ثابت اندازه_t لومړی = component_شاخص(placep->dtypep, lsb);
            لكل (اندازه_t i = لومړی; i < comps.اندازه() && comps[i].lsb <= msb; ++i) {
                ثابت Component& comp = comps[i];
                ثابت صحيح partLsb = std::max(lsb, comp.lsb);
                ثابت صحيح partMsb = std::min(msb, comp.msb);
                newCopy(component_د(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            أرجع;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->غواړي_وېشل = ریښتیا;
        کتار_ته_ورزیاتول(otherp);
        // The sides are the same shape, so pairwise whole copies
        ثابت اندازه_t اندازه = dtypeComponents(placep->dtypep).اندازه();
        لكل (اندازه_t i = 0; i < اندازه; ++i) {
            ځای_پرځای_کړه* ثابت ap = component_د(placep, i);
            إذا (!ap->splittable) استمر;
            ځای_پرځای_کړه* ثابت bp = component_د(otherp, i);
            إذا (!bp->splittable) استمر;
            newCopy(ap, 0, bp, 0, ap->dtypep->پلنوالی());
        }
    }

    // Split the Place
    فراغ وېشل_ځای_پرځای_کړه(ځای_پرځای_کړه* placep) {
        UASSERT(!placep->وېشل, "Place split twice");
        // It is now split
        placep->وېشل = ریښتیا;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ثابت اندازه_t اندازه = dtypeComponents(placep->dtypep).اندازه();
        لكل (اندازه_t i = 0; i < اندازه; ++i) کتار_ته_ورزیاتول(component_د(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        لكل (ثابت Copy& copy : placep->copies) {
            إذا (!copy.otherp->وېشل) وېشل_Copy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    څرګند DecomposeDecision(State& state)
        : Decompose_بنسټ{state} {
        // Enqueue the original variables
        لكل (ځای_پرځای_کړه* ثابت placep : m_state.rootps) کتار_ته_ورزیاتول(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        طالما (!m_worklist.تشه()) {
            ځای_پرځای_کړه* ثابت placep = m_worklist.شا();
            m_worklist.راوباسه_شا();
            placep->queued = خطأ;
            وېشل_ځای_پرځای_کړه(placep);
        }

        V3Stats::جمع_کړه_Stat("Optimizations, Decompose, unpacked variables split",
                         m_stat_وېشل_Unpacked_Orig);
        V3Stats::جمع_کړه_Stat("Optimizations, Decompose, packed variables split",
                         m_stat_وېشل_Packed_Orig);
        V3Stats::جمع_کړه_Stat("Optimizations, Decompose, unpacked components split further",
                         m_stat_وېشل_Unpacked_Comp);
        V3Stats::جمع_کړه_Stat("Optimizations, Decompose, packed components split further",
                         m_stat_وېشل_Packed_Comp);
    }

عام:
    ساكن فراغ apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

صنف DecomposeRewrite final : عام VNDeleter, عام Decompose_بنسټ {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    ساكن AstVarRef* newRef(File_کرښه* flp, ثابت ځای_پرځای_کړه* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        أرجع جديد AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    فراغ rewriteChain(ثابت std::ویکټور<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ثابت Select* selectp = تش_اشاره;
        لكل (ثابت Select& select : chain) {
            إذا (select.placep->parentp->وېشل) selectp = &select;
        }
        إذا (!selectp) أرجع;
        Ast_غوټه_بیان* ثابت exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ثابت VAccess access = [&]() {
            ثابت Ast_غوټه_بیان* rootp = chain.front().exprp;
            طالما (ریښتیا) {
                إذا (ثابت AstArraySel* ثابت selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } وإلا إذا (ثابت AstStructSel* ثابت selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } وإلا إذا (ثابت AstSel* ثابت selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } وإلا {
                    أرجع VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_کرښه* ثابت flp = exprp->fileline();
        Ast_غوټه_بیان* newp = newRef(flp, selectp->placep, access);
        إذا (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            ثابت صحيح پلنوالی = selp->پلنوالی_Const();
            إذا (پلنوالی != newp->پلنوالی()) newp = جديد AstSel{flp, newp, selectp->lsb, پلنوالی};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_غوټه_بیان* newSelect(Ast_غوټه_بیان* fromp, Ast_غوټه_DType* dtypep, اندازه_t idx) {
        File_کرښه* ثابت flp = fromp->fileline();
        ثابت Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        إذا (isPacked(dtypep)) {
            AstSel* ثابت selp = جديد AstSel{flp, fromp, comp.lsb, comp.dtypep->پلنوالی()};
            selp->dtypep(comp.dtypep);
            أرجع selp;
        }
        // If unpacked array, it's an ArraySel
        إذا (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ثابت صحيح i = static_cast<صحيح>(idx);
            UASSERT_OBJ(static_cast<اندازه_t>(i) == idx, fromp, "Index overflow");
            أرجع جديد AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ثابت selp = جديد AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        أرجع selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    ساكن std::ویکټور<Ast_غوټه_بیان*> concatTerms(Ast_غوټه_بیان* nodep) {
        std::ویکټور<Ast_غوټه_بیان*> termps;
        std::ویکټور<Ast_غوټه_بیان*> ډېری{nodep};
        طالما (!ډېری.تشه()) {
            Ast_غوټه_بیان* ثابت exprp = ډېری.شا();
            ډېری.راوباسه_شا();
            إذا (AstConcat* ثابت concatp = VN_CAST(exprp, Concat)) {
                ډېری.ټېل_شا(concatp->lhsp());
                ډېری.ټېل_شا(concatp->rhsp());
                استمر;
            }
            إذا (AstReplicate* ثابت repp = VN_CAST(exprp, Replicate)) {
                ثابت AstConst* ثابت countp = VN_AS(repp->countp(), Const);
                لكل (uint32_t i = 0; i < countp->منزل_UInt(); ++i) ډېری.ټېل_شا(repp->srcp());
                استمر;
            }
            termps.ټېل_شا(exprp);
        }
        أرجع termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    ساكن منطقي isSliceable(ثابت Ast_غوټه_بیان* nodep) {
        إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            أرجع VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        إذا (ثابت AstConcat* ثابت catp = VN_CAST(nodep, Concat)) {
            أرجع isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        إذا (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            أرجع isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        إذا (ثابت AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            أرجع isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        إذا (ثابت AstOr* ثابت orp = VN_CAST(nodep, Or)) {
            أرجع isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        إذا (ثابت AstXor* ثابت xorp = VN_CAST(nodep, Xor)) {
            أرجع isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        إذا (ثابت AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {
            أرجع isSliceable(extp->lhsp());
        }
        إذا (ثابت AstNot* ثابت notp = VN_CAST(nodep, Not)) {  //
            أرجع isSliceable(notp->lhsp());
        }
        إذا (ثابت AstReplicate* ثابت repp = VN_CAST(nodep, Replicate)) {
            أرجع isSliceable(repp->srcp());
        }
        أرجع isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    ساكن Ast_غوټه_بیان* new_ټوټه(Ast_غوټه_بیان* nodep, صحيح lsb, صحيح پلنوالی) {
        File_کرښه* ثابت flp = nodep->fileline();
        // A constant, as a new constant of the bits
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            V3_شمېره عدد{nodep, پلنوالی};
            عدد.opSel(constp->عدد(), lsb + پلنوالی - 1, lsb);
            أرجع جديد AstConst{flp, عدد};
        }
        // A constant bit select, from the bits of what it selects from
        إذا (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            إذا (ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                أرجع new_ټوټه(selp->fromp(), lsbp->منزل_SInt() + lsb, پلنوالی);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        إذا (AstConcat* ثابت catp = VN_CAST(nodep, Concat)) {
            ثابت صحيح r_پلنوالی = catp->rhsp()->پلنوالی();
            ثابت صحيح msb = lsb + پلنوالی - 1;
            إذا (msb < r_پلنوالی) أرجع new_ټوټه(catp->rhsp(), lsb, پلنوالی);
            إذا (lsb >= r_پلنوالی) أرجع new_ټوټه(catp->lhsp(), lsb - r_پلنوالی, پلنوالی);
            أرجع جديد AstConcat{flp, new_ټوټه(catp->lhsp(), 0, msb - r_پلنوالی + 1),
                                 new_ټوټه(catp->rhsp(), lsb, r_پلنوالی - lsb)};
        }
        إذا (AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            أرجع جديد AstCond{flp, condp->condp()->clone_ونه_خالص(خطأ),
                               new_ټوټه(condp->thenp(), lsb, پلنوالی),
                               new_ټوټه(condp->elsep(), lsb, پلنوالی)};
        }
        إذا (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            أرجع جديد AstAnd{flp, new_ټوټه(andp->lhsp(), lsb, پلنوالی),
                              new_ټوټه(andp->rhsp(), lsb, پلنوالی)};
        }
        إذا (AstOr* ثابت orp = VN_CAST(nodep, Or)) {
            أرجع جديد AstOr{flp, new_ټوټه(orp->lhsp(), lsb, پلنوالی),
                             new_ټوټه(orp->rhsp(), lsb, پلنوالی)};
        }
        إذا (AstXor* ثابت xorp = VN_CAST(nodep, Xor)) {
            أرجع جديد AstXor{flp, new_ټوټه(xorp->lhsp(), lsb, پلنوالی),
                              new_ټوټه(xorp->rhsp(), lsb, پلنوالی)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        إذا (AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {
            ثابت صحيح src_پلنوالی = extp->lhsp()->پلنوالی();
            ثابت صحيح msb = lsb + پلنوالی - 1;
            إذا (msb < src_پلنوالی) أرجع new_ټوټه(extp->lhsp(), lsb, پلنوالی);
            إذا (lsb >= src_پلنوالی) أرجع جديد AstConst{flp, AstConst::Widthed_ارزښت{}, پلنوالی, 0};
            أرجع جديد AstConcat{
                flp, جديد AstConst{flp, AstConst::Widthed_ارزښت{}, msb - src_پلنوالی + 1, 0},
                new_ټوټه(extp->lhsp(), lsb, src_پلنوالی - lsb)};
        }
        إذا (AstNot* ثابت notp = VN_CAST(nodep, Not)) {
            أرجع جديد AstNot{flp, new_ټوټه(notp->lhsp(), lsb, پلنوالی)};
        }
        // A replication, from the portions of the copies overlapping the bits
        إذا (AstReplicate* ثابت repp = VN_CAST(nodep, Replicate)) {
            Ast_غوټه_بیان* ثابت srcp = repp->srcp();
            ثابت صحيح src_پلنوالی = srcp->پلنوالی();
            ثابت صحيح msb = lsb + پلنوالی - 1;
            Ast_غوټه_بیان* resultp = تش_اشاره;
            لكل (صحيح copyLsb = lsb / src_پلنوالی * src_پلنوالی; copyLsb <= msb; copyLsb += src_پلنوالی) {
                ثابت صحيح partLsb = std::max(lsb, copyLsb) - copyLsb;
                ثابت صحيح partMsb = std::min(msb, copyLsb + src_پلنوالی - 1) - copyLsb;
                Ast_غوټه_بیان* ثابت bitsp = new_ټوټه(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? جديد AstConcat{flp, bitsp, resultp} : bitsp;
            }
            أرجع resultp;
        }
        Ast_غوټه_بیان* ثابت clonep = nodep->clone_ونه_خالص(خطأ);
        إذا (lsb == 0 && پلنوالی == nodep->پلنوالی()) أرجع clonep;
        أرجع جديد AstSel{flp, clonep, lsb, پلنوالی};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::ویکټور<Ast_غوټه_بیان*> new_ټاکل_Rhsps(Ast_غوټه_بیان* nodep, Ast_غوټه_DType* dtypep) {
        ثابت std::ویکټور<Component>& compsr = dtypeComponents(dtypep);
        std::ویکټور<Ast_غوټه_بیان*> valueps;
        valueps.reserve(compsr.اندازه());

        // If CReset, it is duplicated for each component
        إذا (AstCReset* ثابت cresetp = VN_CAST(nodep, CReset)) {
            لكل (ثابت Component& comp : compsr) {
                AstCReset* ثابت newp = cresetp->clone_ونه(خطأ);
                newp->dtypep(comp.dtypep);
                valueps.ټېل_شا(newp);
            }
            أرجع valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        إذا (isPacked(dtypep)) {
            valueps.resize(compsr.اندازه(), تش_اشاره);
            صحيح lsb = 0;  // LSB of the current term
            لكل (Ast_غوټه_بیان* ثابت termp : concatTerms(nodep)) {
                ثابت صحيح msb = lsb + termp->پلنوالی() - 1;
                ثابت اندازه_t لومړی = component_شاخص(dtypep, lsb);
                لكل (اندازه_t i = لومړی; i < compsr.اندازه() && compsr[i].lsb <= msb; ++i) {
                    ثابت Component& comp = compsr[i];
                    ثابت صحيح partLsb = std::max(lsb, comp.lsb);
                    ثابت صحيح partMsb = std::min(msb, comp.msb);
                    File_کرښه* ثابت flp = termp->fileline();
                    Ast_غوټه_بیان* ثابت sp = new_ټوټه(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? جديد AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            أرجع valueps;
        }

        // Otherwise unpacked, so select each component
        لكل (اندازه_t i = 0; i < compsr.اندازه(); ++i) {
            valueps.ټېل_شا(newSelect(nodep->clone_ونه(خطأ), dtypep, i));
        }
        أرجع valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_غوټه_بیان* new_بټونه(File_کرښه* flp, ثابت ځای_پرځای_کړه* placep, صحيح lsb, صحيح msb) {
        Ast_غوټه_DType* ثابت dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->وېشل, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        إذا (!placep->وېشل) {
            AstVarRef* ثابت refp = newRef(flp, placep, VAccess::READ);
            إذا (lsb == 0 && msb == dtypep->پلنوالی() - 1) أرجع refp;
            أرجع جديد AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ثابت std::ویکټور<Component>& comps = dtypeComponents(dtypep);
        Ast_غوټه_بیان* resultp = تش_اشاره;
        ثابت اندازه_t لومړی = component_شاخص(dtypep, lsb);
        لكل (اندازه_t i = لومړی; i < comps.اندازه() && comps[i].lsb <= msb; ++i) {
            ثابت Component& comp = comps[i];
            ثابت صحيح partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ثابت صحيح partMsb = std::min(msb, comp.msb) - comp.lsb;
            ځای_پرځای_کړه* ثابت childp = placep->childrenp.at(i).get();
            Ast_غوټه_بیان* ثابت bitsp = new_بټونه(flp, childp, partLsb, partMsb);
            resultp = resultp ? جديد AstConcat{flp, bitsp, resultp} : bitsp;
        }
        أرجع resultp;
    }

    ساكن فراغ جمع_کړه_New_ټاکل(Ast_غوټه_ټاکل* origp, Ast_غوټه_بیان* lhsp, Ast_غوټه_بیان* rhsp) {
        origp->جمع_کړه_دلته_This_As_بل(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    فراغ expandSS(Ast_غوټه_ټاکل* origp, ځای_پرځای_کړه* lPlacep, ځای_پرځای_کړه* rPlacep) {
        File_کرښه* ثابت flp = origp->fileline();
        ثابت std::ویکټور<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        إذا (isPacked(rPlacep->dtypep)) {
            لكل (اندازه_t i = 0; i < compsr.اندازه(); ++i) {
                ځای_پرځای_کړه* ثابت lChildp = lPlacep->childrenp.at(i).get();
                Ast_غوټه_بیان* ثابت rp = new_بټونه(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, تش_اشاره, تش_اشاره, rp);
            }
        } وإلا {
            لكل (اندازه_t i = 0; i < compsr.اندازه(); ++i) {
                ځای_پرځای_کړه* ثابت lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, تش_اشاره, rPlacep->childrenp.at(i).get(), تش_اشاره);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    فراغ expandSN(Ast_غوټه_ټاکل* origp, ځای_پرځای_کړه* lPlacep, Ast_غوټه_بیان* rhsp) {
        ثابت std::ویکټور<Ast_غوټه_بیان*> valueps = new_ټاکل_Rhsps(rhsp, lPlacep->dtypep);
        لكل (اندازه_t i = 0; i < valueps.اندازه(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), تش_اشاره, تش_اشاره, valueps[i]);
        }
        VL_DO_DANGLING(ټېل_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    فراغ expandNS(Ast_غوټه_ټاکل* origp, Ast_غوټه_بیان* lhsp, ځای_پرځای_کړه* rPlacep) {
        File_کرښه* ثابت flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        إذا (isPacked(rPlacep->dtypep)) {
            جمع_کړه_New_ټاکل(origp, lhsp, new_بټونه(flp, rPlacep, 0, rPlacep->dtypep->پلنوالی() - 1));
            أرجع;
        }
        // Unpacked of the same shape, component-wise
        Ast_غوټه_DType* ثابت dtypep = dtype_د(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ثابت منطقي readsWrittenVar = !VN_IS(origp, ټاکل_Dly) && [&]() {
            ثابت VNUser2_دننه_Use user2_دننه_Use;
            أرجع lhsp->exists([](ثابت AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                إذا (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                إذا (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                أرجع vscp->user2() == 3;
            });
        }();
        إذا (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ثابت scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, تش_اشاره, جديد AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, تش_اشاره);
            جمع_کړه_New_ټاکل(origp, lhsp, جديد AstVarRef{flp, vscp, VAccess::READ});
            أرجع;
        }
        ثابت اندازه_t اندازه = dtypeComponents(dtypep).اندازه();
        لكل (اندازه_t i = 0; i < اندازه; ++i) {
            Ast_غوټه_بیان* ثابت newLhsp = newSelect(lhsp->clone_ونه(خطأ), dtypep, i);
            expand(origp, تش_اشاره, newLhsp, rPlacep->childrenp.at(i).get(), تش_اشاره);
        }
        VL_DO_DANGLING(ټېل_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    فراغ expand(Ast_غوټه_ټاکل* origp,  //
                ځای_پرځای_کړه* lPlacep, Ast_غوټه_بیان* lhsp,  //
                ځای_پرځای_کړه* rPlacep, Ast_غوټه_بیان* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_کرښه* ثابت flp = origp->fileline();
        // A Place not split is referenced
        إذا (lPlacep && !lPlacep->وېشل) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = تش_اشاره;
        }
        إذا (rPlacep && !rPlacep->وېشل) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = تش_اشاره;
        }
        إذا (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } وإلا إذا (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } وإلا إذا (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } وإلا {
            جمع_کړه_New_ټاکل(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    منطقي is_وېشل_Between(ثابت ځای_پرځای_کړه* placep, صحيح lsb, صحيح msb) {
        طالما (placep->وېشل) {
            ثابت اندازه_t idx = component_شاخص(placep->dtypep, lsb);
            إذا (idx != component_شاخص(placep->dtypep, msb)) أرجع ریښتیا;
            ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        أرجع خطأ;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    فراغ hoistTerms(Ast_غوټه_ټاکل* origp, ثابت ځای_پرځای_کړه* lPlacep) {
        Ast_غوټه_بیان* ثابت rhsp = origp->rhsp();
        ثابت std::ویکټور<Ast_غوټه_بیان*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ثابت VNUser2_دننه_Use user2_دننه_Use;
        لكل (Ast_غوټه_بیان* ثابت termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
        صحيح lsb = 0;  // LSB of the current term
        لكل (Ast_غوټه_بیان* ثابت termp : termps) {
            ثابت صحيح tLsb = lsb;
            ثابت صحيح tMsb = lsb + termp->پلنوالی() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            إذا (!termp->user2()) استمر;
            // Cheap to select from
            إذا (isCheap(termp)) {
                termp->user2(0);
                استمر;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            إذا (termp->is_خالص() && termp->user2() == 1 && !is_وېشل_Between(lPlacep, tLsb, tMsb)) {
                استمر;
            }
            // Bits can be selected without evaluating it more than once
            إذا (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                استمر;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_کرښه* ثابت flp = termp->fileline();
            AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(جديد AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ثابت tmpRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
            origp->جمع_کړه_دلته_This_As_بل(جديد Ast_ټاکل{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    ساكن فراغ markComponents(ثابت ځای_پرځای_کړه* placep) {
        لكل (ثابت std::unique_ptr<ځای_پرځای_کړه>& childp : placep->childrenp) {
            childp->vscp->user2(ریښتیا);
            إذا (childp->وېشل) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    فراغ rewriteAssignment(ثابت Assignment& assignment) {
        ځای_پرځای_کړه* ثابت lPlacep = assignment.lPlacep;
        ځای_پرځای_کړه* ثابت rPlacep = assignment.rPlacep;
        ثابت منطقي l_وېشل = lPlacep && lPlacep->وېشل;
        ثابت منطقي r_وېشل = rPlacep && rPlacep->وېشل;
        // Nothing to do if neither side is split
        إذا (!l_وېشل && !r_وېشل) أرجع;

        Ast_غوټه_ټاکل* ثابت origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        إذا (l_وېشل && !r_وېشل && !VN_IS(origp, ټاکل_Dly)) {
            ثابت VNUser2_دننه_Use user2_دننه_Use;
            markComponents(lPlacep);
            Ast_غوټه_بیان* ثابت rhsp = origp->rhsp();
            إذا (rhsp->exists([](ثابت AstVarRef* refp) { أرجع refp->varScopep()->user2(); })) {
                File_کرښه* ثابت flp = rhsp->fileline();
                AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, dtype_د(rhsp));
                rhsp->replaceWith(جديد AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ثابت tmpRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
                origp->جمع_کړه_دلته_This_As_بل(جديد Ast_ټاکل{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        إذا (l_وېشل && !r_وېشل && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_غوټه_بیان* ثابت lhsp = origp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rhsp = origp->rhsp()->unlink_Fr_شا();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               l_وېشل ? lPlacep : تش_اشاره, l_وېشل ? تش_اشاره : lhsp,  //
               r_وېشل ? rPlacep : تش_اشاره, r_وېشل ? تش_اشاره : rhsp);
        إذا (l_وېشل) VL_DO_DANGLING(ټېل_Deletep(lhsp), lhsp);
        إذا (r_وېشل) VL_DO_DANGLING(ټېل_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ټېل_Deletep(origp->unlink_Fr_شا()), origp);
    }

    // The combinational AstActive of the scope, cached
    ساكن Ast_فعال* combo_فعال(AstScope* scopep) {
        إذا (!scopep->user1p()) scopep->user1p(scopep->comboActivep(ریښتیا));
        أرجع VN_AS(scopep->user1p(), فعال);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    فراغ driveFromComponents(ځای_پرځای_کړه* placep, Ast_فعال* activep) {
        إذا (!placep->وېشل) أرجع;
        AstVarScope* ثابت vscp = placep->vscp;
        File_کرښه* ثابت flp = vscp->fileline();
        لكل (اندازه_t i = 0; i < placep->childrenp.اندازه(); ++i) {
            ځای_پرځای_کړه* ثابت childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_غوټه_بیان* ثابت rhsp = جديد AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_غوټه_بیان* ثابت lhsRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_غوټه_بیان* ثابت lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->جمع_کړه_Stmtsp(جديد AstAlways{جديد Ast_ټاکل_W{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    څرګند DecomposeRewrite(State& state)
        : Decompose_بنسټ{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        لكل (ثابت std::ویکټور<Select>& chain : m_state.chains) rewriteChain(chain);
        لكل (ثابت Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        لكل (ځای_پرځای_کړه* ثابت placep : m_state.rootps) {
            إذا (!placep->وېشل) استمر;
            ثابت AstVarScope* ثابت vscp = placep->vscp;
            // Only traced ones at this point
            إذا (!(vscp->varp()->isTrace() && vscp->isTrace())) استمر;
            Ast_فعال* ثابت activep = combo_فعال(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::جمع_کړه_Stat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::جمع_کړه_Stat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::جمع_کړه_Stat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::جمع_کړه_Stat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

عام:
    ساكن فراغ apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

فراغ V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        Decompose_بنسټ::State state;
        Decompose_ریکارډ::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("decompose", 0, dump_ونه_Either_Level() >= 3);
}
