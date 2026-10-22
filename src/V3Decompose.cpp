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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Decompose.h"

#include "V3AstUserAllocator.h"
#include "V3MemberMap.h"
#include "V3SharedTmps.h"
#include "V3Stats.h"

#include <algorithm>
#include <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

class DecomposeBase VL_NOT_FINAL {
rak_sita:
    // TYPES

    // A component (element or member) of an aggregate type
    sa_nracanaa Component final {
        AstNodeDType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        puur_naa_nka declIdx;  // The declared index of the element, if an array
        puur_naa_nka lsb;  // LSB in the whole value, if packed
        puur_naa_nka msb;  // MSB in the whole value, if packed
    };

    sa_nracanaa Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    sa_nracanaa Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        puur_naa_nka lsb;  // The first bit covered of this end
        puur_naa_nka otherLsb;  // The first bit covered of the other end
        puur_naa_nka width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    sa_nracanaa Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        AstNodeDType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        bool splittable = true;  // Can be split (there is no reason not to)
        bool wantsSplit = false;  // Should be split (if possible, do split)
        bool split = false;  // Place was split
        bool queued = false;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    sa_nracanaa Select final {
        AstNodeExpr* exprp;  // The select
        Place* placep;  // The component
        puur_naa_nka lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    sa_nracanaa Assignment final {
        AstNodeAssign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

public:
    // The state of the pass, shared by the steps
    class State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        const VNUser4InUse m_user4InUse;

    public:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<AstNodeDType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

rak_sita:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    static bool isUnpacked(const AstNodeDType* dtypep) {
        dtypep = dtypep->skipRefp();
        ꦪꦼꦤ꧀ (VN_IS(dtypep, UnpackArrayDType)) ꦮꦭꦶ true;
        const AstStructDType* const structp = VN_CAST(dtypep, StructDType);
        ꦮꦭꦶ structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    static bool isPacked(const AstNodeDType* dtypep) {
        dtypep = dtypep->skipRefp();
        ꦪꦼꦤ꧀ (const AstPackArrayDType* const arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ꦮꦭꦶ arrayp->subDTypep()->width() > 1;
        }
        const AstStructDType* const structp = VN_CAST(dtypep, StructDType);
        ꦮꦭꦶ structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    static bool isSplittableType(const AstNodeDType* dtypep) {
        ꦪꦼꦤ꧀ (isPacked(dtypep)) ꦮꦭꦶ v3Global.opt.fDecomposePacked();
        ꦪꦼꦤ꧀ (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            ꦪꦼꦤ꧀ (const AstStructDType* const structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                ꦏꦁꦒꦺꦴ (const AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ꦪꦼꦤ꧀ (mp->valuep()) ꦮꦭꦶ false;
                }
            }
            ꦮꦭꦶ v3Global.opt.fDecomposeUnpacked();
        }
        ꦮꦭꦶ false;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    static AstNodeDType* dtypeOf(const AstNodeExpr* nodep) {
        ꦪꦼꦤ꧀ (const AstVarRef* const refp = VN_CAST(nodep, VarRef)) ꦮꦭꦶ refp->varp()->dtypep();
        ꦮꦭꦶ nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    const std::vector<Component>& dtypeComponents(AstNodeDType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        ꦪꦼꦤ꧀ (compsr.empty()) {
            ꦪꦼꦤ꧀ (const AstNodeArrayDType* const arrayp = VN_CAST(dtypep, NodeArrayDType)) {
                const bool packed = isPacked(dtypep);
                const VNumRange range = arrayp->declRange();
                const bool rev = packed && range.ascending();  // Only for naming components
                AstNodeDType* const subp = arrayp->subDTypep();
                ꦏꦁꦒꦺꦴ (puur_naa_nka i = 0; i < range.elements(); ++i) {
                    const puur_naa_nka lsb = packed ? i * subp->width() : 0;
                    const puur_naa_nka msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    const puur_naa_nka declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.push_back({subp, nullptr, declIdx, lsb, msb});
                }
            } ꦭꦶꦪ {
                const AstStructDType* const structp = VN_AS(dtypep, StructDType);
                const bool packed = isPacked(dtypep);
                ꦏꦁꦒꦺꦴ (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    const puur_naa_nka lsb = packed ? mp->lsb() : 0;
                    const puur_naa_nka msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.push_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        ꦮꦭꦶ compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(AstNodeDType* dtypep, puur_naa_nka bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        const std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        const auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](const Component& comp, puur_naa_nka b) {  //
                                             ꦮꦭꦶ b > comp.msb;
                                         });
        ꦮꦭꦶ it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        const std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        ꦪꦼꦤ꧀ (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        ꦪꦼꦤ꧀ (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        ꦮꦭꦶ childpr.get();
    }

    // Is 'nodep' cheap to clone
    static bool isCheap(const AstNodeExpr* nodep) {
        // Constants are cheap
        ꦪꦼꦤ꧀ (VN_IS(nodep, Const)) ꦮꦭꦶ true;
        // So are resets
        ꦪꦼꦤ꧀ (VN_IS(nodep, CReset)) ꦮꦭꦶ true;
        // Variable references are cheap for the most part
        ꦪꦼꦤ꧀ (const AstVarRef* const refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            ꦪꦼꦤ꧀ (refp->varp()->isForced()) ꦮꦭꦶ false;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            ꦪꦼꦤ꧀ (refp->varp()->isSc()) ꦮꦭꦶ false;
            // Otherwise the path rooted here is cheap
            ꦮꦭꦶ true;
        }
        // Bit selects only with a constant LSB
        ꦪꦼꦤ꧀ (const AstSel* const selp = VN_CAST(nodep, Sel)) {
            ꦮꦭꦶ VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        ꦪꦼꦤ꧀ (const AstArraySel* const selp = VN_CAST(nodep, ArraySel)) {
            ꦮꦭꦶ isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        ꦪꦼꦤ꧀ (const AstStructSel* const selp = VN_CAST(nodep, StructSel)) {  //
            ꦮꦭꦶ isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        ꦪꦼꦤ꧀ (const AstExtend* const extp = VN_CAST(nodep, Extend)) {  //
            ꦮꦭꦶ isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        ꦮꦭꦶ false;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

class DecomposeRecord final : public VNVisitorConst, public DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    const VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    AstNodeAssign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    static void warnNoSplit(AstVar* varp, const AstNode* wherep, const var_na* reasonp) {
        // Only warn if user requested splitting
        ꦪꦼꦤ꧀ (!varp->attrSplitVar()) ꦮꦭꦶ;
        // A packed variable might still be split by V3BitBlast, which warns if not
        ꦪꦼꦤ꧀ (isPacked(varp->dtypep())) ꦮꦭꦶ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, true);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    static bool isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ꦪꦼꦤ꧀ (!varp->user1()) {
            const bool eligible = [&]() {
                // Check data type
                ꦪꦼꦤ꧀ (!isSplittableType(varp->dtypep())) ꦮꦭꦶ false;
                // Check properties
                const var_na* const reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                ꦪꦼꦤ꧀ (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ꦮꦭꦶ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ꦮꦭꦶ varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(const AstVarRef* refp) {
        AstVarScope* const vscp = refp->varScopep();
        ꦪꦼꦤ꧀ (!isEligible(vscp->varp())) ꦮꦭꦶ nullptr;
        Place& place = m_state.places(vscp);
        ꦪꦼꦤ꧀ (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.push_back(&place);
        }
        ꦮꦭꦶ &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    static void block(Place* placep, const AstNode* wherep) {
        placep->splittable = false;
        // Warn on the variable only, a component is split as far as possible, as requested
        ꦪꦼꦤ꧀ (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(AstNodeExpr* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        ꦪꦼꦤ꧀ (AstArraySel* const selp = VN_CAST(exprp, ArraySel)) {
            ꦪꦼꦤ꧀ (Place* const fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                const AstConst* const bitp = VN_CAST(selp->bitp(), Const);
                ꦪꦼꦤ꧀ (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* const childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = true;
                    chain.push_back({selp, childp, 0});
                    ꦮꦭꦶ childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            ꦮꦭꦶ nullptr;
        }
        // Struct selects correspond one to one with a component select
        ꦪꦼꦤ꧀ (AstStructSel* const selp = VN_CAST(exprp, StructSel)) {
            ꦪꦼꦤ꧀ (Place* const fromp = resolveChain(selp->fromp(), chain)) {
                ꦪꦼꦤ꧀ (!isUnpacked(fromp->dtypep)) ꦮꦭꦶ nullptr;
                AstStructDType* const structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                ꦪꦼꦤ꧀ (!structp->user1SetOnce()) {
                    const std::vector<Component>& compsr = dtypeComponents(structp);
                    ꦏꦁꦒꦺꦴ (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                const AstNode* const memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* const childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = true;
                chain.push_back({selp, childp, 0});
                ꦮꦭꦶ childp;
            }
            ꦮꦭꦶ nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        ꦪꦼꦤ꧀ (AstSel* const selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            ꦪꦼꦤ꧀ (!placep) {
                iterateConst(selp->lsbp());
                ꦮꦭꦶ nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            const AstConst* const lsbp = VN_CAST(selp->lsbp(), Const);
            ꦪꦼꦤ꧀ (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                ꦮꦭꦶ nullptr;
            }
            // Descend into the Place containing the bits, until exact
            puur_naa_nka lsb = lsbp->toSInt();
            puur_naa_nka msb = lsb + selp->widthConst() - 1;
            ꦱꦼꦢꦼꦭ (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                ꦪꦼꦤ꧀ (!isPacked(placep->dtypep)) ꦮꦭꦶ nullptr;
                const size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                ꦪꦼꦤ꧀ (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    ꦮꦭꦶ nullptr;
                }
                placep->wantsSplit = true;
                const Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.push_back({selp, placep, lsb});
            }
            ꦮꦭꦶ placep;
        }
        // Base case
        ꦪꦼꦤ꧀ (AstVarRef* const refp = VN_CAST(exprp, VarRef)) ꦮꦭꦶ placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        ꦮꦭꦶ nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    void referencedWhole(AstNodeExpr* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        ꦪꦼꦤ꧀ (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            ꦮꦭꦶ;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    void visitSelect(AstNodeExpr* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* const placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        ꦪꦼꦤ꧀ (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        ꦪꦼꦤ꧀ (!chain.empty()) m_state.chains.push_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    static bool hasReversedRange(const AstNodeDType* aDTypep, const AstNodeDType* bDTypep) {
        const AstUnpackArrayDType* const aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        const AstUnpackArrayDType* const bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        ꦪꦼꦤ꧀ (!aArrayp || !bArrayp) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) ꦮꦭꦶ true;
        ꦮꦭꦶ hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    void visitAssignment(AstNodeAssign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        const bool reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (!m_assignp) ꦮꦭꦶ;

        // Note overlapping LHS/RHS handled in Rewrite

        AstNodeExpr* const lhsp = nodep->lhsp();
        AstNodeExpr* const rhsp = nodep->rhsp();
        Place* const lPlacep = lhsp->user1u().to<Place*>();
        Place* const rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        const bool splittable = [&]() {
            // Assignment between two Places is splittable
            ꦪꦼꦤ꧀ (lPlacep && rPlacep) ꦮꦭꦶ true;
            // If the LHS is a Place, the RHS must be a splittable expression
            ꦪꦼꦤ꧀ (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                ꦪꦼꦤ꧀ (isPacked(lPlacep->dtypep)) ꦮꦭꦶ true;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                ꦮꦭꦶ isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            ꦪꦼꦤ꧀ (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                ꦪꦼꦤ꧀ (isPacked(rPlacep->dtypep)) ꦮꦭꦶ true;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                ꦮꦭꦶ isCheap(lhsp);
            }
            // Otherwise not splittable
            ꦮꦭꦶ false;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        ꦪꦼꦤ꧀ (!splittable) {
            ꦪꦼꦤ꧀ (rPlacep) block(rPlacep, rhsp);
            ꦪꦼꦤ꧀ (lPlacep) block(lPlacep, lhsp);
            ꦮꦭꦶ;
        }

        // If both sides are places, record the whole copies for the Decision phase
        ꦪꦼꦤ꧀ (lPlacep && rPlacep) {
            const puur_naa_nka width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.push_back({rPlacep, 0, 0, width});
            rPlacep->copies.push_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.push_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    void visit(AstNodeDType*) override {}  // No references in data types
    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    void visit(AstArraySel* nodep) override { visitSelect(nodep); }
    void visit(AstStructSel* nodep) override { visitSelect(nodep); }
    void visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    void visit(AstAssign* nodep) override { visitAssignment(nodep); }
    void visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    void visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    void visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        ꦪꦼꦤ꧀ (Place* const placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    void visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* const varp = nodep->varp();
        ꦪꦼꦤ꧀ (!isEligible(varp)) ꦮꦭꦶ;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        ꦏꦁꦒꦺꦴ (Place* const placep : m_state.rootps) {
            ꦪꦼꦤ꧀ (!isEligible(placep->vscp->varp())) placep->splittable = false;
        }
    }

public:
    static void apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

class DecomposeDecision final : public DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    const VNUser1InUse m_user1InUse;

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
    static bool canSplit(const Place* placep) {
        // Not if not splittable
        ꦪꦼꦤ꧀ (!placep->splittable) ꦮꦭꦶ false;
        // Not if already split
        ꦪꦼꦤ꧀ (placep->split) ꦮꦭꦶ false;
        // Original variable or its parent was already split
        ꦮꦭꦶ !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    void enqueue(Place* placep) {
        ꦪꦼꦤ꧀ (placep->queued || !placep->wantsSplit || !canSplit(placep)) ꦮꦭꦶ;
        placep->queued = true;
        m_worklist.push_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    void createComponents(Place* placep) {
        AstVarScope* const vscp = placep->vscp;
        AstVar* const varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        ꦪꦼꦤ꧀ (varps.empty()) {
            const bool packed = isPacked(varp->dtypep());
            ꦪꦼꦤ꧀ (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } ꦭꦶꦪ {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            FileLine* const flp = varp->fileline();
            const VVarType varType = varp->varType();
            const std::string name = varp->name();
            ꦏꦁꦒꦺꦴ (const Component& comp : dtypeComponents(varp->dtypep())) {
                const std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + AstNode::encodeNumber(comp.declIdx) + "__KET__";
                AstVar* const newp = new AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.push_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* const scopep = vscp->scopep();
        FileLine* const flp = vscp->fileline();
        ꦏꦁꦒꦺꦴ (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* const newp = new AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    void addCopy(Place* placep, const Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        ꦪꦼꦤ꧀ (!placep->splittable) ꦮꦭꦶ;
        // Add the copy to the Place
        placep->copies.push_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        ꦪꦼꦤ꧀ (!isPacked(placep->dtypep)) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = true;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    void newCopy(Place* ap, puur_naa_nka aLsb, Place* bp, puur_naa_nka bLsb, puur_naa_nka width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    void splitCopy(Place* placep, const Copy& copy) {
        Place* const otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        ꦪꦼꦤ꧀ (isPacked(placep->dtypep)) {
            const std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            const puur_naa_nka lsb = copy.lsb;
            const puur_naa_nka msb = copy.lsb + copy.width - 1;
            const size_t first = componentIndex(placep->dtypep, lsb);
            ꦏꦁꦒꦺꦴ (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                const Component& comp = comps[i];
                const puur_naa_nka partLsb = std::max(lsb, comp.lsb);
                const puur_naa_nka partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            ꦮꦭꦶ;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = true;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        const size_t size = dtypeComponents(placep->dtypep).size();
        ꦏꦁꦒꦺꦴ (size_t i = 0; i < size; ++i) {
            Place* const ap = componentOf(placep, i);
            ꦪꦼꦤ꧀ (!ap->splittable) continue;
            Place* const bp = componentOf(otherp, i);
            ꦪꦼꦤ꧀ (!bp->splittable) continue;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    void splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = true;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        const size_t size = dtypeComponents(placep->dtypep).size();
        ꦏꦁꦒꦺꦴ (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        ꦏꦁꦒꦺꦴ (const Copy& copy : placep->copies) {
            ꦪꦼꦤ꧀ (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        ꦏꦁꦒꦺꦴ (Place* const placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        ꦱꦼꦢꦼꦭ (!m_worklist.empty()) {
            Place* const placep = m_worklist.back();
            m_worklist.pop_back();
            placep->queued = false;
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

public:
    static void apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

class DecomposeRewrite final : public VNDeleter, public DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    const VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    static AstVarRef* newRef(FileLine* flp, const Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        ꦮꦭꦶ new AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    void rewriteChain(const std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        const Select* selectp = nullptr;
        ꦏꦁꦒꦺꦴ (const Select& select : chain) {
            ꦪꦼꦤ꧀ (select.placep->parentp->split) selectp = &select;
        }
        ꦪꦼꦤ꧀ (!selectp) ꦮꦭꦶ;
        AstNodeExpr* const exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        const VAccess access = [&]() {
            const AstNodeExpr* rootp = chain.front().exprp;
            ꦱꦼꦢꦼꦭ (true) {
                ꦪꦼꦤ꧀ (const AstArraySel* const selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } ꦭꦶꦪ ꦪꦼꦤ꧀ (const AstStructSel* const selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } ꦭꦶꦪ ꦪꦼꦤ꧀ (const AstSel* const selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } ꦭꦶꦪ {
                    ꦮꦭꦶ VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        FileLine* const flp = exprp->fileline();
        AstNodeExpr* newp = newRef(flp, selectp->placep, access);
        ꦪꦼꦤ꧀ (const AstSel* const selp = VN_CAST(exprp, Sel)) {
            const puur_naa_nka width = selp->widthConst();
            ꦪꦼꦤ꧀ (width != newp->width()) newp = new AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    AstNodeExpr* newSelect(AstNodeExpr* fromp, AstNodeDType* dtypep, size_t idx) {
        FileLine* const flp = fromp->fileline();
        const Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        ꦪꦼꦤ꧀ (isPacked(dtypep)) {
            AstSel* const selp = new AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            ꦮꦭꦶ selp;
        }
        // If unpacked array, it's an ArraySel
        ꦪꦼꦤ꧀ (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            const puur_naa_nka i = static_cast<puur_naa_nka>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            ꦮꦭꦶ new AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* const selp = new AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        ꦮꦭꦶ selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    static std::vector<AstNodeExpr*> concatTerms(AstNodeExpr* nodep) {
        std::vector<AstNodeExpr*> termps;
        std::vector<AstNodeExpr*> stack{nodep};
        ꦱꦼꦢꦼꦭ (!stack.empty()) {
            AstNodeExpr* const exprp = stack.back();
            stack.pop_back();
            ꦪꦼꦤ꧀ (AstConcat* const concatp = VN_CAST(exprp, Concat)) {
                stack.push_back(concatp->lhsp());
                stack.push_back(concatp->rhsp());
                continue;
            }
            ꦪꦼꦤ꧀ (AstReplicate* const repp = VN_CAST(exprp, Replicate)) {
                const AstConst* const countp = VN_AS(repp->countp(), Const);
                ꦏꦁꦒꦺꦴ (uint32_t i = 0; i < countp->toUInt(); ++i) stack.push_back(repp->srcp());
                continue;
            }
            termps.push_back(exprp);
        }
        ꦮꦭꦶ termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    static bool isSliceable(const AstNodeExpr* nodep) {
        ꦪꦼꦤ꧀ (const AstSel* const selp = VN_CAST(nodep, Sel)) {
            ꦮꦭꦶ VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        ꦪꦼꦤ꧀ (const AstConcat* const catp = VN_CAST(nodep, Concat)) {
            ꦮꦭꦶ isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        ꦪꦼꦤ꧀ (const AstCond* const condp = VN_CAST(nodep, Cond)) {
            ꦮꦭꦶ isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        ꦪꦼꦤ꧀ (const AstAnd* const andp = VN_CAST(nodep, And)) {
            ꦮꦭꦶ isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        ꦪꦼꦤ꧀ (const AstOr* const orp = VN_CAST(nodep, Or)) {
            ꦮꦭꦶ isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        ꦪꦼꦤ꧀ (const AstXor* const xorp = VN_CAST(nodep, Xor)) {
            ꦮꦭꦶ isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        ꦪꦼꦤ꧀ (const AstExtend* const extp = VN_CAST(nodep, Extend)) {
            ꦮꦭꦶ isSliceable(extp->lhsp());
        }
        ꦪꦼꦤ꧀ (const AstNot* const notp = VN_CAST(nodep, Not)) {  //
            ꦮꦭꦶ isSliceable(notp->lhsp());
        }
        ꦪꦼꦤ꧀ (const AstReplicate* const repp = VN_CAST(nodep, Replicate)) {
            ꦮꦭꦶ isSliceable(repp->srcp());
        }
        ꦮꦭꦶ isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    static AstNodeExpr* newSlice(AstNodeExpr* nodep, puur_naa_nka lsb, puur_naa_nka width) {
        FileLine* const flp = nodep->fileline();
        // A constant, as a new constant of the bits
        ꦪꦼꦤ꧀ (const AstConst* const constp = VN_CAST(nodep, Const)) {
            V3Number num{nodep, width};
            num.opSel(constp->num(), lsb + width - 1, lsb);
            ꦮꦭꦶ new AstConst{flp, num};
        }
        // A constant bit select, from the bits of what it selects from
        ꦪꦼꦤ꧀ (AstSel* const selp = VN_CAST(nodep, Sel)) {
            ꦪꦼꦤ꧀ (const AstConst* const lsbp = VN_CAST(selp->lsbp(), Const)) {
                ꦮꦭꦶ newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        ꦪꦼꦤ꧀ (AstConcat* const catp = VN_CAST(nodep, Concat)) {
            const puur_naa_nka rWidth = catp->rhsp()->width();
            const puur_naa_nka msb = lsb + width - 1;
            ꦪꦼꦤ꧀ (msb < rWidth) ꦮꦭꦶ newSlice(catp->rhsp(), lsb, width);
            ꦪꦼꦤ꧀ (lsb >= rWidth) ꦮꦭꦶ newSlice(catp->lhsp(), lsb - rWidth, width);
            ꦮꦭꦶ new AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        ꦪꦼꦤ꧀ (AstCond* const condp = VN_CAST(nodep, Cond)) {
            ꦮꦭꦶ new AstCond{flp, condp->condp()->cloneTreePure(false),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        ꦪꦼꦤ꧀ (AstAnd* const andp = VN_CAST(nodep, And)) {
            ꦮꦭꦶ new AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        ꦪꦼꦤ꧀ (AstOr* const orp = VN_CAST(nodep, Or)) {
            ꦮꦭꦶ new AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        ꦪꦼꦤ꧀ (AstXor* const xorp = VN_CAST(nodep, Xor)) {
            ꦮꦭꦶ new AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        ꦪꦼꦤ꧀ (AstExtend* const extp = VN_CAST(nodep, Extend)) {
            const puur_naa_nka srcWidth = extp->lhsp()->width();
            const puur_naa_nka msb = lsb + width - 1;
            ꦪꦼꦤ꧀ (msb < srcWidth) ꦮꦭꦶ newSlice(extp->lhsp(), lsb, width);
            ꦪꦼꦤ꧀ (lsb >= srcWidth) ꦮꦭꦶ new AstConst{flp, AstConst::WidthedValue{}, width, 0};
            ꦮꦭꦶ new AstConcat{
                flp, new AstConst{flp, AstConst::WidthedValue{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        ꦪꦼꦤ꧀ (AstNot* const notp = VN_CAST(nodep, Not)) {
            ꦮꦭꦶ new AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        ꦪꦼꦤ꧀ (AstReplicate* const repp = VN_CAST(nodep, Replicate)) {
            AstNodeExpr* const srcp = repp->srcp();
            const puur_naa_nka srcWidth = srcp->width();
            const puur_naa_nka msb = lsb + width - 1;
            AstNodeExpr* resultp = nullptr;
            ꦏꦁꦒꦺꦴ (puur_naa_nka copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                const puur_naa_nka partLsb = std::max(lsb, copyLsb) - copyLsb;
                const puur_naa_nka partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                AstNodeExpr* const bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? new AstConcat{flp, bitsp, resultp} : bitsp;
            }
            ꦮꦭꦶ resultp;
        }
        AstNodeExpr* const clonep = nodep->cloneTreePure(false);
        ꦪꦼꦤ꧀ (lsb == 0 && width == nodep->width()) ꦮꦭꦶ clonep;
        ꦮꦭꦶ new AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<AstNodeExpr*> newAssignRhsps(AstNodeExpr* nodep, AstNodeDType* dtypep) {
        const std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<AstNodeExpr*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        ꦪꦼꦤ꧀ (AstCReset* const cresetp = VN_CAST(nodep, CReset)) {
            ꦏꦁꦒꦺꦴ (const Component& comp : compsr) {
                AstCReset* const newp = cresetp->cloneTree(false);
                newp->dtypep(comp.dtypep);
                valueps.push_back(newp);
            }
            ꦮꦭꦶ valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        ꦪꦼꦤ꧀ (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            puur_naa_nka lsb = 0;  // LSB of the current term
            ꦏꦁꦒꦺꦴ (AstNodeExpr* const termp : concatTerms(nodep)) {
                const puur_naa_nka msb = lsb + termp->width() - 1;
                const size_t first = componentIndex(dtypep, lsb);
                ꦏꦁꦒꦺꦴ (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    const Component& comp = compsr[i];
                    const puur_naa_nka partLsb = std::max(lsb, comp.lsb);
                    const puur_naa_nka partMsb = std::min(msb, comp.msb);
                    FileLine* const flp = termp->fileline();
                    AstNodeExpr* const sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? new AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            ꦮꦭꦶ valueps;
        }

        // Otherwise unpacked, so select each component
        ꦏꦁꦒꦺꦴ (size_t i = 0; i < compsr.size(); ++i) {
            valueps.push_back(newSelect(nodep->cloneTree(false), dtypep, i));
        }
        ꦮꦭꦶ valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    AstNodeExpr* newBits(FileLine* flp, const Place* placep, puur_naa_nka lsb, puur_naa_nka msb) {
        AstNodeDType* const dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        ꦪꦼꦤ꧀ (!placep->split) {
            AstVarRef* const refp = newRef(flp, placep, VAccess::READ);
            ꦪꦼꦤ꧀ (lsb == 0 && msb == dtypep->width() - 1) ꦮꦭꦶ refp;
            ꦮꦭꦶ new AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        const std::vector<Component>& comps = dtypeComponents(dtypep);
        AstNodeExpr* resultp = nullptr;
        const size_t first = componentIndex(dtypep, lsb);
        ꦏꦁꦒꦺꦴ (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            const Component& comp = comps[i];
            const puur_naa_nka partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            const puur_naa_nka partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* const childp = placep->childrenp.at(i).get();
            AstNodeExpr* const bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? new AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ꦮꦭꦶ resultp;
    }

    static void addNewAssign(AstNodeAssign* origp, AstNodeExpr* lhsp, AstNodeExpr* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    void expandSS(AstNodeAssign* origp, Place* lPlacep, Place* rPlacep) {
        FileLine* const flp = origp->fileline();
        const std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        ꦪꦼꦤ꧀ (isPacked(rPlacep->dtypep)) {
            ꦏꦁꦒꦺꦴ (size_t i = 0; i < compsr.size(); ++i) {
                Place* const lChildp = lPlacep->childrenp.at(i).get();
                AstNodeExpr* const rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } ꦭꦶꦪ {
            ꦏꦁꦒꦺꦴ (size_t i = 0; i < compsr.size(); ++i) {
                Place* const lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    void expandSN(AstNodeAssign* origp, Place* lPlacep, AstNodeExpr* rhsp) {
        const std::vector<AstNodeExpr*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        ꦏꦁꦒꦺꦴ (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(pushDeletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    void expandNS(AstNodeAssign* origp, AstNodeExpr* lhsp, Place* rPlacep) {
        FileLine* const flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        ꦪꦼꦤ꧀ (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            ꦮꦭꦶ;
        }
        // Unpacked of the same shape, component-wise
        AstNodeDType* const dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        const bool readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            const VNUser2InUse user2InUse;
            ꦮꦭꦶ lhsp->exists([](const AstVarRef* refp) {
                AstVarScope* const vscp = refp->varScopep();
                ꦪꦼꦤ꧀ (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                ꦪꦼꦤ꧀ (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                ꦮꦭꦶ vscp->user2() == 3;
            });
        }();
        ꦪꦼꦤ꧀ (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* const scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* const vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, new AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, new AstVarRef{flp, vscp, VAccess::READ});
            ꦮꦭꦶ;
        }
        const size_t size = dtypeComponents(dtypep).size();
        ꦏꦁꦒꦺꦴ (size_t i = 0; i < size; ++i) {
            AstNodeExpr* const newLhsp = newSelect(lhsp->cloneTree(false), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(pushDeletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    void expand(AstNodeAssign* origp,  //
                Place* lPlacep, AstNodeExpr* lhsp,  //
                Place* rPlacep, AstNodeExpr* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        FileLine* const flp = origp->fileline();
        // A Place not split is referenced
        ꦪꦼꦤ꧀ (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        ꦪꦼꦤ꧀ (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        ꦪꦼꦤ꧀ (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } ꦭꦶꦪ {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    bool isSplitBetween(const Place* placep, puur_naa_nka lsb, puur_naa_nka msb) {
        ꦱꦼꦢꦼꦭ (placep->split) {
            const size_t idx = componentIndex(placep->dtypep, lsb);
            ꦪꦼꦤ꧀ (idx != componentIndex(placep->dtypep, msb)) ꦮꦭꦶ true;
            const Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        ꦮꦭꦶ false;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    void hoistTerms(AstNodeAssign* origp, const Place* lPlacep) {
        AstNodeExpr* const rhsp = origp->rhsp();
        const std::vector<AstNodeExpr*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        const VNUser2InUse user2InUse;
        ꦏꦁꦒꦺꦴ (AstNodeExpr* const termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* const scopep = lPlacep->rootp->vscp->scopep();
        puur_naa_nka lsb = 0;  // LSB of the current term
        ꦏꦁꦒꦺꦴ (AstNodeExpr* const termp : termps) {
            const puur_naa_nka tLsb = lsb;
            const puur_naa_nka tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            ꦪꦼꦤ꧀ (!termp->user2()) continue;
            // Cheap to select from
            ꦪꦼꦤ꧀ (isCheap(termp)) {
                termp->user2(0);
                continue;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            ꦪꦼꦤ꧀ (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                continue;
            }
            // Bits can be selected without evaluating it more than once
            ꦪꦼꦤ꧀ (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                continue;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            FileLine* const flp = termp->fileline();
            AstVarScope* const vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(new AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* const tmpRefp = new AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(new AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    static void markComponents(const Place* placep) {
        ꦏꦁꦒꦺꦴ (const std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(true);
            ꦪꦼꦤ꧀ (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    void rewriteAssignment(const Assignment& assignment) {
        Place* const lPlacep = assignment.lPlacep;
        Place* const rPlacep = assignment.rPlacep;
        const bool lSplit = lPlacep && lPlacep->split;
        const bool rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        ꦪꦼꦤ꧀ (!lSplit && !rSplit) ꦮꦭꦶ;

        AstNodeAssign* const origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        ꦪꦼꦤ꧀ (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            const VNUser2InUse user2InUse;
            markComponents(lPlacep);
            AstNodeExpr* const rhsp = origp->rhsp();
            ꦪꦼꦤ꧀ (rhsp->exists([](const AstVarRef* refp) { ꦮꦭꦶ refp->varScopep()->user2(); })) {
                FileLine* const flp = rhsp->fileline();
                AstScope* const scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* const vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(new AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* const tmpRefp = new AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(new AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        ꦪꦼꦤ꧀ (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        AstNodeExpr* const lhsp = origp->lhsp()->unlinkFrBack();
        AstNodeExpr* const rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        ꦪꦼꦤ꧀ (lSplit) VL_DO_DANGLING(pushDeletep(lhsp), lhsp);
        ꦪꦼꦤ꧀ (rSplit) VL_DO_DANGLING(pushDeletep(rhsp), rhsp);
        VL_DO_DANGLING(pushDeletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    static AstActive* comboActive(AstScope* scopep) {
        ꦪꦼꦤ꧀ (!scopep->user1p()) scopep->user1p(scopep->comboActivep(true));
        ꦮꦭꦶ VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    void driveFromComponents(Place* placep, AstActive* activep) {
        ꦪꦼꦤ꧀ (!placep->split) ꦮꦭꦶ;
        AstVarScope* const vscp = placep->vscp;
        FileLine* const flp = vscp->fileline();
        ꦏꦁꦒꦺꦴ (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* const childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            AstNodeExpr* const rhsp = new AstVarRef{flp, childp->vscp, VAccess::READ};
            AstNodeExpr* const lhsRefp = new AstVarRef{flp, vscp, VAccess::WRITE};
            AstNodeExpr* const lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(new AstAlways{new AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        ꦏꦁꦒꦺꦴ (const std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        ꦏꦁꦒꦺꦴ (const Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        ꦏꦁꦒꦺꦴ (Place* const placep : m_state.rootps) {
            ꦪꦼꦤ꧀ (!placep->split) continue;
            const AstVarScope* const vscp = placep->vscp;
            // Only traced ones at this point
            ꦪꦼꦤ꧀ (!(vscp->varp()->isTrace() && vscp->isTrace())) continue;
            AstActive* const activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

public:
    static void apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

void V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("decompose", 0, dumpTreeEitherLevel() >= 3);
}
