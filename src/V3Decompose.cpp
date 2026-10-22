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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Decompose.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

ವರ್ಗ DecomposeBase VL_NOT_FINAL {
ರಕ್ಷಿತ:
    // TYPES

    // A component (element or member) of an aggregate type
    ಸಂರಚನಾ Component final {
        Ast_ಗಂಟು_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        ಪೂರ್ಣಾಙ್ಕ declIdx;  // The declared index of the element, if an array
        ಪೂರ್ಣಾಙ್ಕ lsb;  // LSB in the whole value, if packed
        ಪೂರ್ಣಾಙ್ಕ msb;  // MSB in the whole value, if packed
    };

    ಸಂರಚನಾ Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    ಸಂರಚನಾ Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        ಪೂರ್ಣಾಙ್ಕ lsb;  // The first bit covered of this end
        ಪೂರ್ಣಾಙ್ಕ otherLsb;  // The first bit covered of the other end
        ಪೂರ್ಣಾಙ್ಕ width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    ಸಂರಚನಾ Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_ಗಂಟು_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        ತಾರ್ಕಿಕ splittable = ಸತ್ಯ;  // Can be split (there is no reason not to)
        ತಾರ್ಕಿಕ wantsSplit = ಅಸತ್ಯ;  // Should be split (if possible, do split)
        ತಾರ್ಕಿಕ split = ಅಸತ್ಯ;  // Place was split
        ತಾರ್ಕಿಕ queued = ಅಸತ್ಯ;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    ಸಂರಚನಾ Select final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;  // The select
        Place* placep;  // The component
        ಪೂರ್ಣಾಙ್ಕ lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    ಸಂರಚನಾ Assignment final {
        Ast_ಗಂಟು_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

ಸಾರ್ವಜನಿಕ:
    // The state of the pass, shared by the steps
    ವರ್ಗ State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ಧ್ರುವ VNUser4InUse m_user4InUse;

    ಸಾರ್ವಜನಿಕ:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_ಗಂಟು_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

ರಕ್ಷಿತ:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isUnpacked(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಯದಿ (VN_IS(dtypep, UnpackArrayDType)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಧ್ರುವ AstStructDType* ಧ್ರುವ structp = VN_CAST(dtypep, StructDType);
        ಪ್ರತ್ಯಾಗಮ structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isPacked(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಯದಿ (ಧ್ರುವ AstPackArrayDType* ಧ್ರುವ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ಪ್ರತ್ಯಾಗಮ arrayp->subDTypep()->width() > 1;
        }
        ಧ್ರುವ AstStructDType* ಧ್ರುವ structp = VN_CAST(dtypep, StructDType);
        ಪ್ರತ್ಯಾಗಮ structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSplittableType(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        ಯದಿ (isPacked(dtypep)) ಪ್ರತ್ಯಾಗಮ v3Global.opt.fDecomposePacked();
        ಯದಿ (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            ಯದಿ (ಧ್ರುವ AstStructDType* ಧ್ರುವ structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ಯದಿ (mp->valuep()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
            }
            ಪ್ರತ್ಯಾಗಮ v3Global.opt.fDecomposeUnpacked();
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* dtypeOf(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) ಪ್ರತ್ಯಾಗಮ refp->varp()->dtypep();
        ಪ್ರತ್ಯಾಗಮ nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ಧ್ರುವ std::vector<Component>& dtypeComponents(Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        ಯದಿ (compsr.empty()) {
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ arrayp = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                ಧ್ರುವ ತಾರ್ಕಿಕ packed = isPacked(dtypep);
                ಧ್ರುವ VNumRange range = arrayp->declRange();
                ಧ್ರುವ ತಾರ್ಕಿಕ rev = packed && range.ascending();  // Only for naming components
                Ast_ಗಂಟು_DType* ಧ್ರುವ subp = arrayp->subDTypep();
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < range.elements(); ++i) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = packed ? i * subp->width() : 0;
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ತಳ್ಳು_back({subp, nullptr, declIdx, lsb, msb});
                }
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ AstStructDType* ಧ್ರುವ structp = VN_AS(dtypep, StructDType);
                ಧ್ರುವ ತಾರ್ಕಿಕ packed = isPacked(dtypep);
                ಪ್ರತಿ (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = packed ? mp->lsb() : 0;
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.ತಳ್ಳು_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        ಪ್ರತ್ಯಾಗಮ compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(Ast_ಗಂಟು_DType* dtypep, ಪೂರ್ಣಾಙ್ಕ bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ಧ್ರುವ std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ಧ್ರುವ auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](ಧ್ರುವ Component& comp, ಪೂರ್ಣಾಙ್ಕ b) {  //
                                             ಪ್ರತ್ಯಾಗಮ b > comp.msb;
                                         });
        ಪ್ರತ್ಯಾಗಮ it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        ಧ್ರುವ std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        ಯದಿ (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        ಯದಿ (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        ಪ್ರತ್ಯಾಗಮ childpr.get();
    }

    // Is 'nodep' cheap to clone
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isCheap(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Constants are cheap
        ಯದಿ (VN_IS(nodep, Const)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // So are resets
        ಯದಿ (VN_IS(nodep, CReset)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Variable references are cheap for the most part
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            ಯದಿ (refp->varp()->isForced()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            ಯದಿ (refp->varp()->isSc()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Otherwise the path rooted here is cheap
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        // Bit selects only with a constant LSB
        ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(nodep, Sel)) {
            ಪ್ರತ್ಯಾಗಮ VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        ಯದಿ (ಧ್ರುವ AstArraySel* ಧ್ರುವ selp = VN_CAST(nodep, ArraySel)) {
            ಪ್ರತ್ಯಾಗಮ isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        ಯದಿ (ಧ್ರುವ AstStructSel* ಧ್ರುವ selp = VN_CAST(nodep, StructSel)) {  //
            ಪ್ರತ್ಯಾಗಮ isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        ಯದಿ (ಧ್ರುವ AstExtend* ಧ್ರುವ extp = VN_CAST(nodep, Extend)) {  //
            ಪ್ರತ್ಯಾಗಮ isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

ವರ್ಗ DecomposeRecord final : ಸಾರ್ವಜನಿಕ VNVisitorConst, ಸಾರ್ವಜನಿಕ DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ಧ್ರುವ VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_ಗಂಟು_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ಸ್ಥಿರ ಶೂನ್ಯ warnNoSplit(AstVar* varp, ಧ್ರುವ Ast_ಗಂಟು* wherep, ಧ್ರುವ ವರ್ಣ* reasonp) {
        // Only warn if user requested splitting
        ಯದಿ (!varp->attrSplitVar()) ಪ್ರತ್ಯಾಗಮ;
        // A packed variable might still be split by V3BitBlast, which warns if not
        ಯದಿ (isPacked(varp->dtypep())) ಪ್ರತ್ಯಾಗಮ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, ಸತ್ಯ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ಯದಿ (!varp->user1()) {
            ಧ್ರುವ ತಾರ್ಕಿಕ eligible = [&]() {
                // Check data type
                ಯದಿ (!isSplittableType(varp->dtypep())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Check properties
                ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                ಯದಿ (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ಪ್ರತ್ಯಾಗಮ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ಪ್ರತ್ಯಾಗಮ varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(ಧ್ರುವ AstVarRef* refp) {
        AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
        ಯದಿ (!isEligible(vscp->varp())) ಪ್ರತ್ಯಾಗಮ nullptr;
        Place& place = m_state.places(vscp);
        ಯದಿ (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.ತಳ್ಳು_back(&place);
        }
        ಪ್ರತ್ಯಾಗಮ &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    ಸ್ಥಿರ ಶೂನ್ಯ block(Place* placep, ಧ್ರುವ Ast_ಗಂಟು* wherep) {
        placep->splittable = ಅಸತ್ಯ;
        // Warn on the variable only, a component is split as far as possible, as requested
        ಯದಿ (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        ಯದಿ (AstArraySel* ಧ್ರುವ selp = VN_CAST(exprp, ArraySel)) {
            ಯದಿ (Place* ಧ್ರುವ fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ಧ್ರುವ AstConst* ಧ್ರುವ bitp = VN_CAST(selp->bitp(), Const);
                ಯದಿ (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* ಧ್ರುವ childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = ಸತ್ಯ;
                    chain.ತಳ್ಳು_back({selp, childp, 0});
                    ಪ್ರತ್ಯಾಗಮ childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        // Struct selects correspond one to one with a component select
        ಯದಿ (AstStructSel* ಧ್ರುವ selp = VN_CAST(exprp, StructSel)) {
            ಯದಿ (Place* ಧ್ರುವ fromp = resolveChain(selp->fromp(), chain)) {
                ಯದಿ (!isUnpacked(fromp->dtypep)) ಪ್ರತ್ಯಾಗಮ nullptr;
                AstStructDType* ಧ್ರುವ structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                ಯದಿ (!structp->user1SetOnce()) {
                    ಧ್ರುವ std::vector<Component>& compsr = dtypeComponents(structp);
                    ಪ್ರತಿ (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* ಧ್ರುವ childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = ಸತ್ಯ;
                chain.ತಳ್ಳು_back({selp, childp, 0});
                ಪ್ರತ್ಯಾಗಮ childp;
            }
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            ಯದಿ (!placep) {
                iterateConst(selp->lsbp());
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ಧ್ರುವ AstConst* ಧ್ರುವ lsbp = VN_CAST(selp->lsbp(), Const);
            ಯದಿ (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
            // Descend into the Place containing the bits, until exact
            ಪೂರ್ಣಾಙ್ಕ lsb = lsbp->toSInt();
            ಪೂರ್ಣಾಙ್ಕ msb = lsb + selp->widthConst() - 1;
            ಯಾವತ್ (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                ಯದಿ (!isPacked(placep->dtypep)) ಪ್ರತ್ಯಾಗಮ nullptr;
                ಧ್ರುವ size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                ಯದಿ (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    ಪ್ರತ್ಯಾಗಮ nullptr;
                }
                placep->wantsSplit = ಸತ್ಯ;
                ಧ್ರುವ Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.ತಳ್ಳು_back({selp, placep, lsb});
            }
            ಪ್ರತ್ಯಾಗಮ placep;
        }
        // Base case
        ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(exprp, VarRef)) ಪ್ರತ್ಯಾಗಮ placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    ಶೂನ್ಯ referencedWhole(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        ಯದಿ (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    ಶೂನ್ಯ visitSelect(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* ಧ್ರುವ placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        ಯದಿ (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        ಯದಿ (!chain.empty()) m_state.chains.ತಳ್ಳು_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasReversedRange(ಧ್ರುವ Ast_ಗಂಟು_DType* aDTypep, ಧ್ರುವ Ast_ಗಂಟು_DType* bDTypep) {
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        ಯದಿ (!aArrayp || !bArrayp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    ಶೂನ್ಯ visitAssignment(Ast_ಗಂಟು_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ಧ್ರುವ ತಾರ್ಕಿಕ reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        ಯದಿ (!m_assignp) ಪ್ರತ್ಯಾಗಮ;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp();
        Place* ಧ್ರುವ lPlacep = lhsp->user1u().to<Place*>();
        Place* ಧ್ರುವ rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        ಧ್ರುವ ತಾರ್ಕಿಕ splittable = [&]() {
            // Assignment between two Places is splittable
            ಯದಿ (lPlacep && rPlacep) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            // If the LHS is a Place, the RHS must be a splittable expression
            ಯದಿ (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                ಯದಿ (isPacked(lPlacep->dtypep)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                ಪ್ರತ್ಯಾಗಮ isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            ಯದಿ (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                ಯದಿ (isPacked(rPlacep->dtypep)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                ಪ್ರತ್ಯಾಗಮ isCheap(lhsp);
            }
            // Otherwise not splittable
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        ಯದಿ (!splittable) {
            ಯದಿ (rPlacep) block(rPlacep, rhsp);
            ಯದಿ (lPlacep) block(lPlacep, lhsp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // If both sides are places, record the whole copies for the Decision phase
        ಯದಿ (lPlacep && rPlacep) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.ತಳ್ಳು_back({rPlacep, 0, 0, width});
            rPlacep->copies.ತಳ್ಳು_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.ತಳ್ಳು_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_DType*) override {}  // No references in data types
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    ಶೂನ್ಯ visit(AstArraySel* nodep) override { visitSelect(nodep); }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override { visitSelect(nodep); }
    ಶೂನ್ಯ visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    ಶೂನ್ಯ visit(AstAssign* nodep) override { visitAssignment(nodep); }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        ಯದಿ (Place* ಧ್ರುವ placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ಧ್ರುವ varp = nodep->varp();
        ಯದಿ (!isEligible(varp)) ಪ್ರತ್ಯಾಗಮ;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        ಪ್ರತಿ (Place* ಧ್ರುವ placep : m_state.rootps) {
            ಯದಿ (!isEligible(placep->vscp->varp())) placep->splittable = ಅಸತ್ಯ;
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

ವರ್ಗ DecomposeDecision final : ಸಾರ್ವಜನಿಕ DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ಧ್ರುವ VNUser1InUse m_user1InUse;

    // STATE
    std::vector<Place*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::vector<AstVar*>> m_splitVarps;
    VDouble0 m_statSplitUnpackedOrig;  // Original AstVars of unpacked type split
    VDouble0 m_statSplitPackedOrig;  // Original AstVars of packed type split
    VDouble0 m_statSplitUnpackedComp;  // Component AstVars of unpacked type split
    VDouble0 m_statSplitPackedComp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    ಸ್ಥಿರ ತಾರ್ಕಿಕ canSplit(ಧ್ರುವ Place* placep) {
        // Not if not splittable
        ಯದಿ (!placep->splittable) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Not if already split
        ಯದಿ (placep->split) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Original variable or its parent was already split
        ಪ್ರತ್ಯಾಗಮ !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    ಶೂನ್ಯ enqueue(Place* placep) {
        ಯದಿ (placep->queued || !placep->wantsSplit || !canSplit(placep)) ಪ್ರತ್ಯಾಗಮ;
        placep->queued = ಸತ್ಯ;
        m_worklist.ತಳ್ಳು_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    ಶೂನ್ಯ createComponents(Place* placep) {
        AstVarScope* ಧ್ರುವ vscp = placep->vscp;
        AstVar* ಧ್ರುವ varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        ಯದಿ (varps.empty()) {
            ಧ್ರುವ ತಾರ್ಕಿಕ packed = isPacked(varp->dtypep());
            ಯದಿ (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } ಅನ್ಯಥಾ {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_ಸಾಲು* ಧ್ರುವ flp = varp->fileline();
            ಧ್ರುವ VVarType varType = varp->varType();
            ಧ್ರುವ std::string name = varp->name();
            ಪ್ರತಿ (ಧ್ರುವ Component& comp : dtypeComponents(varp->dtypep())) {
                ಧ್ರುವ std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(comp.declIdx) + "__KET__";
                AstVar* ಧ್ರುವ newp = ನೂತನ AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.ತಳ್ಳು_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ಧ್ರುವ scopep = vscp->scopep();
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        ಪ್ರತಿ (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* ಧ್ರುವ newp = ನೂತನ AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    ಶೂನ್ಯ addCopy(Place* placep, ಧ್ರುವ Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        ಯದಿ (!placep->splittable) ಪ್ರತ್ಯಾಗಮ;
        // Add the copy to the Place
        placep->copies.ತಳ್ಳು_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        ಯದಿ (!isPacked(placep->dtypep)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = ಸತ್ಯ;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    ಶೂನ್ಯ newCopy(Place* ap, ಪೂರ್ಣಾಙ್ಕ aLsb, Place* bp, ಪೂರ್ಣಾಙ್ಕ bLsb, ಪೂರ್ಣಾಙ್ಕ width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    ಶೂನ್ಯ splitCopy(Place* placep, ಧ್ರುವ Copy& copy) {
        Place* ಧ್ರುವ otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        ಯದಿ (isPacked(placep->dtypep)) {
            ಧ್ರುವ std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = copy.lsb;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = copy.lsb + copy.width - 1;
            ಧ್ರುವ size_t first = componentIndex(placep->dtypep, lsb);
            ಪ್ರತಿ (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                ಧ್ರುವ Component& comp = comps[i];
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partLsb = std::max(lsb, comp.lsb);
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = ಸತ್ಯ;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        ಧ್ರುವ size_t size = dtypeComponents(placep->dtypep).size();
        ಪ್ರತಿ (size_t i = 0; i < size; ++i) {
            Place* ಧ್ರುವ ap = componentOf(placep, i);
            ಯದಿ (!ap->splittable) ಅನುವರ್ತ;
            Place* ಧ್ರುವ bp = componentOf(otherp, i);
            ಯದಿ (!bp->splittable) ಅನುವರ್ತ;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    ಶೂನ್ಯ splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = ಸತ್ಯ;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ಧ್ರುವ size_t size = dtypeComponents(placep->dtypep).size();
        ಪ್ರತಿ (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        ಪ್ರತಿ (ಧ್ರುವ Copy& copy : placep->copies) {
            ಯದಿ (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        ಪ್ರತಿ (Place* ಧ್ರುವ placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        ಯಾವತ್ (!m_worklist.empty()) {
            Place* ಧ್ರುವ placep = m_worklist.back();
            m_worklist.ತೆಗೆ_back();
            placep->queued = ಅಸತ್ಯ;
            splitPlace(placep);
        }

        V3Stats::addStat("Optimizations, Decompose, unpacked variables split",
                         m_statSplitUnpackedOrig);
        V3Stats::addStat("Optimizations, Decompose, packed variables split",
                         m_statSplitPackedOrig);
        V3Stats::addStat("Optimizations, Decompose, unpacked components split further",
                         m_statSplitUnpackedComp);
        V3Stats::addStat("Optimizations, Decompose, packed components split further",
                         m_statSplitPackedComp);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

ವರ್ಗ DecomposeRewrite final : ಸಾರ್ವಜನಿಕ VNDeleter, ಸಾರ್ವಜನಿಕ DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ಧ್ರುವ VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    ಸ್ಥಿರ AstVarRef* newRef(File_ಸಾಲು* flp, ಧ್ರುವ Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    ಶೂನ್ಯ rewriteChain(ಧ್ರುವ std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ಧ್ರುವ Select* selectp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ Select& select : chain) {
            ಯದಿ (select.placep->parentp->split) selectp = &select;
        }
        ಯದಿ (!selectp) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ಧ್ರುವ VAccess access = [&]() {
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rootp = chain.front().exprp;
            ಯಾವತ್ (ಸತ್ಯ) {
                ಯದಿ (ಧ್ರುವ AstArraySel* ಧ್ರುವ selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstStructSel* ಧ್ರುವ selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } ಅನ್ಯಥಾ {
                    ಪ್ರತ್ಯಾಗಮ VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = newRef(flp, selectp->placep, access);
        ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(exprp, Sel)) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = selp->widthConst();
            ಯದಿ (width != newp->width()) newp = ನೂತನ AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSelect(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtypep, size_t idx) {
        File_ಸಾಲು* ಧ್ರುವ flp = fromp->fileline();
        ಧ್ರುವ Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        ಯದಿ (isPacked(dtypep)) {
            AstSel* ಧ್ರುವ selp = ನೂತನ AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            ಪ್ರತ್ಯಾಗಮ selp;
        }
        // If unpacked array, it's an ArraySel
        ಯದಿ (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ i = static_cast<ಪೂರ್ಣಾಙ್ಕ>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ಧ್ರುವ selp = ನೂತನ AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        ಪ್ರತ್ಯಾಗಮ selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    ಸ್ಥಿರ std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> concatTerms(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps;
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ರಾಶಿ{nodep};
        ಯಾವತ್ (!ರಾಶಿ.empty()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = ರಾಶಿ.back();
            ರಾಶಿ.ತೆಗೆ_back();
            ಯದಿ (AstConcat* ಧ್ರುವ concatp = VN_CAST(exprp, Concat)) {
                ರಾಶಿ.ತಳ್ಳು_back(concatp->lhsp());
                ರಾಶಿ.ತಳ್ಳು_back(concatp->rhsp());
                ಅನುವರ್ತ;
            }
            ಯದಿ (AstReplicate* ಧ್ರುವ repp = VN_CAST(exprp, Replicate)) {
                ಧ್ರುವ AstConst* ಧ್ರುವ countp = VN_AS(repp->countp(), Const);
                ಪ್ರತಿ (uint32_t i = 0; i < countp->toUInt(); ++i) ರಾಶಿ.ತಳ್ಳು_back(repp->srcp());
                ಅನುವರ್ತ;
            }
            termps.ತಳ್ಳು_back(exprp);
        }
        ಪ್ರತ್ಯಾಗಮ termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSliceable(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(nodep, Sel)) {
            ಪ್ರತ್ಯಾಗಮ VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        ಯದಿ (ಧ್ರುವ AstConcat* ಧ್ರುವ catp = VN_CAST(nodep, Concat)) {
            ಪ್ರತ್ಯಾಗಮ isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        ಯದಿ (ಧ್ರುವ AstCond* ಧ್ರುವ condp = VN_CAST(nodep, Cond)) {
            ಪ್ರತ್ಯಾಗಮ isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ andp = VN_CAST(nodep, And)) {
            ಪ್ರತ್ಯಾಗಮ isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        ಯದಿ (ಧ್ರುವ AstOr* ಧ್ರುವ orp = VN_CAST(nodep, Or)) {
            ಪ್ರತ್ಯಾಗಮ isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        ಯದಿ (ಧ್ರುವ AstXor* ಧ್ರುವ xorp = VN_CAST(nodep, Xor)) {
            ಪ್ರತ್ಯಾಗಮ isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        ಯದಿ (ಧ್ರುವ AstExtend* ಧ್ರುವ extp = VN_CAST(nodep, Extend)) {
            ಪ್ರತ್ಯಾಗಮ isSliceable(extp->lhsp());
        }
        ಯದಿ (ಧ್ರುವ AstNot* ಧ್ರುವ notp = VN_CAST(nodep, Not)) {  //
            ಪ್ರತ್ಯಾಗಮ isSliceable(notp->lhsp());
        }
        ಯದಿ (ಧ್ರುವ AstReplicate* ಧ್ರುವ repp = VN_CAST(nodep, Replicate)) {
            ಪ್ರತ್ಯಾಗಮ isSliceable(repp->srcp());
        }
        ಪ್ರತ್ಯಾಗಮ isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSlice(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ width) {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        // A constant, as a new constant of the bits
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) {
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, width};
            ಅಂಕೆ.opSel(constp->ಅಂಕೆ(), lsb + width - 1, lsb);
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, ಅಂಕೆ};
        }
        // A constant bit select, from the bits of what it selects from
        ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(nodep, Sel)) {
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ lsbp = VN_CAST(selp->lsbp(), Const)) {
                ಪ್ರತ್ಯಾಗಮ newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        ಯದಿ (AstConcat* ಧ್ರುವ catp = VN_CAST(nodep, Concat)) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rWidth = catp->rhsp()->width();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = lsb + width - 1;
            ಯದಿ (msb < rWidth) ಪ್ರತ್ಯಾಗಮ newSlice(catp->rhsp(), lsb, width);
            ಯದಿ (lsb >= rWidth) ಪ್ರತ್ಯಾಗಮ newSlice(catp->lhsp(), lsb - rWidth, width);
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        ಯದಿ (AstCond* ಧ್ರುವ condp = VN_CAST(nodep, Cond)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCond{flp, condp->condp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        ಯದಿ (AstAnd* ಧ್ರುವ andp = VN_CAST(nodep, And)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        ಯದಿ (AstOr* ಧ್ರುವ orp = VN_CAST(nodep, Or)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        ಯದಿ (AstXor* ಧ್ರುವ xorp = VN_CAST(nodep, Xor)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        ಯದಿ (AstExtend* ಧ್ರುವ extp = VN_CAST(nodep, Extend)) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ srcWidth = extp->lhsp()->width();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = lsb + width - 1;
            ಯದಿ (msb < srcWidth) ಪ್ರತ್ಯಾಗಮ newSlice(extp->lhsp(), lsb, width);
            ಯದಿ (lsb >= srcWidth) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, width, 0};
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConcat{
                flp, ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        ಯದಿ (AstNot* ಧ್ರುವ notp = VN_CAST(nodep, Not)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        ಯದಿ (AstReplicate* ಧ್ರುವ repp = VN_CAST(nodep, Replicate)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ srcp = repp->srcp();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ srcWidth = srcp->width();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = lsb + width - 1;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = nullptr;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partLsb = std::max(lsb, copyLsb) - copyLsb;
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? ನೂತನ AstConcat{flp, bitsp, resultp} : bitsp;
            }
            ಪ್ರತ್ಯಾಗಮ resultp;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ clonep = nodep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        ಯದಿ (lsb == 0 && width == nodep->width()) ಪ್ರತ್ಯಾಗಮ clonep;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> newAssignRhsps(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Ast_ಗಂಟು_DType* dtypep) {
        ಧ್ರುವ std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        ಯದಿ (AstCReset* ಧ್ರುವ cresetp = VN_CAST(nodep, CReset)) {
            ಪ್ರತಿ (ಧ್ರುವ Component& comp : compsr) {
                AstCReset* ಧ್ರುವ newp = cresetp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                newp->dtypep(comp.dtypep);
                valueps.ತಳ್ಳು_back(newp);
            }
            ಪ್ರತ್ಯಾಗಮ valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        ಯದಿ (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            ಪೂರ್ಣಾಙ್ಕ lsb = 0;  // LSB of the current term
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp : concatTerms(nodep)) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = lsb + termp->width() - 1;
                ಧ್ರುವ size_t first = componentIndex(dtypep, lsb);
                ಪ್ರತಿ (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    ಧ್ರುವ Component& comp = compsr[i];
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partLsb = std::max(lsb, comp.lsb);
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partMsb = std::min(msb, comp.msb);
                    File_ಸಾಲು* ಧ್ರುವ flp = termp->fileline();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? ನೂತನ AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            ಪ್ರತ್ಯಾಗಮ valueps;
        }

        // Otherwise unpacked, so select each component
        ಪ್ರತಿ (size_t i = 0; i < compsr.size(); ++i) {
            valueps.ತಳ್ಳು_back(newSelect(nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), dtypep, i));
        }
        ಪ್ರತ್ಯಾಗಮ valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newBits(File_ಸಾಲು* flp, ಧ್ರುವ Place* placep, ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ msb) {
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        ಯದಿ (!placep->split) {
            AstVarRef* ಧ್ರುವ refp = newRef(flp, placep, VAccess::READ);
            ಯದಿ (lsb == 0 && msb == dtypep->width() - 1) ಪ್ರತ್ಯಾಗಮ refp;
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ಧ್ರುವ std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = nullptr;
        ಧ್ರುವ size_t first = componentIndex(dtypep, lsb);
        ಪ್ರತಿ (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            ಧ್ರುವ Component& comp = comps[i];
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* ಧ್ರುವ childp = placep->childrenp.at(i).get();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? ನೂತನ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ addNewAssign(Ast_ಗಂಟು_Assign* origp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    ಶೂನ್ಯ expandSS(Ast_ಗಂಟು_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_ಸಾಲು* ಧ್ರುವ flp = origp->fileline();
        ಧ್ರುವ std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        ಯದಿ (isPacked(rPlacep->dtypep)) {
            ಪ್ರತಿ (size_t i = 0; i < compsr.size(); ++i) {
                Place* ಧ್ರುವ lChildp = lPlacep->childrenp.at(i).get();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (size_t i = 0; i < compsr.size(); ++i) {
                Place* ಧ್ರುವ lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    ಶೂನ್ಯ expandSN(Ast_ಗಂಟು_Assign* origp, Place* lPlacep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        ಧ್ರುವ std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        ಪ್ರತಿ (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    ಶೂನ್ಯ expandNS(Ast_ಗಂಟು_Assign* origp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Place* rPlacep) {
        File_ಸಾಲು* ಧ್ರುವ flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        ಯದಿ (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Unpacked of the same shape, component-wise
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ಧ್ರುವ ತಾರ್ಕಿಕ readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            ಧ್ರುವ VNUser2InUse user2InUse;
            ಪ್ರತ್ಯಾಗಮ lhsp->exists([](ಧ್ರುವ AstVarRef* refp) {
                AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
                ಯದಿ (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                ಯದಿ (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                ಪ್ರತ್ಯಾಗಮ vscp->user2() == 3;
            });
        }();
        ಯದಿ (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ಧ್ರುವ scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ಧ್ರುವ vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, ನೂತನ AstVarRef{flp, vscp, VAccess::READ});
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ size_t size = dtypeComponents(dtypep).size();
        ಪ್ರತಿ (size_t i = 0; i < size; ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newLhsp = newSelect(lhsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    ಶೂನ್ಯ expand(Ast_ಗಂಟು_Assign* origp,  //
                Place* lPlacep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp,  //
                Place* rPlacep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_ಸಾಲು* ಧ್ರುವ flp = origp->fileline();
        // A Place not split is referenced
        ಯದಿ (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        ಯದಿ (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        ಯದಿ (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } ಅನ್ಯಥಾ ಯದಿ (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } ಅನ್ಯಥಾ ಯದಿ (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } ಅನ್ಯಥಾ {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    ತಾರ್ಕಿಕ isSplitBetween(ಧ್ರುವ Place* placep, ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ msb) {
        ಯಾವತ್ (placep->split) {
            ಧ್ರುವ size_t idx = componentIndex(placep->dtypep, lsb);
            ಯದಿ (idx != componentIndex(placep->dtypep, msb)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಧ್ರುವ Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    ಶೂನ್ಯ hoistTerms(Ast_ಗಂಟು_Assign* origp, ಧ್ರುವ Place* lPlacep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = origp->rhsp();
        ಧ್ರುವ std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ಧ್ರುವ VNUser2InUse user2InUse;
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ಧ್ರುವ scopep = lPlacep->rootp->vscp->scopep();
        ಪೂರ್ಣಾಙ್ಕ lsb = 0;  // LSB of the current term
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp : termps) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ tLsb = lsb;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            ಯದಿ (!termp->user2()) ಅನುವರ್ತ;
            // Cheap to select from
            ಯದಿ (isCheap(termp)) {
                termp->user2(0);
                ಅನುವರ್ತ;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            ಯದಿ (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                ಅನುವರ್ತ;
            }
            // Bits can be selected without evaluating it more than once
            ಯದಿ (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                ಅನುವರ್ತ;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_ಸಾಲು* ಧ್ರುವ flp = termp->fileline();
            AstVarScope* ಧ್ರುವ vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(ನೂತನ AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ಧ್ರುವ tmpRefp = ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(ನೂತನ AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    ಸ್ಥಿರ ಶೂನ್ಯ markComponents(ಧ್ರುವ Place* placep) {
        ಪ್ರತಿ (ಧ್ರುವ std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(ಸತ್ಯ);
            ಯದಿ (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    ಶೂನ್ಯ rewriteAssignment(ಧ್ರುವ Assignment& assignment) {
        Place* ಧ್ರುವ lPlacep = assignment.lPlacep;
        Place* ಧ್ರುವ rPlacep = assignment.rPlacep;
        ಧ್ರುವ ತಾರ್ಕಿಕ lSplit = lPlacep && lPlacep->split;
        ಧ್ರುವ ತಾರ್ಕಿಕ rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        ಯದಿ (!lSplit && !rSplit) ಪ್ರತ್ಯಾಗಮ;

        Ast_ಗಂಟು_Assign* ಧ್ರುವ origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        ಯದಿ (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            ಧ್ರುವ VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = origp->rhsp();
            ಯದಿ (rhsp->exists([](ಧ್ರುವ AstVarRef* refp) { ಪ್ರತ್ಯಾಗಮ refp->varScopep()->user2(); })) {
                File_ಸಾಲು* ಧ್ರುವ flp = rhsp->fileline();
                AstScope* ಧ್ರುವ scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ಧ್ರುವ vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(ನೂತನ AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ಧ್ರುವ tmpRefp = ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(ನೂತನ AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        ಯದಿ (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = origp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        ಯದಿ (lSplit) VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
        ಯದಿ (rSplit) VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    ಸ್ಥಿರ AstActive* comboActive(AstScope* scopep) {
        ಯದಿ (!scopep->user1p()) scopep->user1p(scopep->comboActivep(ಸತ್ಯ));
        ಪ್ರತ್ಯಾಗಮ VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    ಶೂನ್ಯ driveFromComponents(Place* placep, AstActive* activep) {
        ಯದಿ (!placep->split) ಪ್ರತ್ಯಾಗಮ;
        AstVarScope* ಧ್ರುವ vscp = placep->vscp;
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        ಪ್ರತಿ (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* ಧ್ರುವ childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsRefp = ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(ನೂತನ AstAlways{ನೂತನ AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        ಪ್ರತಿ (ಧ್ರುವ std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        ಪ್ರತಿ (ಧ್ರುವ Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        ಪ್ರತಿ (Place* ಧ್ರುವ placep : m_state.rootps) {
            ಯದಿ (!placep->split) ಅನುವರ್ತ;
            ಧ್ರುವ AstVarScope* ಧ್ರುವ vscp = placep->vscp;
            // Only traced ones at this point
            ಯದಿ (!(vscp->varp()->isTrace() && vscp->isTrace())) ಅನುವರ್ತ;
            AstActive* ಧ್ರುವ activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

ಶೂನ್ಯ V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("decompose", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
