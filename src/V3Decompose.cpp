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

ವರ್ಗ Decompose_ತಳ VL_NOT_FINAL {
ಸಂರಕ್ಷಿತ:
    // TYPES

    // A component (element or member) of an aggregate type
    ರಚನೆ Component final {
        Ast_ಗಂಟು_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        ಪೂರ್ಣಾಂಕ declIdx;  // The declared index of the element, if an array
        ಪೂರ್ಣಾಂಕ lsb;  // LSB in the whole value, if packed
        ಪೂರ್ಣಾಂಕ msb;  // MSB in the whole value, if packed
    };

    ರಚನೆ ಇರಿಸು;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    ರಚನೆ Copy final {
        ಇರಿಸು* otherp;  // The Place at the other end
        // Following only for packed places
        ಪೂರ್ಣಾಂಕ lsb;  // The first bit covered of this end
        ಪೂರ್ಣಾಂಕ otherLsb;  // The first bit covered of the other end
        ಪೂರ್ಣಾಂಕ ಅಗಲ;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    ರಚನೆ ಇರಿಸು final {
        ಇರಿಸು* parentp = ಶೂನ್ಯಸೂಚಕ;  // The Place this is a component of, nullptr if original
        ಇರಿಸು* rootp = ಶೂನ್ಯಸೂಚಕ;  // The original variable (root of the tree), root points to itself
        Ast_ಗಂಟು_DType* dtypep = ಶೂನ್ಯಸೂಚಕ;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = ಶೂನ್ಯಸೂಚಕ;
        // The component Places created so far, by index, sized on the first one created
        std::ಸದಿಶ<std::unique_ptr<ಇರಿಸು>> childrenp;
        std::ಸದಿಶ<Copy> copies;  // The copies covering it, until split
        ತಾರ್ಕಿಕ splittable = ನಿಜ;  // Can be split (there is no reason not to)
        ತಾರ್ಕಿಕ ಬಯಸುತ್ತದೆ_ವಿಭಜಿಸು = ಸುಳ್ಳು;  // Should be split (if possible, do split)
        ತಾರ್ಕಿಕ ವಿಭಜಿಸು = ಸುಳ್ಳು;  // Place was split
        ತಾರ್ಕಿಕ queued = ಸುಳ್ಳು;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    ರಚನೆ Select final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;  // The select
        ಇರಿಸು* placep;  // The component
        ಪೂರ್ಣಾಂಕ lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    ರಚನೆ Assignment final {
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* assp;  // The assignment
        ಇರಿಸು* lPlacep;  // The Place the LHS addresses exactly, if any
        ಇರಿಸು* rPlacep;  // The Place the RHS addresses exactly, if any
    };

ಸಾರ್ವಜನಿಕ:
    // The state of the pass, shared by the steps
    ವರ್ಗ State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_user4_ಒಳ_Use;

    ಸಾರ್ವಜನಿಕ:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_ಗಂಟು_DType, std::ಸದಿಶ<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, ಇರಿಸು> places;
        std::ಸದಿಶ<ಇರಿಸು*> rootps;  // The original Places, in creation order
        std::ಸದಿಶ<std::ಸದಿಶ<Select>> chains;  // Select chains addressing components
        std::ಸದಿಶ<Assignment> assignments;  // Assignments to expand if a side is split
    };

ಸಂರಕ್ಷಿತ:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Decompose_ತಳ(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isUnpacked(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಆದರೆ (VN_IS(dtypep, UnpackArrayDType)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಸ್ಥಿರಾಂಕ AstStructDType* ಸ್ಥಿರಾಂಕ structp = VN_CAST(dtypep, StructDType);
        ಹಿಂತಿರುಗಿ structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isPacked(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackArrayDType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ಹಿಂತಿರುಗಿ arrayp->subDTypep()->ಅಗಲ() > 1;
        }
        ಸ್ಥಿರಾಂಕ AstStructDType* ಸ್ಥಿರಾಂಕ structp = VN_CAST(dtypep, StructDType);
        ಹಿಂತಿರುಗಿ structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSplittableType(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        ಆದರೆ (isPacked(dtypep)) ಹಿಂತಿರುಗಿ v3Global.opt.fDecomposePacked();
        ಆದರೆ (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructDType* ಸ್ಥಿರಾಂಕ structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ಆದರೆ (mp->valuep()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
            }
            ಹಿಂತಿರುಗಿ v3Global.opt.fDecomposeUnpacked();
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* dtype_ದ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) ಹಿಂತಿರುಗಿ refp->varp()->dtypep();
        ಹಿಂತಿರುಗಿ nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Component>& dtypeComponents(Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::ಸದಿಶ<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        ಆದರೆ (compsr.ಖಾಲಿಗಳು()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ packed = isPacked(dtypep);
                ಸ್ಥಿರಾಂಕ VNumRange range = arrayp->declRange();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rev = packed && range.ascending();  // Only for naming components
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subp = arrayp->subDTypep();
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < range.elements(); ++i) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = packed ? i * subp->ಅಗಲ() : 0;
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = packed ? lsb + subp->ಅಗಲ() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ತಳ್ಳು_ಹಿಂಭಾಗ({subp, ಶೂನ್ಯಸೂಚಕ, declIdx, lsb, msb});
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ AstStructDType* ಸ್ಥಿರಾಂಕ structp = VN_AS(dtypep, StructDType);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ packed = isPacked(dtypep);
                ಗಾಗಿ (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = packed ? mp->lsb() : 0;
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = packed ? lsb + mp->ಅಗಲ() - 1 : 0;
                    compsr.ತಳ್ಳು_ಹಿಂಭಾಗ({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::ಹಿಮ್ಮುಖ(compsr.begin(), compsr.ಕೊನೆ());
            }
        }

        // The components of this data type
        ಹಿಂತಿರುಗಿ compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    ಗಾತ್ರ_t component_ಸೂಚ್ಯಂಕ(Ast_ಗಂಟು_DType* dtypep, ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = std::ಅಧೋ_bound(compsr.begin(), compsr.ಕೊನೆ(), ಬಿಟ್_2,
                                         [](ಸ್ಥಿರಾಂಕ Component& comp, ಪೂರ್ಣಾಂಕ b) {  //
                                             ಹಿಂತಿರುಗಿ b > comp.msb;
                                         });
        ಹಿಂತಿರುಗಿ it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    ಇರಿಸು* component_ದ(ಇರಿಸು* placep, ಗಾತ್ರ_t idx) {
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Component>& comps = dtypeComponents(placep->dtypep);
        ಆದರೆ (placep->childrenp.ಖಾಲಿಗಳು()) placep->childrenp.resize(comps.ಗಾತ್ರ_2());
        std::unique_ptr<ಇರಿಸು>& childpr = placep->childrenp.at(idx);
        ಆದರೆ (!childpr) {
            childpr = std::make_unique<ಇರಿಸು>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->ಬಯಸುತ್ತದೆ_ವಿಭಜಿಸು = placep->rootp->vscp->varp()->attr_ವಿಭಜಿಸು_Var();
        }
        ಹಿಂತಿರುಗಿ childpr.get();
    }

    // Is 'nodep' cheap to clone
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isCheap(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Constants are cheap
        ಆದರೆ (VN_IS(nodep, Const)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // So are resets
        ಆದರೆ (VN_IS(nodep, CReset)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Variable references are cheap for the most part
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            ಆದರೆ (refp->varp()->isForced()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            ಆದರೆ (refp->varp()->isSc()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Otherwise the path rooted here is cheap
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        // Bit selects only with a constant LSB
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, Sel)) {
            ಹಿಂತಿರುಗಿ VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, ArraySel)) {
            ಹಿಂತಿರುಗಿ isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, StructSel)) {  //
            ಹಿಂತಿರುಗಿ isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstExtend* ಸ್ಥಿರಾಂಕ extp = VN_CAST(nodep, Extend)) {  //
            ಹಿಂತಿರುಗಿ isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

ವರ್ಗ Decompose_ದಾಖಲೆ final : ಸಾರ್ವಜನಿಕ VNVisitorConst, ಸಾರ್ವಜನಿಕ Decompose_ತಳ {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* m_assignp = ಶೂನ್ಯಸೂಚಕ;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ಸ್ಥಿರ ಶೂನ್ಯ warn_No_ವಿಭಜಿಸು(AstVar* varp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* wherep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* reasonp) {
        // Only warn if user requested splitting
        ಆದರೆ (!varp->attr_ವಿಭಜಿಸು_Var()) ಹಿಂತಿರುಗಿ;
        // A packed variable might still be split by V3BitBlast, which warns if not
        ಆದರೆ (isPacked(varp->dtypep())) ಹಿಂತಿರುಗಿ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::SPLITVAR, ನಿಜ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ಆದರೆ (!varp->user1()) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ eligible = [&]() {
                // Check data type
                ಆದರೆ (!isSplittableType(varp->dtypep())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Check properties
                ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ reasonp = varp->cannot_ವಿಭಜಿಸು_ಬಗೆ_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                ಆದರೆ (reasonp) warn_No_ವಿಭಜಿಸು(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ಹಿಂತಿರುಗಿ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ಹಿಂತಿರುಗಿ varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    ಇರಿಸು* ಇರಿಸು_ದ(ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
        ಆದರೆ (!isEligible(vscp->varp())) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಇರಿಸು& ಇರಿಸು_2 = m_state.places(vscp);
        ಆದರೆ (!ಇರಿಸು_2.rootp) {
            ಇರಿಸು_2.rootp = &ಇರಿಸು_2;
            ಇರಿಸು_2.dtypep = vscp->varp()->dtypep();
            ಇರಿಸು_2.vscp = vscp;
            ಇರಿಸು_2.ಬಯಸುತ್ತದೆ_ವಿಭಜಿಸು = vscp->varp()->attr_ವಿಭಜಿಸು_Var();
            m_state.rootps.ತಳ್ಳು_ಹಿಂಭಾಗ(&ಇರಿಸು_2);
        }
        ಹಿಂತಿರುಗಿ &ಇರಿಸು_2;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    ಸ್ಥಿರ ಶೂನ್ಯ ಖಂಡ(ಇರಿಸು* placep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* wherep) {
        placep->splittable = ಸುಳ್ಳು;
        // Warn on the variable only, a component is split as far as possible, as requested
        ಆದರೆ (!placep->parentp) {
            warn_No_ವಿಭಜಿಸು(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    ಇರಿಸು* resolveChain(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, std::ಸದಿಶ<Select>& chain) {
        // Array selects correspond one to one with a component select
        ಆದರೆ (AstArraySel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, ArraySel)) {
            ಆದರೆ (ಇರಿಸು* ಸ್ಥಿರಾಂಕ fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ bitp = VN_CAST(selp->bitp(), Const);
                ಆದರೆ (bitp && bitp->ಗಮ್ಯ_UQuad() < dtypeComponents(fromp->dtypep).ಗಾತ್ರ_2()) {
                    ಇರಿಸು* ಸ್ಥಿರಾಂಕ childp = component_ದ(fromp, bitp->ಗಮ್ಯ_UQuad());
                    fromp->ಬಯಸುತ್ತದೆ_ವಿಭಜಿಸು = ನಿಜ;
                    chain.ತಳ್ಳು_ಹಿಂಭಾಗ({selp, childp, 0});
                    ಹಿಂತಿರುಗಿ childp;
                }
                // Otherwise block splitting of the Place
                ಖಂಡ(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        // Struct selects correspond one to one with a component select
        ಆದರೆ (AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, StructSel)) {
            ಆದರೆ (ಇರಿಸು* ಸ್ಥಿರಾಂಕ fromp = resolveChain(selp->fromp(), chain)) {
                ಆದರೆ (!isUnpacked(fromp->dtypep)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                AstStructDType* ಸ್ಥಿರಾಂಕ structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                ಆದರೆ (!structp->user1_ಗಣ_Once()) {
                    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Component>& compsr = dtypeComponents(structp);
                    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < compsr.ಗಾತ್ರ_2(); ++i) compsr[i].memberp->user1(i);
                }
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ memberp = m_memberMap.ಕಂಡುಹಿಡಿ_Member(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                ಇರಿಸು* ಸ್ಥಿರಾಂಕ childp = component_ದ(fromp, memberp->user1());
                fromp->ಬಯಸುತ್ತದೆ_ವಿಭಜಿಸು = ನಿಜ;
                chain.ತಳ್ಳು_ಹಿಂಭಾಗ({selp, childp, 0});
                ಹಿಂತಿರುಗಿ childp;
            }
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, Sel)) {
            ಇರಿಸು* placep = resolveChain(selp->fromp(), chain);
            ಆದರೆ (!placep) {
                iterateConst(selp->lsbp());
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lsbp = VN_CAST(selp->lsbp(), Const);
            ಆದರೆ (!lsbp || lsbp->ಗಮ್ಯ_SInt() + selp->ಅಗಲ_Const() > placep->dtypep->ಅಗಲ()) {
                ಖಂಡ(placep, selp->fromp());
                iterateConst(selp->lsbp());
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
            // Descend into the Place containing the bits, until exact
            ಪೂರ್ಣಾಂಕ lsb = lsbp->ಗಮ್ಯ_SInt();
            ಪೂರ್ಣಾಂಕ msb = lsb + selp->ಅಗಲ_Const() - 1;
            ಆಗ (lsb != 0 || msb != placep->dtypep->ಅಗಲ() - 1) {
                // Within a component of a type not split
                ಆದರೆ (!isPacked(placep->dtypep)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t idx = component_ಸೂಚ್ಯಂಕ(placep->dtypep, lsb);
                // Crossing components prevents splitting
                ಆದರೆ (idx != component_ಸೂಚ್ಯಂಕ(placep->dtypep, msb)) {
                    ಖಂಡ(placep, selp->fromp());
                    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                }
                placep->ಬಯಸುತ್ತದೆ_ವಿಭಜಿಸು = ನಿಜ;
                ಸ್ಥಿರಾಂಕ Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = component_ದ(placep, idx);
                chain.ತಳ್ಳು_ಹಿಂಭಾಗ({selp, placep, lsb});
            }
            ಹಿಂತಿರುಗಿ placep;
        }
        // Base case
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarRef)) ಹಿಂತಿರುಗಿ ಇರಿಸು_ದ(refp);
        // Not a select chain
        iterateConst(exprp);
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    // 'nodep' references the Place 'placep' whole
    ಶೂನ್ಯ referenced_ಸಂಪೂರ್ಣ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಇರಿಸು* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        ಆದರೆ (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            ಹಿಂತಿರುಗಿ;
        }

        // Otherwise must block splitting of this Place
        ಖಂಡ(placep, nodep);
    }

    // Visit a select expression
    ಶೂನ್ಯ ಭೇಟಿ_Select(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::ಸದಿಶ<Select> chain;
        ಇರಿಸು* ಸ್ಥಿರಾಂಕ placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        ಆದರೆ (placep) referenced_ಸಂಪೂರ್ಣ(nodep, placep);
        // Record the chain for the rewriting phase
        ಆದರೆ (!chain.ಖಾಲಿಗಳು()) m_state.chains.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasReversedRange(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* aDTypep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* bDTypep) {
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        ಆದರೆ (!aArrayp || !bArrayp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಹಿಂತಿರುಗಿ hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    ಶೂನ್ಯ ಭೇಟಿ_Assignment(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ reversed = hasReversedRange(dtype_ದ(nodep->lhsp()), dtype_ದ(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? ಶೂನ್ಯಸೂಚಕ : nodep;
        iterateChildrenConst(nodep);
        ಆದರೆ (!m_assignp) ಹಿಂತಿರುಗಿ;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();
        ಇರಿಸು* ಸ್ಥಿರಾಂಕ lPlacep = lhsp->user1u().ಗಮ್ಯ<ಇರಿಸು*>();
        ಇರಿಸು* ಸ್ಥಿರಾಂಕ rPlacep = rhsp->user1u().ಗಮ್ಯ<ಇರಿಸು*>();

        // Is it a splittable assignment
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ splittable = [&]() {
            // Assignment between two Places is splittable
            ಆದರೆ (lPlacep && rPlacep) ಹಿಂತಿರುಗಿ ನಿಜ;
            // If the LHS is a Place, the RHS must be a splittable expression
            ಆದರೆ (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                ಆದರೆ (isPacked(lPlacep->dtypep)) ಹಿಂತಿರುಗಿ ನಿಜ;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                ಹಿಂತಿರುಗಿ isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            ಆದರೆ (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                ಆದರೆ (isPacked(rPlacep->dtypep)) ಹಿಂತಿರುಗಿ ನಿಜ;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                ಹಿಂತಿರುಗಿ isCheap(lhsp);
            }
            // Otherwise not splittable
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        ಆದರೆ (!splittable) {
            ಆದರೆ (rPlacep) ಖಂಡ(rPlacep, rhsp);
            ಆದರೆ (lPlacep) ಖಂಡ(lPlacep, lhsp);
            ಹಿಂತಿರುಗಿ;
        }

        // If both sides are places, record the whole copies for the Decision phase
        ಆದರೆ (lPlacep && rPlacep) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = lPlacep->dtypep->ಅಗಲ();  // Unused when unpacked
            lPlacep->copies.ತಳ್ಳು_ಹಿಂಭಾಗ({rPlacep, 0, 0, ಅಗಲ});
            rPlacep->copies.ತಳ್ಳು_ಹಿಂಭಾಗ({lPlacep, 0, 0, ಅಗಲ});
        }
        // Record for the rewriting phase
        m_state.assignments.ತಳ್ಳು_ಹಿಂಭಾಗ({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_DType*) override {}  // No references in data types
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override { ಭೇಟಿ_Select(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override { ಭೇಟಿ_Select(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override { ಭೇಟಿ_Select(nodep); }
    // VISITORS - Assignments recordable as Copy
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override { ಭೇಟಿ_Assignment(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override { ಭೇಟಿ_Assignment(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override { ಭೇಟಿ_Assignment(nodep); }
    // VISITORS - References to Places
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        ಆದರೆ (ಇರಿಸು* ಸ್ಥಿರಾಂಕ placep = ಇರಿಸು_ದ(nodep)) referenced_ಸಂಪೂರ್ಣ(nodep, placep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (!isEligible(varp)) ಹಿಂತಿರುಗಿ;
        varp->user1(2);  // Mark not eligible
        warn_No_ವಿಭಜಿಸು(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    Decompose_ದಾಖಲೆ(State& state, AstNetlist* netlistp)
        : Decompose_ತಳ{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        ಗಾಗಿ (ಇರಿಸು* ಸ್ಥಿರಾಂಕ placep : m_state.rootps) {
            ಆದರೆ (!isEligible(placep->vscp->varp())) placep->splittable = ಸುಳ್ಳು;
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(State& state, AstNetlist* netlistp) { Decompose_ದಾಖಲೆ{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

ವರ್ಗ DecomposeDecision final : ಸಾರ್ವಜನಿಕ Decompose_ತಳ {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // STATE
    std::ಸದಿಶ<ಇರಿಸು*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::ಸದಿಶ<AstVar*>> m_ವಿಭಜಿಸು_Varps;
    VDouble0 m_stat_ವಿಭಜಿಸು_Unpacked_Orig;  // Original AstVars of unpacked type split
    VDouble0 m_stat_ವಿಭಜಿಸು_Packed_Orig;  // Original AstVars of packed type split
    VDouble0 m_stat_ವಿಭಜಿಸು_Unpacked_Comp;  // Component AstVars of unpacked type split
    VDouble0 m_stat_ವಿಭಜಿಸು_Packed_Comp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    ಸ್ಥಿರ ತಾರ್ಕಿಕ can_ವಿಭಜಿಸು(ಸ್ಥಿರಾಂಕ ಇರಿಸು* placep) {
        // Not if not splittable
        ಆದರೆ (!placep->splittable) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Not if already split
        ಆದರೆ (placep->ವಿಭಜಿಸು) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Original variable or its parent was already split
        ಹಿಂತಿರುಗಿ !placep->parentp || placep->parentp->ವಿಭಜಿಸು;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    ಶೂನ್ಯ ಸರತಿಗೆ_ಸೇರಿಸು(ಇರಿಸು* placep) {
        ಆದರೆ (placep->queued || !placep->ಬಯಸುತ್ತದೆ_ವಿಭಜಿಸು || !can_ವಿಭಜಿಸು(placep)) ಹಿಂತಿರುಗಿ;
        placep->queued = ನಿಜ;
        m_worklist.ತಳ್ಳು_ಹಿಂಭಾಗ(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    ಶೂನ್ಯ createComponents(ಇರಿಸು* placep) {
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = placep->vscp;
        AstVar* ಸ್ಥಿರಾಂಕ varp = vscp->varp();
        std::ಸದಿಶ<AstVar*>& varps = m_ವಿಭಜಿಸು_Varps(varp);
        // Create the component AstVars on first encounter
        ಆದರೆ (varps.ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ packed = isPacked(varp->dtypep());
            ಆದರೆ (placep->parentp) {
                ++(packed ? m_stat_ವಿಭಜಿಸು_Packed_Comp : m_stat_ವಿಭಜಿಸು_Unpacked_Comp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ++(packed ? m_stat_ವಿಭಜಿಸು_Packed_Orig : m_stat_ವಿಭಜಿಸು_Unpacked_Orig);
            }
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = varp->fileline();
            ಸ್ಥಿರಾಂಕ VVarType varType = varp->varType();
            ಸ್ಥಿರಾಂಕ std::string name = varp->name();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Component& comp : dtypeComponents(varp->dtypep())) {
                ಸ್ಥಿರಾಂಕ std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(comp.declIdx) + "__KET__";
                AstVar* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagate_ವಿಭಜಿಸು_Attr_From(varp);
                varps.ತಳ್ಳು_ಹಿಂಭಾಗ(newp);
                varp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ಸ್ಥಿರಾಂಕ scopep = vscp->scopep();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < varps.ಗಾತ್ರ_2(); ++i) {
            AstVarScope* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVarScope{flp, scopep, varps[i]};
            component_ದ(placep, i)->vscp = newp;
            vscp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    ಶೂನ್ಯ ಕೂಡಿಸು_Copy(ಇರಿಸು* placep, ಸ್ಥಿರಾಂಕ Copy& copy) {
        UASSERT(!placep->ವಿಭಜಿಸು, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        ಆದರೆ (!placep->splittable) ಹಿಂತಿರುಗಿ;
        // Add the copy to the Place
        placep->copies.ತಳ್ಳು_ಹಿಂಭಾಗ(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        ಆದರೆ (!isPacked(placep->dtypep)) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (copy.lsb != 0 || copy.ಅಗಲ != placep->dtypep->ಅಗಲ()) {
            placep->ಬಯಸುತ್ತದೆ_ವಿಭಜಿಸು = ನಿಜ;
            ಸರತಿಗೆ_ಸೇರಿಸು(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    ಶೂನ್ಯ newCopy(ಇರಿಸು* ap, ಪೂರ್ಣಾಂಕ aLsb, ಇರಿಸು* bp, ಪೂರ್ಣಾಂಕ bLsb, ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        UASSERT(ap != bp, "Copy within a Place");
        ಕೂಡಿಸು_Copy(ap, {bp, aLsb, bLsb, ಅಗಲ});
        ಕೂಡಿಸು_Copy(bp, {ap, bLsb, aLsb, ಅಗಲ});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    ಶೂನ್ಯ ವಿಭಜಿಸು_Copy(ಇರಿಸು* placep, ಸ್ಥಿರಾಂಕ Copy& copy) {
        ಇರಿಸು* ಸ್ಥಿರಾಂಕ otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        ಆದರೆ (isPacked(placep->dtypep)) {
            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = copy.lsb;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = copy.lsb + copy.ಅಗಲ - 1;
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮೊದಲ = component_ಸೂಚ್ಯಂಕ(placep->dtypep, lsb);
            ಗಾಗಿ (ಗಾತ್ರ_t i = ಮೊದಲ; i < comps.ಗಾತ್ರ_2() && comps[i].lsb <= msb; ++i) {
                ಸ್ಥಿರಾಂಕ Component& comp = comps[i];
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partLsb = std::max(lsb, comp.lsb);
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partMsb = std::min(msb, comp.msb);
                newCopy(component_ದ(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->ಬಯಸುತ್ತದೆ_ವಿಭಜಿಸು = ನಿಜ;
        ಸರತಿಗೆ_ಸೇರಿಸು(otherp);
        // The sides are the same shape, so pairwise whole copies
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಗಾತ್ರ_2 = dtypeComponents(placep->dtypep).ಗಾತ್ರ_2();
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಗಾತ್ರ_2; ++i) {
            ಇರಿಸು* ಸ್ಥಿರಾಂಕ ap = component_ದ(placep, i);
            ಆದರೆ (!ap->splittable) ಮುಂದುವರಿ;
            ಇರಿಸು* ಸ್ಥಿರಾಂಕ bp = component_ದ(otherp, i);
            ಆದರೆ (!bp->splittable) ಮುಂದುವರಿ;
            newCopy(ap, 0, bp, 0, ap->dtypep->ಅಗಲ());
        }
    }

    // Split the Place
    ಶೂನ್ಯ ವಿಭಜಿಸು_ಇರಿಸು(ಇರಿಸು* placep) {
        UASSERT(!placep->ವಿಭಜಿಸು, "Place split twice");
        // It is now split
        placep->ವಿಭಜಿಸು = ನಿಜ;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಗಾತ್ರ_2 = dtypeComponents(placep->dtypep).ಗಾತ್ರ_2();
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಗಾತ್ರ_2; ++i) ಸರತಿಗೆ_ಸೇರಿಸು(component_ದ(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Copy& copy : placep->copies) {
            ಆದರೆ (!copy.otherp->ವಿಭಜಿಸು) ವಿಭಜಿಸು_Copy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ DecomposeDecision(State& state)
        : Decompose_ತಳ{state} {
        // Enqueue the original variables
        ಗಾಗಿ (ಇರಿಸು* ಸ್ಥಿರಾಂಕ placep : m_state.rootps) ಸರತಿಗೆ_ಸೇರಿಸು(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        ಆಗ (!m_worklist.ಖಾಲಿಗಳು()) {
            ಇರಿಸು* ಸ್ಥಿರಾಂಕ placep = m_worklist.ಹಿಂಭಾಗ();
            m_worklist.ತೆಗೆ_ಹಿಂಭಾಗ();
            placep->queued = ಸುಳ್ಳು;
            ವಿಭಜಿಸು_ಇರಿಸು(placep);
        }

        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Decompose, unpacked variables split",
                         m_stat_ವಿಭಜಿಸು_Unpacked_Orig);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Decompose, packed variables split",
                         m_stat_ವಿಭಜಿಸು_Packed_Orig);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Decompose, unpacked components split further",
                         m_stat_ವಿಭಜಿಸು_Unpacked_Comp);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Decompose, packed components split further",
                         m_stat_ವಿಭಜಿಸು_Packed_Comp);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

ವರ್ಗ DecomposeRewrite final : ಸಾರ್ವಜನಿಕ VNDeleter, ಸಾರ್ವಜನಿಕ Decompose_ತಳ {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    ಸ್ಥಿರ AstVarRef* newRef(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ ಇರಿಸು* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    ಶೂನ್ಯ rewriteChain(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ಸ್ಥಿರಾಂಕ Select* selectp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Select& select : chain) {
            ಆದರೆ (select.placep->parentp->ವಿಭಜಿಸು) selectp = &select;
        }
        ಆದರೆ (!selectp) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ಸ್ಥಿರಾಂಕ VAccess access = [&]() {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rootp = chain.front().exprp;
            ಆಗ (ನಿಜ) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಹಿಂತಿರುಗಿ VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = newRef(flp, selectp->placep, access);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, Sel)) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = selp->ಅಗಲ_Const();
            ಆದರೆ (ಅಗಲ != newp->ಅಗಲ()) newp = ಹೊಸ AstSel{flp, newp, selectp->lsb, ಅಗಲ};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSelect(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtypep, ಗಾತ್ರ_t idx) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = fromp->fileline();
        ಸ್ಥಿರಾಂಕ Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        ಆದರೆ (isPacked(dtypep)) {
            AstSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstSel{flp, fromp, comp.lsb, comp.dtypep->ಅಗಲ()};
            selp->dtypep(comp.dtypep);
            ಹಿಂತಿರುಗಿ selp;
        }
        // If unpacked array, it's an ArraySel
        ಆದರೆ (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ i = static_cast<ಪೂರ್ಣಾಂಕ>(idx);
            UASSERT_OBJ(static_cast<ಗಾತ್ರ_t>(i) == idx, fromp, "Index overflow");
            ಹಿಂತಿರುಗಿ ಹೊಸ AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        ಹಿಂತಿರುಗಿ selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    ಸ್ಥಿರ std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> concatTerms(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps;
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ರಾಶಿ{nodep};
        ಆಗ (!ರಾಶಿ.ಖಾಲಿಗಳು()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = ರಾಶಿ.ಹಿಂಭಾಗ();
            ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
            ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ concatp = VN_CAST(exprp, Concat)) {
                ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(concatp->lhsp());
                ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(concatp->rhsp());
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (AstReplicate* ಸ್ಥಿರಾಂಕ repp = VN_CAST(exprp, Replicate)) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ countp = VN_AS(repp->countp(), Const);
                ಗಾಗಿ (uint32_t i = 0; i < countp->ಗಮ್ಯ_UInt(); ++i) ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(repp->srcp());
                ಮುಂದುವರಿ;
            }
            termps.ತಳ್ಳು_ಹಿಂಭಾಗ(exprp);
        }
        ಹಿಂತಿರುಗಿ termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSliceable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, Sel)) {
            ಹಿಂತಿರುಗಿ VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConcat* ಸ್ಥಿರಾಂಕ catp = VN_CAST(nodep, Concat)) {
            ಹಿಂತಿರುಗಿ isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(nodep, Cond)) {
            ಹಿಂತಿರುಗಿ isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(nodep, And)) {
            ಹಿಂತಿರುಗಿ isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstOr* ಸ್ಥಿರಾಂಕ orp = VN_CAST(nodep, Or)) {
            ಹಿಂತಿರುಗಿ isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstXor* ಸ್ಥಿರಾಂಕ xorp = VN_CAST(nodep, Xor)) {
            ಹಿಂತಿರುಗಿ isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstExtend* ಸ್ಥಿರಾಂಕ extp = VN_CAST(nodep, Extend)) {
            ಹಿಂತಿರುಗಿ isSliceable(extp->lhsp());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstNot* ಸ್ಥಿರಾಂಕ notp = VN_CAST(nodep, Not)) {  //
            ಹಿಂತಿರುಗಿ isSliceable(notp->lhsp());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstReplicate* ಸ್ಥಿರಾಂಕ repp = VN_CAST(nodep, Replicate)) {
            ಹಿಂತಿರುಗಿ isSliceable(repp->srcp());
        }
        ಹಿಂತಿರುಗಿ isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_ತುಂಡು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        // A constant, as a new constant of the bits
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const)) {
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, ಅಗಲ};
            ಅಂಕೆ.opSel(constp->ಅಂಕೆ(), lsb + ಅಗಲ - 1, lsb);
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{flp, ಅಂಕೆ};
        }
        // A constant bit select, from the bits of what it selects from
        ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, Sel)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lsbp = VN_CAST(selp->lsbp(), Const)) {
                ಹಿಂತಿರುಗಿ new_ತುಂಡು(selp->fromp(), lsbp->ಗಮ್ಯ_SInt() + lsb, ಅಗಲ);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ catp = VN_CAST(nodep, Concat)) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ r_ಅಗಲ = catp->rhsp()->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = lsb + ಅಗಲ - 1;
            ಆದರೆ (msb < r_ಅಗಲ) ಹಿಂತಿರುಗಿ new_ತುಂಡು(catp->rhsp(), lsb, ಅಗಲ);
            ಆದರೆ (lsb >= r_ಅಗಲ) ಹಿಂತಿರುಗಿ new_ತುಂಡು(catp->lhsp(), lsb - r_ಅಗಲ, ಅಗಲ);
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConcat{flp, new_ತುಂಡು(catp->lhsp(), 0, msb - r_ಅಗಲ + 1),
                                 new_ತುಂಡು(catp->rhsp(), lsb, r_ಅಗಲ - lsb)};
        }
        ಆದರೆ (AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(nodep, Cond)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstCond{flp, condp->condp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                               new_ತುಂಡು(condp->thenp(), lsb, ಅಗಲ),
                               new_ತುಂಡು(condp->elsep(), lsb, ಅಗಲ)};
        }
        ಆದರೆ (AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(nodep, And)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstAnd{flp, new_ತುಂಡು(andp->lhsp(), lsb, ಅಗಲ),
                              new_ತುಂಡು(andp->rhsp(), lsb, ಅಗಲ)};
        }
        ಆದರೆ (AstOr* ಸ್ಥಿರಾಂಕ orp = VN_CAST(nodep, Or)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstOr{flp, new_ತುಂಡು(orp->lhsp(), lsb, ಅಗಲ),
                             new_ತುಂಡು(orp->rhsp(), lsb, ಅಗಲ)};
        }
        ಆದರೆ (AstXor* ಸ್ಥಿರಾಂಕ xorp = VN_CAST(nodep, Xor)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstXor{flp, new_ತುಂಡು(xorp->lhsp(), lsb, ಅಗಲ),
                              new_ತುಂಡು(xorp->rhsp(), lsb, ಅಗಲ)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        ಆದರೆ (AstExtend* ಸ್ಥಿರಾಂಕ extp = VN_CAST(nodep, Extend)) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ src_ಅಗಲ = extp->lhsp()->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = lsb + ಅಗಲ - 1;
            ಆದರೆ (msb < src_ಅಗಲ) ಹಿಂತಿರುಗಿ new_ತುಂಡು(extp->lhsp(), lsb, ಅಗಲ);
            ಆದರೆ (lsb >= src_ಅಗಲ) ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, ಅಗಲ, 0};
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConcat{
                flp, ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, msb - src_ಅಗಲ + 1, 0},
                new_ತುಂಡು(extp->lhsp(), lsb, src_ಅಗಲ - lsb)};
        }
        ಆದರೆ (AstNot* ಸ್ಥಿರಾಂಕ notp = VN_CAST(nodep, Not)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstNot{flp, new_ತುಂಡು(notp->lhsp(), lsb, ಅಗಲ)};
        }
        // A replication, from the portions of the copies overlapping the bits
        ಆದರೆ (AstReplicate* ಸ್ಥಿರಾಂಕ repp = VN_CAST(nodep, Replicate)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ srcp = repp->srcp();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ src_ಅಗಲ = srcp->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = lsb + ಅಗಲ - 1;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ copyLsb = lsb / src_ಅಗಲ * src_ಅಗಲ; copyLsb <= msb; copyLsb += src_ಅಗಲ) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partLsb = std::max(lsb, copyLsb) - copyLsb;
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partMsb = std::min(msb, copyLsb + src_ಅಗಲ - 1) - copyLsb;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bitsp = new_ತುಂಡು(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? ಹೊಸ AstConcat{flp, bitsp, resultp} : bitsp;
            }
            ಹಿಂತಿರುಗಿ resultp;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ clonep = nodep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        ಆದರೆ (lsb == 0 && ಅಗಲ == nodep->ಅಗಲ()) ಹಿಂತಿರುಗಿ clonep;
        ಹಿಂತಿರುಗಿ ಹೊಸ AstSel{flp, clonep, lsb, ಅಗಲ};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> new_ನಿಯೋಜಿಸು_Rhsps(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Ast_ಗಂಟು_DType* dtypep) {
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Component>& compsr = dtypeComponents(dtypep);
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> valueps;
        valueps.reserve(compsr.ಗಾತ್ರ_2());

        // If CReset, it is duplicated for each component
        ಆದರೆ (AstCReset* ಸ್ಥಿರಾಂಕ cresetp = VN_CAST(nodep, CReset)) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Component& comp : compsr) {
                AstCReset* ಸ್ಥಿರಾಂಕ newp = cresetp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                newp->dtypep(comp.dtypep);
                valueps.ತಳ್ಳು_ಹಿಂಭಾಗ(newp);
            }
            ಹಿಂತಿರುಗಿ valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        ಆದರೆ (isPacked(dtypep)) {
            valueps.resize(compsr.ಗಾತ್ರ_2(), ಶೂನ್ಯಸೂಚಕ);
            ಪೂರ್ಣಾಂಕ lsb = 0;  // LSB of the current term
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp : concatTerms(nodep)) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = lsb + termp->ಅಗಲ() - 1;
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮೊದಲ = component_ಸೂಚ್ಯಂಕ(dtypep, lsb);
                ಗಾಗಿ (ಗಾತ್ರ_t i = ಮೊದಲ; i < compsr.ಗಾತ್ರ_2() && compsr[i].lsb <= msb; ++i) {
                    ಸ್ಥಿರಾಂಕ Component& comp = compsr[i];
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partLsb = std::max(lsb, comp.lsb);
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partMsb = std::min(msb, comp.msb);
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = termp->fileline();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sp = new_ತುಂಡು(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? ಹೊಸ AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            ಹಿಂತಿರುಗಿ valueps;
        }

        // Otherwise unpacked, so select each component
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < compsr.ಗಾತ್ರ_2(); ++i) {
            valueps.ತಳ್ಳು_ಹಿಂಭಾಗ(newSelect(nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), dtypep, i));
        }
        ಹಿಂತಿರುಗಿ valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_ಬಿಟ್ಟುಗಳು(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ ಇರಿಸು* placep, ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ msb) {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->ವಿಭಜಿಸು, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        ಆದರೆ (!placep->ವಿಭಜಿಸು) {
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = newRef(flp, placep, VAccess::READ);
            ಆದರೆ (lsb == 0 && msb == dtypep->ಅಗಲ() - 1) ಹಿಂತಿರುಗಿ refp;
            ಹಿಂತಿರುಗಿ ಹೊಸ AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Component>& comps = dtypeComponents(dtypep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮೊದಲ = component_ಸೂಚ್ಯಂಕ(dtypep, lsb);
        ಗಾಗಿ (ಗಾತ್ರ_t i = ಮೊದಲ; i < comps.ಗಾತ್ರ_2() && comps[i].lsb <= msb; ++i) {
            ಸ್ಥಿರಾಂಕ Component& comp = comps[i];
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partMsb = std::min(msb, comp.msb) - comp.lsb;
            ಇರಿಸು* ಸ್ಥಿರಾಂಕ childp = placep->childrenp.at(i).get();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bitsp = new_ಬಿಟ್ಟುಗಳು(flp, childp, partLsb, partMsb);
            resultp = resultp ? ಹೊಸ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ಹಿಂತಿರುಗಿ resultp;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_New_ನಿಯೋಜಿಸು(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* origp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        origp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    ಶೂನ್ಯ expandSS(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* origp, ಇರಿಸು* lPlacep, ಇರಿಸು* rPlacep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = origp->fileline();
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        ಆದರೆ (isPacked(rPlacep->dtypep)) {
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < compsr.ಗಾತ್ರ_2(); ++i) {
                ಇರಿಸು* ಸ್ಥಿರಾಂಕ lChildp = lPlacep->childrenp.at(i).get();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rp = new_ಬಿಟ್ಟುಗಳು(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, rp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < compsr.ಗಾತ್ರ_2(); ++i) {
                ಇರಿಸು* ಸ್ಥಿರಾಂಕ lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, ಶೂನ್ಯಸೂಚಕ, rPlacep->childrenp.at(i).get(), ಶೂನ್ಯಸೂಚಕ);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    ಶೂನ್ಯ expandSN(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* origp, ಇರಿಸು* lPlacep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> valueps = new_ನಿಯೋಜಿಸು_Rhsps(rhsp, lPlacep->dtypep);
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < valueps.ಗಾತ್ರ_2(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, valueps[i]);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    ಶೂನ್ಯ expandNS(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* origp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಇರಿಸು* rPlacep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        ಆದರೆ (isPacked(rPlacep->dtypep)) {
            ಕೂಡಿಸು_New_ನಿಯೋಜಿಸು(origp, lhsp, new_ಬಿಟ್ಟುಗಳು(flp, rPlacep, 0, rPlacep->dtypep->ಅಗಲ() - 1));
            ಹಿಂತಿರುಗಿ;
        }
        // Unpacked of the same shape, component-wise
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = dtype_ದ(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ readsWrittenVar = !VN_IS(origp, ನಿಯೋಜಿಸು_Dly) && [&]() {
            ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
            ಹಿಂತಿರುಗಿ lhsp->exists([](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
                ಆದರೆ (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                ಆದರೆ (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                ಹಿಂತಿರುಗಿ vscp->user2() == 3;
            });
        }();
        ಆದರೆ (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ಸ್ಥಿರಾಂಕ scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, ಶೂನ್ಯಸೂಚಕ, ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, ಶೂನ್ಯಸೂಚಕ);
            ಕೂಡಿಸು_New_ನಿಯೋಜಿಸು(origp, lhsp, ಹೊಸ AstVarRef{flp, vscp, VAccess::READ});
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಗಾತ್ರ_2 = dtypeComponents(dtypep).ಗಾತ್ರ_2();
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಗಾತ್ರ_2; ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newLhsp = newSelect(lhsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), dtypep, i);
            expand(origp, ಶೂನ್ಯಸೂಚಕ, newLhsp, rPlacep->childrenp.at(i).get(), ಶೂನ್ಯಸೂಚಕ);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    ಶೂನ್ಯ expand(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* origp,  //
                ಇರಿಸು* lPlacep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp,  //
                ಇರಿಸು* rPlacep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = origp->fileline();
        // A Place not split is referenced
        ಆದರೆ (lPlacep && !lPlacep->ವಿಭಜಿಸು) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (rPlacep && !rPlacep->ವಿಭಜಿಸು) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_New_ನಿಯೋಜಿಸು(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    ತಾರ್ಕಿಕ is_ವಿಭಜಿಸು_Between(ಸ್ಥಿರಾಂಕ ಇರಿಸು* placep, ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ msb) {
        ಆಗ (placep->ವಿಭಜಿಸು) {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t idx = component_ಸೂಚ್ಯಂಕ(placep->dtypep, lsb);
            ಆದರೆ (idx != component_ಸೂಚ್ಯಂಕ(placep->dtypep, msb)) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಸ್ಥಿರಾಂಕ Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    ಶೂನ್ಯ hoistTerms(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* origp, ಸ್ಥಿರಾಂಕ ಇರಿಸು* lPlacep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = origp->rhsp();
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ಸ್ಥಿರಾಂಕ scopep = lPlacep->rootp->vscp->scopep();
        ಪೂರ್ಣಾಂಕ lsb = 0;  // LSB of the current term
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp : termps) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ tLsb = lsb;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ tMsb = lsb + termp->ಅಗಲ() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            ಆದರೆ (!termp->user2()) ಮುಂದುವರಿ;
            // Cheap to select from
            ಆದರೆ (isCheap(termp)) {
                termp->user2(0);
                ಮುಂದುವರಿ;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            ಆದರೆ (termp->is_ಶುದ್ಧ() && termp->user2() == 1 && !is_ವಿಭಜಿಸು_Between(lPlacep, tLsb, tMsb)) {
                ಮುಂದುವರಿ;
            }
            // Bits can be selected without evaluating it more than once
            ಆದರೆ (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                ಮುಂದುವರಿ;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = termp->fileline();
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(ಹೊಸ AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ಸ್ಥಿರಾಂಕ tmpRefp = ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE};
            origp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    ಸ್ಥಿರ ಶೂನ್ಯ markComponents(ಸ್ಥಿರಾಂಕ ಇರಿಸು* placep) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::unique_ptr<ಇರಿಸು>& childp : placep->childrenp) {
            childp->vscp->user2(ನಿಜ);
            ಆದರೆ (childp->ವಿಭಜಿಸು) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    ಶೂನ್ಯ rewriteAssignment(ಸ್ಥಿರಾಂಕ Assignment& assignment) {
        ಇರಿಸು* ಸ್ಥಿರಾಂಕ lPlacep = assignment.lPlacep;
        ಇರಿಸು* ಸ್ಥಿರಾಂಕ rPlacep = assignment.rPlacep;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ l_ವಿಭಜಿಸು = lPlacep && lPlacep->ವಿಭಜಿಸು;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ r_ವಿಭಜಿಸು = rPlacep && rPlacep->ವಿಭಜಿಸು;
        // Nothing to do if neither side is split
        ಆದರೆ (!l_ವಿಭಜಿಸು && !r_ವಿಭಜಿಸು) ಹಿಂತಿರುಗಿ;

        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        ಆದರೆ (l_ವಿಭಜಿಸು && !r_ವಿಭಜಿಸು && !VN_IS(origp, ನಿಯೋಜಿಸು_Dly)) {
            ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
            markComponents(lPlacep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = origp->rhsp();
            ಆದರೆ (rhsp->exists([](ಸ್ಥಿರಾಂಕ AstVarRef* refp) { ಹಿಂತಿರುಗಿ refp->varScopep()->user2(); })) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = rhsp->fileline();
                AstScope* ಸ್ಥಿರಾಂಕ scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = m_tmps.make(flp, scopep, dtype_ದ(rhsp));
                rhsp->replaceWith(ಹೊಸ AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ಸ್ಥಿರಾಂಕ tmpRefp = ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE};
                origp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        ಆದರೆ (l_ವಿಭಜಿಸು && !r_ವಿಭಜಿಸು && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = origp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = origp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               l_ವಿಭಜಿಸು ? lPlacep : ಶೂನ್ಯಸೂಚಕ, l_ವಿಭಜಿಸು ? ಶೂನ್ಯಸೂಚಕ : lhsp,  //
               r_ವಿಭಜಿಸು ? rPlacep : ಶೂನ್ಯಸೂಚಕ, r_ವಿಭಜಿಸು ? ಶೂನ್ಯಸೂಚಕ : rhsp);
        ಆದರೆ (l_ವಿಭಜಿಸು) VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
        ಆದರೆ (r_ವಿಭಜಿಸು) VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(origp->unlink_Fr_ಹಿಂಭಾಗ()), origp);
    }

    // The combinational AstActive of the scope, cached
    ಸ್ಥಿರ Ast_ಸಕ್ರಿಯ* combo_ಸಕ್ರಿಯ(AstScope* scopep) {
        ಆದರೆ (!scopep->user1p()) scopep->user1p(scopep->comboActivep(ನಿಜ));
        ಹಿಂತಿರುಗಿ VN_AS(scopep->user1p(), ಸಕ್ರಿಯ);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    ಶೂನ್ಯ driveFromComponents(ಇರಿಸು* placep, Ast_ಸಕ್ರಿಯ* activep) {
        ಆದರೆ (!placep->ವಿಭಜಿಸು) ಹಿಂತಿರುಗಿ;
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = placep->vscp;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < placep->childrenp.ಗಾತ್ರ_2(); ++i) {
            ಇರಿಸು* ಸ್ಥಿರಾಂಕ childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsRefp = ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ DecomposeRewrite(State& state)
        : Decompose_ತಳ{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Select>& chain : m_state.chains) rewriteChain(chain);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        ಗಾಗಿ (ಇರಿಸು* ಸ್ಥಿರಾಂಕ placep : m_state.rootps) {
            ಆದರೆ (!placep->ವಿಭಜಿಸು) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp = placep->vscp;
            // Only traced ones at this point
            ಆದರೆ (!(vscp->varp()->isTrace() && vscp->isTrace())) ಮುಂದುವರಿ;
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = combo_ಸಕ್ರಿಯ(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

ಶೂನ್ಯ V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        Decompose_ತಳ::State state;
        Decompose_ದಾಖಲೆ::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("decompose", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
