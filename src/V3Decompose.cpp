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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Decompose.h"

#অন্তর্ভাৱ "V3AstUserAllocator.h"
#অন্তর্ভাৱ "V3MemberMap.h"
#অন্তর্ভাৱ "V3SharedTmps.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

শ্ৰেণী DecomposeBase VL_NOT_FINAL {
রক্ষিত:
    // TYPES

    // A component (element or member) of an aggregate type
    সংরচনা Component final {
        Ast_গ্ৰন্থি_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        পূর্ণাঙ্ক declIdx;  // The declared index of the element, if an array
        পূর্ণাঙ্ক lsb;  // LSB in the whole value, if packed
        পূর্ণাঙ্ক msb;  // MSB in the whole value, if packed
    };

    সংরচনা Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    সংরচনা Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        পূর্ণাঙ্ক lsb;  // The first bit covered of this end
        পূর্ণাঙ্ক otherLsb;  // The first bit covered of the other end
        পূর্ণাঙ্ক width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    সংরচনা Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_গ্ৰন্থি_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        তার্কিক splittable = সঁচা;  // Can be split (there is no reason not to)
        তার্কিক wantsSplit = মিছা;  // Should be split (if possible, do split)
        তার্কিক split = মিছা;  // Place was split
        তার্কিক queued = মিছা;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    সংরচনা Select final {
        Ast_গ্ৰন্থি_ৰাশিমালা* exprp;  // The select
        Place* placep;  // The component
        পূর্ণাঙ্ক lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    সংরচনা Assignment final {
        Ast_গ্ৰন্থি_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

সার্ৱজনিক:
    // The state of the pass, shared by the steps
    শ্ৰেণী State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ধ্রুৱ VNUser4InUse m_user4InUse;

    সার্ৱজনিক:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_গ্ৰন্থি_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

রক্ষিত:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    স্থির তার্কিক isUnpacked(ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        যদি (VN_IS(dtypep, UnpackArrayDType)) ঘূৰাই সঁচা;
        ধ্রুৱ AstStructDType* ধ্রুৱ structp = VN_CAST(dtypep, StructDType);
        ঘূৰাই structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    স্থির তার্কিক isPacked(ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        যদি (ধ্রুৱ AstPackArrayDType* ধ্রুৱ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ঘূৰাই arrayp->subDTypep()->width() > 1;
        }
        ধ্রুৱ AstStructDType* ধ্রুৱ structp = VN_CAST(dtypep, StructDType);
        ঘূৰাই structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    স্থির তার্কিক isSplittableType(ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        যদি (isPacked(dtypep)) ঘূৰাই v3Global.opt.fDecomposePacked();
        যদি (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            যদি (ধ্রুৱ AstStructDType* ধ্রুৱ structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                বাবে (ধ্রুৱ AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    যদি (mp->valuep()) ঘূৰাই মিছা;
                }
            }
            ঘূৰাই v3Global.opt.fDecomposeUnpacked();
        }
        ঘূৰাই মিছা;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    স্থির Ast_গ্ৰন্থি_DType* dtypeOf(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(nodep, VarRef)) ঘূৰাই refp->varp()->dtypep();
        ঘূৰাই nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ধ্রুৱ std::vector<Component>& dtypeComponents(Ast_গ্ৰন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        যদি (compsr.empty()) {
            যদি (ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ arrayp = VN_CAST(dtypep, গ্ৰন্থি_Array_DType)) {
                ধ্রুৱ তার্কিক packed = isPacked(dtypep);
                ধ্রুৱ VNumRange range = arrayp->declRange();
                ধ্রুৱ তার্কিক rev = packed && range.ascending();  // Only for naming components
                Ast_গ্ৰন্থি_DType* ধ্রুৱ subp = arrayp->subDTypep();
                বাবে (পূর্ণাঙ্ক i = 0; i < range.elements(); ++i) {
                    ধ্রুৱ পূর্ণাঙ্ক lsb = packed ? i * subp->width() : 0;
                    ধ্রুৱ পূর্ণাঙ্ক msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ধ্রুৱ পূর্ণাঙ্ক declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ঠেলো_back({subp, nullptr, declIdx, lsb, msb});
                }
            } অন্যথা {
                ধ্রুৱ AstStructDType* ধ্রুৱ structp = VN_AS(dtypep, StructDType);
                ধ্রুৱ তার্কিক packed = isPacked(dtypep);
                বাবে (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ধ্রুৱ পূর্ণাঙ্ক lsb = packed ? mp->lsb() : 0;
                    ধ্রুৱ পূর্ণাঙ্ক msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.ঠেলো_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        ঘূৰাই compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(Ast_গ্ৰন্থি_DType* dtypep, পূর্ণাঙ্ক bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ধ্রুৱ std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ধ্রুৱ auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](ধ্রুৱ Component& comp, পূর্ণাঙ্ক b) {  //
                                             ঘূৰাই b > comp.msb;
                                         });
        ঘূৰাই it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        ধ্রুৱ std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        যদি (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        যদি (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        ঘূৰাই childpr.get();
    }

    // Is 'nodep' cheap to clone
    স্থির তার্কিক isCheap(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        // Constants are cheap
        যদি (VN_IS(nodep, Const)) ঘূৰাই সঁচা;
        // So are resets
        যদি (VN_IS(nodep, CReset)) ঘূৰাই সঁচা;
        // Variable references are cheap for the most part
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            যদি (refp->varp()->isForced()) ঘূৰাই মিছা;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            যদি (refp->varp()->isSc()) ঘূৰাই মিছা;
            // Otherwise the path rooted here is cheap
            ঘূৰাই সঁচা;
        }
        // Bit selects only with a constant LSB
        যদি (ধ্রুৱ AstSel* ধ্রুৱ selp = VN_CAST(nodep, Sel)) {
            ঘূৰাই VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        যদি (ধ্রুৱ AstArraySel* ধ্রুৱ selp = VN_CAST(nodep, ArraySel)) {
            ঘূৰাই isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        যদি (ধ্রুৱ AstStructSel* ধ্রুৱ selp = VN_CAST(nodep, StructSel)) {  //
            ঘূৰাই isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        যদি (ধ্রুৱ AstExtend* ধ্রুৱ extp = VN_CAST(nodep, Extend)) {  //
            ঘূৰাই isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        ঘূৰাই মিছা;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

শ্ৰেণী DecomposeRecord final : সার্ৱজনিক VNVisitorConst, সার্ৱজনিক DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ধ্রুৱ VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_গ্ৰন্থি_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    স্থির শূন্য warnNoSplit(AstVar* varp, ধ্রুৱ Ast_গ্ৰন্থি* wherep, ধ্রুৱ ৱর্ণ* reasonp) {
        // Only warn if user requested splitting
        যদি (!varp->attrSplitVar()) ঘূৰাই;
        // A packed variable might still be split by V3BitBlast, which warns if not
        যদি (isPacked(varp->dtypep())) ঘূৰাই;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, সঁচা);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    স্থির তার্কিক isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        যদি (!varp->user1()) {
            ধ্রুৱ তার্কিক eligible = [&]() {
                // Check data type
                যদি (!isSplittableType(varp->dtypep())) ঘূৰাই মিছা;
                // Check properties
                ধ্রুৱ ৱর্ণ* ধ্রুৱ reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                যদি (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ঘূৰাই !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ঘূৰাই varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(ধ্রুৱ AstVarRef* refp) {
        AstVarScope* ধ্রুৱ vscp = refp->varScopep();
        যদি (!isEligible(vscp->varp())) ঘূৰাই nullptr;
        Place& place = m_state.places(vscp);
        যদি (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.ঠেলো_back(&place);
        }
        ঘূৰাই &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    স্থির শূন্য block(Place* placep, ধ্রুৱ Ast_গ্ৰন্থি* wherep) {
        placep->splittable = মিছা;
        // Warn on the variable only, a component is split as far as possible, as requested
        যদি (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_গ্ৰন্থি_ৰাশিমালা* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        যদি (AstArraySel* ধ্রুৱ selp = VN_CAST(exprp, ArraySel)) {
            যদি (Place* ধ্রুৱ fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ধ্রুৱ AstConst* ধ্রুৱ bitp = VN_CAST(selp->bitp(), Const);
                যদি (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* ধ্রুৱ childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = সঁচা;
                    chain.ঠেলো_back({selp, childp, 0});
                    ঘূৰাই childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            ঘূৰাই nullptr;
        }
        // Struct selects correspond one to one with a component select
        যদি (AstStructSel* ধ্রুৱ selp = VN_CAST(exprp, StructSel)) {
            যদি (Place* ধ্রুৱ fromp = resolveChain(selp->fromp(), chain)) {
                যদি (!isUnpacked(fromp->dtypep)) ঘূৰাই nullptr;
                AstStructDType* ধ্রুৱ structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                যদি (!structp->user1SetOnce()) {
                    ধ্রুৱ std::vector<Component>& compsr = dtypeComponents(structp);
                    বাবে (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* ধ্রুৱ childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = সঁচা;
                chain.ঠেলো_back({selp, childp, 0});
                ঘূৰাই childp;
            }
            ঘূৰাই nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        যদি (AstSel* ধ্রুৱ selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            যদি (!placep) {
                iterateConst(selp->lsbp());
                ঘূৰাই nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ধ্রুৱ AstConst* ধ্রুৱ lsbp = VN_CAST(selp->lsbp(), Const);
            যদি (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                ঘূৰাই nullptr;
            }
            // Descend into the Place containing the bits, until exact
            পূর্ণাঙ্ক lsb = lsbp->toSInt();
            পূর্ণাঙ্ক msb = lsb + selp->widthConst() - 1;
            যেতিয়ালৈ (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                যদি (!isPacked(placep->dtypep)) ঘূৰাই nullptr;
                ধ্রুৱ size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                যদি (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    ঘূৰাই nullptr;
                }
                placep->wantsSplit = সঁচা;
                ধ্রুৱ Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.ঠেলো_back({selp, placep, lsb});
            }
            ঘূৰাই placep;
        }
        // Base case
        যদি (AstVarRef* ধ্রুৱ refp = VN_CAST(exprp, VarRef)) ঘূৰাই placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        ঘূৰাই nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    শূন্য referencedWhole(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        যদি (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            ঘূৰাই;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    শূন্য visitSelect(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* ধ্রুৱ placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        যদি (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        যদি (!chain.empty()) m_state.chains.ঠেলো_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    স্থির তার্কিক hasReversedRange(ধ্রুৱ Ast_গ্ৰন্থি_DType* aDTypep, ধ্রুৱ Ast_গ্ৰন্থি_DType* bDTypep) {
        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        যদি (!aArrayp || !bArrayp) ঘূৰাই মিছা;
        যদি (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) ঘূৰাই সঁচা;
        ঘূৰাই hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    শূন্য visitAssignment(Ast_গ্ৰন্থি_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ধ্রুৱ তার্কিক reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        যদি (!m_assignp) ঘূৰাই;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp();
        Place* ধ্রুৱ lPlacep = lhsp->user1u().to<Place*>();
        Place* ধ্রুৱ rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        ধ্রুৱ তার্কিক splittable = [&]() {
            // Assignment between two Places is splittable
            যদি (lPlacep && rPlacep) ঘূৰাই সঁচা;
            // If the LHS is a Place, the RHS must be a splittable expression
            যদি (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                যদি (isPacked(lPlacep->dtypep)) ঘূৰাই সঁচা;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                ঘূৰাই isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            যদি (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                যদি (isPacked(rPlacep->dtypep)) ঘূৰাই সঁচা;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                ঘূৰাই isCheap(lhsp);
            }
            // Otherwise not splittable
            ঘূৰাই মিছা;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        যদি (!splittable) {
            যদি (rPlacep) block(rPlacep, rhsp);
            যদি (lPlacep) block(lPlacep, lhsp);
            ঘূৰাই;
        }

        // If both sides are places, record the whole copies for the Decision phase
        যদি (lPlacep && rPlacep) {
            ধ্রুৱ পূর্ণাঙ্ক width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.ঠেলো_back({rPlacep, 0, 0, width});
            rPlacep->copies.ঠেলো_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.ঠেলো_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_DType*) override {}  // No references in data types
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    শূন্য visit(AstArraySel* nodep) override { visitSelect(nodep); }
    শূন্য visit(AstStructSel* nodep) override { visitSelect(nodep); }
    শূন্য visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    শূন্য visit(AstAssign* nodep) override { visitAssignment(nodep); }
    শূন্য visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    শূন্য visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    শূন্য visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        যদি (Place* ধ্রুৱ placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    শূন্য visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ধ্রুৱ varp = nodep->varp();
        যদি (!isEligible(varp)) ঘূৰাই;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        বাবে (Place* ধ্রুৱ placep : m_state.rootps) {
            যদি (!isEligible(placep->vscp->varp())) placep->splittable = মিছা;
        }
    }

সার্ৱজনিক:
    স্থির শূন্য apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

শ্ৰেণী DecomposeDecision final : সার্ৱজনিক DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ধ্রুৱ VNUser1InUse m_user1InUse;

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
    স্থির তার্কিক canSplit(ধ্রুৱ Place* placep) {
        // Not if not splittable
        যদি (!placep->splittable) ঘূৰাই মিছা;
        // Not if already split
        যদি (placep->split) ঘূৰাই মিছা;
        // Original variable or its parent was already split
        ঘূৰাই !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    শূন্য enqueue(Place* placep) {
        যদি (placep->queued || !placep->wantsSplit || !canSplit(placep)) ঘূৰাই;
        placep->queued = সঁচা;
        m_worklist.ঠেলো_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    শূন্য createComponents(Place* placep) {
        AstVarScope* ধ্রুৱ vscp = placep->vscp;
        AstVar* ধ্রুৱ varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        যদি (varps.empty()) {
            ধ্রুৱ তার্কিক packed = isPacked(varp->dtypep());
            যদি (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } অন্যথা {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_সাৰি* ধ্রুৱ flp = varp->fileline();
            ধ্রুৱ VVarType varType = varp->varType();
            ধ্রুৱ std::string name = varp->name();
            বাবে (ধ্রুৱ Component& comp : dtypeComponents(varp->dtypep())) {
                ধ্রুৱ std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_গ্ৰন্থি::encode_সংখ্যা(comp.declIdx) + "__KET__";
                AstVar* ধ্রুৱ newp = নূতন AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.ঠেলো_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ধ্রুৱ scopep = vscp->scopep();
        File_সাৰি* ধ্রুৱ flp = vscp->fileline();
        বাবে (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* ধ্রুৱ newp = নূতন AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    শূন্য addCopy(Place* placep, ধ্রুৱ Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        যদি (!placep->splittable) ঘূৰাই;
        // Add the copy to the Place
        placep->copies.ঠেলো_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        যদি (!isPacked(placep->dtypep)) ঘূৰাই;
        যদি (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = সঁচা;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    শূন্য newCopy(Place* ap, পূর্ণাঙ্ক aLsb, Place* bp, পূর্ণাঙ্ক bLsb, পূর্ণাঙ্ক width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    শূন্য splitCopy(Place* placep, ধ্রুৱ Copy& copy) {
        Place* ধ্রুৱ otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        যদি (isPacked(placep->dtypep)) {
            ধ্রুৱ std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ধ্রুৱ পূর্ণাঙ্ক lsb = copy.lsb;
            ধ্রুৱ পূর্ণাঙ্ক msb = copy.lsb + copy.width - 1;
            ধ্রুৱ size_t first = componentIndex(placep->dtypep, lsb);
            বাবে (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                ধ্রুৱ Component& comp = comps[i];
                ধ্রুৱ পূর্ণাঙ্ক partLsb = std::max(lsb, comp.lsb);
                ধ্রুৱ পূর্ণাঙ্ক partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            ঘূৰাই;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = সঁচা;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        ধ্রুৱ size_t size = dtypeComponents(placep->dtypep).size();
        বাবে (size_t i = 0; i < size; ++i) {
            Place* ধ্রুৱ ap = componentOf(placep, i);
            যদি (!ap->splittable) অনুৱর্ত;
            Place* ধ্রুৱ bp = componentOf(otherp, i);
            যদি (!bp->splittable) অনুৱর্ত;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    শূন্য splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = সঁচা;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ধ্রুৱ size_t size = dtypeComponents(placep->dtypep).size();
        বাবে (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        বাবে (ধ্রুৱ Copy& copy : placep->copies) {
            যদি (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        বাবে (Place* ধ্রুৱ placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        যেতিয়ালৈ (!m_worklist.empty()) {
            Place* ধ্রুৱ placep = m_worklist.back();
            m_worklist.বেৰ_কৰো_back();
            placep->queued = মিছা;
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

সার্ৱজনিক:
    স্থির শূন্য apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

শ্ৰেণী DecomposeRewrite final : সার্ৱজনিক VNDeleter, সার্ৱজনিক DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ধ্রুৱ VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    স্থির AstVarRef* newRef(File_সাৰি* flp, ধ্রুৱ Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        ঘূৰাই নূতন AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    শূন্য rewriteChain(ধ্রুৱ std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ধ্রুৱ Select* selectp = nullptr;
        বাবে (ধ্রুৱ Select& select : chain) {
            যদি (select.placep->parentp->split) selectp = &select;
        }
        যদি (!selectp) ঘূৰাই;
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ধ্রুৱ VAccess access = [&]() {
            ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* rootp = chain.front().exprp;
            যেতিয়ালৈ (সঁচা) {
                যদি (ধ্রুৱ AstArraySel* ধ্রুৱ selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } অন্যথা যদি (ধ্রুৱ AstStructSel* ধ্রুৱ selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } অন্যথা যদি (ধ্রুৱ AstSel* ধ্রুৱ selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } অন্যথা {
                    ঘূৰাই VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_সাৰি* ধ্রুৱ flp = exprp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* newp = newRef(flp, selectp->placep, access);
        যদি (ধ্রুৱ AstSel* ধ্রুৱ selp = VN_CAST(exprp, Sel)) {
            ধ্রুৱ পূর্ণাঙ্ক width = selp->widthConst();
            যদি (width != newp->width()) newp = নূতন AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_গ্ৰন্থি_ৰাশিমালা* newSelect(Ast_গ্ৰন্থি_ৰাশিমালা* fromp, Ast_গ্ৰন্থি_DType* dtypep, size_t idx) {
        File_সাৰি* ধ্রুৱ flp = fromp->fileline();
        ধ্রুৱ Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        যদি (isPacked(dtypep)) {
            AstSel* ধ্রুৱ selp = নূতন AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            ঘূৰাই selp;
        }
        // If unpacked array, it's an ArraySel
        যদি (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ধ্রুৱ পূর্ণাঙ্ক i = static_cast<পূর্ণাঙ্ক>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            ঘূৰাই নূতন AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ধ্রুৱ selp = নূতন AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        ঘূৰাই selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    স্থির std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> concatTerms(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> termps;
        std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> স্তূপ{nodep};
        যেতিয়ালৈ (!স্তূপ.empty()) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = স্তূপ.back();
            স্তূপ.বেৰ_কৰো_back();
            যদি (AstConcat* ধ্রুৱ concatp = VN_CAST(exprp, Concat)) {
                স্তূপ.ঠেলো_back(concatp->lhsp());
                স্তূপ.ঠেলো_back(concatp->rhsp());
                অনুৱর্ত;
            }
            যদি (AstReplicate* ধ্রুৱ repp = VN_CAST(exprp, Replicate)) {
                ধ্রুৱ AstConst* ধ্রুৱ countp = VN_AS(repp->countp(), Const);
                বাবে (uint32_t i = 0; i < countp->toUInt(); ++i) স্তূপ.ঠেলো_back(repp->srcp());
                অনুৱর্ত;
            }
            termps.ঠেলো_back(exprp);
        }
        ঘূৰাই termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    স্থির তার্কিক isSliceable(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যদি (ধ্রুৱ AstSel* ধ্রুৱ selp = VN_CAST(nodep, Sel)) {
            ঘূৰাই VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        যদি (ধ্রুৱ AstConcat* ধ্রুৱ catp = VN_CAST(nodep, Concat)) {
            ঘূৰাই isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        যদি (ধ্রুৱ AstCond* ধ্রুৱ condp = VN_CAST(nodep, Cond)) {
            ঘূৰাই isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        যদি (ধ্রুৱ AstAnd* ধ্রুৱ andp = VN_CAST(nodep, And)) {
            ঘূৰাই isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        যদি (ধ্রুৱ AstOr* ধ্রুৱ orp = VN_CAST(nodep, Or)) {
            ঘূৰাই isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        যদি (ধ্রুৱ AstXor* ধ্রুৱ xorp = VN_CAST(nodep, Xor)) {
            ঘূৰাই isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        যদি (ধ্রুৱ AstExtend* ধ্রুৱ extp = VN_CAST(nodep, Extend)) {
            ঘূৰাই isSliceable(extp->lhsp());
        }
        যদি (ধ্রুৱ AstNot* ধ্রুৱ notp = VN_CAST(nodep, Not)) {  //
            ঘূৰাই isSliceable(notp->lhsp());
        }
        যদি (ধ্রুৱ AstReplicate* ধ্রুৱ repp = VN_CAST(nodep, Replicate)) {
            ঘূৰাই isSliceable(repp->srcp());
        }
        ঘূৰাই isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newSlice(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক width) {
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        // A constant, as a new constant of the bits
        যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const)) {
            V3_সংখ্যা অঙ্ক{nodep, width};
            অঙ্ক.opSel(constp->অঙ্ক(), lsb + width - 1, lsb);
            ঘূৰাই নূতন AstConst{flp, অঙ্ক};
        }
        // A constant bit select, from the bits of what it selects from
        যদি (AstSel* ধ্রুৱ selp = VN_CAST(nodep, Sel)) {
            যদি (ধ্রুৱ AstConst* ধ্রুৱ lsbp = VN_CAST(selp->lsbp(), Const)) {
                ঘূৰাই newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        যদি (AstConcat* ধ্রুৱ catp = VN_CAST(nodep, Concat)) {
            ধ্রুৱ পূর্ণাঙ্ক rWidth = catp->rhsp()->width();
            ধ্রুৱ পূর্ণাঙ্ক msb = lsb + width - 1;
            যদি (msb < rWidth) ঘূৰাই newSlice(catp->rhsp(), lsb, width);
            যদি (lsb >= rWidth) ঘূৰাই newSlice(catp->lhsp(), lsb - rWidth, width);
            ঘূৰাই নূতন AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        যদি (AstCond* ধ্রুৱ condp = VN_CAST(nodep, Cond)) {
            ঘূৰাই নূতন AstCond{flp, condp->condp()->clone_বৃক্ষ_Pure(মিছা),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        যদি (AstAnd* ধ্রুৱ andp = VN_CAST(nodep, And)) {
            ঘূৰাই নূতন AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        যদি (AstOr* ধ্রুৱ orp = VN_CAST(nodep, Or)) {
            ঘূৰাই নূতন AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        যদি (AstXor* ধ্রুৱ xorp = VN_CAST(nodep, Xor)) {
            ঘূৰাই নূতন AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        যদি (AstExtend* ধ্রুৱ extp = VN_CAST(nodep, Extend)) {
            ধ্রুৱ পূর্ণাঙ্ক srcWidth = extp->lhsp()->width();
            ধ্রুৱ পূর্ণাঙ্ক msb = lsb + width - 1;
            যদি (msb < srcWidth) ঘূৰাই newSlice(extp->lhsp(), lsb, width);
            যদি (lsb >= srcWidth) ঘূৰাই নূতন AstConst{flp, AstConst::Widthed_মান{}, width, 0};
            ঘূৰাই নূতন AstConcat{
                flp, নূতন AstConst{flp, AstConst::Widthed_মান{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        যদি (AstNot* ধ্রুৱ notp = VN_CAST(nodep, Not)) {
            ঘূৰাই নূতন AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        যদি (AstReplicate* ধ্রুৱ repp = VN_CAST(nodep, Replicate)) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ srcp = repp->srcp();
            ধ্রুৱ পূর্ণাঙ্ক srcWidth = srcp->width();
            ধ্রুৱ পূর্ণাঙ্ক msb = lsb + width - 1;
            Ast_গ্ৰন্থি_ৰাশিমালা* resultp = nullptr;
            বাবে (পূর্ণাঙ্ক copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                ধ্রুৱ পূর্ণাঙ্ক partLsb = std::max(lsb, copyLsb) - copyLsb;
                ধ্রুৱ পূর্ণাঙ্ক partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? নূতন AstConcat{flp, bitsp, resultp} : bitsp;
            }
            ঘূৰাই resultp;
        }
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ clonep = nodep->clone_বৃক্ষ_Pure(মিছা);
        যদি (lsb == 0 && width == nodep->width()) ঘূৰাই clonep;
        ঘূৰাই নূতন AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> newAssignRhsps(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, Ast_গ্ৰন্থি_DType* dtypep) {
        ধ্রুৱ std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        যদি (AstCReset* ধ্রুৱ cresetp = VN_CAST(nodep, CReset)) {
            বাবে (ধ্রুৱ Component& comp : compsr) {
                AstCReset* ধ্রুৱ newp = cresetp->clone_বৃক্ষ(মিছা);
                newp->dtypep(comp.dtypep);
                valueps.ঠেলো_back(newp);
            }
            ঘূৰাই valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        যদি (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            পূর্ণাঙ্ক lsb = 0;  // LSB of the current term
            বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ termp : concatTerms(nodep)) {
                ধ্রুৱ পূর্ণাঙ্ক msb = lsb + termp->width() - 1;
                ধ্রুৱ size_t first = componentIndex(dtypep, lsb);
                বাবে (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    ধ্রুৱ Component& comp = compsr[i];
                    ধ্রুৱ পূর্ণাঙ্ক partLsb = std::max(lsb, comp.lsb);
                    ধ্রুৱ পূর্ণাঙ্ক partMsb = std::min(msb, comp.msb);
                    File_সাৰি* ধ্রুৱ flp = termp->fileline();
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? নূতন AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            ঘূৰাই valueps;
        }

        // Otherwise unpacked, so select each component
        বাবে (size_t i = 0; i < compsr.size(); ++i) {
            valueps.ঠেলো_back(newSelect(nodep->clone_বৃক্ষ(মিছা), dtypep, i));
        }
        ঘূৰাই valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_গ্ৰন্থি_ৰাশিমালা* newBits(File_সাৰি* flp, ধ্রুৱ Place* placep, পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক msb) {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        যদি (!placep->split) {
            AstVarRef* ধ্রুৱ refp = newRef(flp, placep, VAccess::READ);
            যদি (lsb == 0 && msb == dtypep->width() - 1) ঘূৰাই refp;
            ঘূৰাই নূতন AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ধ্রুৱ std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_গ্ৰন্থি_ৰাশিমালা* resultp = nullptr;
        ধ্রুৱ size_t first = componentIndex(dtypep, lsb);
        বাবে (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            ধ্রুৱ Component& comp = comps[i];
            ধ্রুৱ পূর্ণাঙ্ক partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ধ্রুৱ পূর্ণাঙ্ক partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* ধ্রুৱ childp = placep->childrenp.at(i).get();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? নূতন AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ঘূৰাই resultp;
    }

    স্থির শূন্য addNewAssign(Ast_গ্ৰন্থি_Assign* origp, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, Ast_গ্ৰন্থি_ৰাশিমালা* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    শূন্য expandSS(Ast_গ্ৰন্থি_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_সাৰি* ধ্রুৱ flp = origp->fileline();
        ধ্রুৱ std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        যদি (isPacked(rPlacep->dtypep)) {
            বাবে (size_t i = 0; i < compsr.size(); ++i) {
                Place* ধ্রুৱ lChildp = lPlacep->childrenp.at(i).get();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } অন্যথা {
            বাবে (size_t i = 0; i < compsr.size(); ++i) {
                Place* ধ্রুৱ lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    শূন্য expandSN(Ast_গ্ৰন্থি_Assign* origp, Place* lPlacep, Ast_গ্ৰন্থি_ৰাশিমালা* rhsp) {
        ধ্রুৱ std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        বাবে (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    শূন্য expandNS(Ast_গ্ৰন্থি_Assign* origp, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, Place* rPlacep) {
        File_সাৰি* ধ্রুৱ flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        যদি (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            ঘূৰাই;
        }
        // Unpacked of the same shape, component-wise
        Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ধ্রুৱ তার্কিক readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            ধ্রুৱ VNUser2InUse user2InUse;
            ঘূৰাই lhsp->exists([](ধ্রুৱ AstVarRef* refp) {
                AstVarScope* ধ্রুৱ vscp = refp->varScopep();
                যদি (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                যদি (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                ঘূৰাই vscp->user2() == 3;
            });
        }();
        যদি (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ধ্রুৱ scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ধ্রুৱ vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, নূতন AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, নূতন AstVarRef{flp, vscp, VAccess::READ});
            ঘূৰাই;
        }
        ধ্রুৱ size_t size = dtypeComponents(dtypep).size();
        বাবে (size_t i = 0; i < size; ++i) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newLhsp = newSelect(lhsp->clone_বৃক্ষ(মিছা), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    শূন্য expand(Ast_গ্ৰন্থি_Assign* origp,  //
                Place* lPlacep, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp,  //
                Place* rPlacep, Ast_গ্ৰন্থি_ৰাশিমালা* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_সাৰি* ধ্রুৱ flp = origp->fileline();
        // A Place not split is referenced
        যদি (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        যদি (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        যদি (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } অন্যথা যদি (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } অন্যথা যদি (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } অন্যথা {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    তার্কিক isSplitBetween(ধ্রুৱ Place* placep, পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক msb) {
        যেতিয়ালৈ (placep->split) {
            ধ্রুৱ size_t idx = componentIndex(placep->dtypep, lsb);
            যদি (idx != componentIndex(placep->dtypep, msb)) ঘূৰাই সঁচা;
            ধ্রুৱ Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        ঘূৰাই মিছা;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    শূন্য hoistTerms(Ast_গ্ৰন্থি_Assign* origp, ধ্রুৱ Place* lPlacep) {
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = origp->rhsp();
        ধ্রুৱ std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ধ্রুৱ VNUser2InUse user2InUse;
        বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ধ্রুৱ scopep = lPlacep->rootp->vscp->scopep();
        পূর্ণাঙ্ক lsb = 0;  // LSB of the current term
        বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ termp : termps) {
            ধ্রুৱ পূর্ণাঙ্ক tLsb = lsb;
            ধ্রুৱ পূর্ণাঙ্ক tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            যদি (!termp->user2()) অনুৱর্ত;
            // Cheap to select from
            যদি (isCheap(termp)) {
                termp->user2(0);
                অনুৱর্ত;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            যদি (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                অনুৱর্ত;
            }
            // Bits can be selected without evaluating it more than once
            যদি (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                অনুৱর্ত;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_সাৰি* ধ্রুৱ flp = termp->fileline();
            AstVarScope* ধ্রুৱ vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(নূতন AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ধ্রুৱ tmpRefp = নূতন AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(নূতন AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    স্থির শূন্য markComponents(ধ্রুৱ Place* placep) {
        বাবে (ধ্রুৱ std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(সঁচা);
            যদি (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    শূন্য rewriteAssignment(ধ্রুৱ Assignment& assignment) {
        Place* ধ্রুৱ lPlacep = assignment.lPlacep;
        Place* ধ্রুৱ rPlacep = assignment.rPlacep;
        ধ্রুৱ তার্কিক lSplit = lPlacep && lPlacep->split;
        ধ্রুৱ তার্কিক rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        যদি (!lSplit && !rSplit) ঘূৰাই;

        Ast_গ্ৰন্থি_Assign* ধ্রুৱ origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        যদি (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            ধ্রুৱ VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = origp->rhsp();
            যদি (rhsp->exists([](ধ্রুৱ AstVarRef* refp) { ঘূৰাই refp->varScopep()->user2(); })) {
                File_সাৰি* ধ্রুৱ flp = rhsp->fileline();
                AstScope* ধ্রুৱ scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ধ্রুৱ vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(নূতন AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ধ্রুৱ tmpRefp = নূতন AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(নূতন AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        যদি (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = origp->lhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        যদি (lSplit) VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
        যদি (rSplit) VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ঠেলো_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    স্থির AstActive* comboActive(AstScope* scopep) {
        যদি (!scopep->user1p()) scopep->user1p(scopep->comboActivep(সঁচা));
        ঘূৰাই VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    শূন্য driveFromComponents(Place* placep, AstActive* activep) {
        যদি (!placep->split) ঘূৰাই;
        AstVarScope* ধ্রুৱ vscp = placep->vscp;
        File_সাৰি* ধ্রুৱ flp = vscp->fileline();
        বাবে (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* ধ্রুৱ childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = নূতন AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsRefp = নূতন AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(নূতন AstAlways{নূতন AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        বাবে (ধ্রুৱ std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        বাবে (ধ্রুৱ Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        বাবে (Place* ধ্রুৱ placep : m_state.rootps) {
            যদি (!placep->split) অনুৱর্ত;
            ধ্রুৱ AstVarScope* ধ্রুৱ vscp = placep->vscp;
            // Only traced ones at this point
            যদি (!(vscp->varp()->isTrace() && vscp->isTrace())) অনুৱর্ত;
            AstActive* ধ্রুৱ activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

সার্ৱজনিক:
    স্থির শূন্য apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

শূন্য V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("decompose", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
