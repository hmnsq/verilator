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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Decompose.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3MemberMap.h"
#અન્તર્ભાવ "V3SharedTmps.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

વર્ગ DecomposeBase VL_NOT_FINAL {
સુરક્ષિત:
    // TYPES

    // A component (element or member) of an aggregate type
    સંરચના Component final {
        Ast_ગાંઠ_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        પૂર્ણાંક declIdx;  // The declared index of the element, if an array
        પૂર્ણાંક lsb;  // LSB in the whole value, if packed
        પૂર્ણાંક msb;  // MSB in the whole value, if packed
    };

    સંરચના Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    સંરચના Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        પૂર્ણાંક lsb;  // The first bit covered of this end
        પૂર્ણાંક otherLsb;  // The first bit covered of the other end
        પૂર્ણાંક width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    સંરચના Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_ગાંઠ_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        તાર્કિક splittable = સાચું;  // Can be split (there is no reason not to)
        તાર્કિક wantsSplit = ખોટું;  // Should be split (if possible, do split)
        તાર્કિક split = ખોટું;  // Place was split
        તાર્કિક queued = ખોટું;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    સંરચના Select final {
        Ast_ગાંઠ_પદાવલિ* exprp;  // The select
        Place* placep;  // The component
        પૂર્ણાંક lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    સંરચના Assignment final {
        Ast_ગાંઠ_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

જાહેર:
    // The state of the pass, shared by the steps
    વર્ગ State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        અચલ VNUser4InUse m_user4InUse;

    જાહેર:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_ગાંઠ_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

સુરક્ષિત:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    સ્થિર તાર્કિક isUnpacked(અચલ Ast_ગાંઠ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        જો (VN_IS(dtypep, UnpackArrayDType)) પાછા સાચું;
        અચલ AstStructDType* અચલ structp = VN_CAST(dtypep, StructDType);
        પાછા structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    સ્થિર તાર્કિક isPacked(અચલ Ast_ગાંઠ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        જો (અચલ AstPackArrayDType* અચલ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            પાછા arrayp->subDTypep()->width() > 1;
        }
        અચલ AstStructDType* અચલ structp = VN_CAST(dtypep, StructDType);
        પાછા structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    સ્થિર તાર્કિક isSplittableType(અચલ Ast_ગાંઠ_DType* dtypep) {
        જો (isPacked(dtypep)) પાછા v3Global.opt.fDecomposePacked();
        જો (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            જો (અચલ AstStructDType* અચલ structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                માટે (અચલ AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    જો (mp->valuep()) પાછા ખોટું;
                }
            }
            પાછા v3Global.opt.fDecomposeUnpacked();
        }
        પાછા ખોટું;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    સ્થિર Ast_ગાંઠ_DType* dtypeOf(અચલ Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (અચલ AstVarRef* અચલ refp = VN_CAST(nodep, VarRef)) પાછા refp->varp()->dtypep();
        પાછા nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    અચલ std::vector<Component>& dtypeComponents(Ast_ગાંઠ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        જો (compsr.empty()) {
            જો (અચલ Ast_ગાંઠ_Array_DType* અચલ arrayp = VN_CAST(dtypep, ગાંઠ_Array_DType)) {
                અચલ તાર્કિક packed = isPacked(dtypep);
                અચલ VNumRange range = arrayp->declRange();
                અચલ તાર્કિક rev = packed && range.ascending();  // Only for naming components
                Ast_ગાંઠ_DType* અચલ subp = arrayp->subDTypep();
                માટે (પૂર્ણાંક i = 0; i < range.elements(); ++i) {
                    અચલ પૂર્ણાંક lsb = packed ? i * subp->width() : 0;
                    અચલ પૂર્ણાંક msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    અચલ પૂર્ણાંક declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ધકેલો_back({subp, nullptr, declIdx, lsb, msb});
                }
            } નહીંતર {
                અચલ AstStructDType* અચલ structp = VN_AS(dtypep, StructDType);
                અચલ તાર્કિક packed = isPacked(dtypep);
                માટે (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    અચલ પૂર્ણાંક lsb = packed ? mp->lsb() : 0;
                    અચલ પૂર્ણાંક msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.ધકેલો_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        પાછા compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(Ast_ગાંઠ_DType* dtypep, પૂર્ણાંક bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        અચલ std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        અચલ auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](અચલ Component& comp, પૂર્ણાંક b) {  //
                                             પાછા b > comp.msb;
                                         });
        પાછા it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        અચલ std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        જો (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        જો (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        પાછા childpr.get();
    }

    // Is 'nodep' cheap to clone
    સ્થિર તાર્કિક isCheap(અચલ Ast_ગાંઠ_પદાવલિ* nodep) {
        // Constants are cheap
        જો (VN_IS(nodep, Const)) પાછા સાચું;
        // So are resets
        જો (VN_IS(nodep, CReset)) પાછા સાચું;
        // Variable references are cheap for the most part
        જો (અચલ AstVarRef* અચલ refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            જો (refp->varp()->isForced()) પાછા ખોટું;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            જો (refp->varp()->isSc()) પાછા ખોટું;
            // Otherwise the path rooted here is cheap
            પાછા સાચું;
        }
        // Bit selects only with a constant LSB
        જો (અચલ AstSel* અચલ selp = VN_CAST(nodep, Sel)) {
            પાછા VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        જો (અચલ AstArraySel* અચલ selp = VN_CAST(nodep, ArraySel)) {
            પાછા isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        જો (અચલ AstStructSel* અચલ selp = VN_CAST(nodep, StructSel)) {  //
            પાછા isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        જો (અચલ AstExtend* અચલ extp = VN_CAST(nodep, Extend)) {  //
            પાછા isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        પાછા ખોટું;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

વર્ગ DecomposeRecord final : જાહેર VNVisitorConst, જાહેર DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    અચલ VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_ગાંઠ_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    સ્થિર શૂન્ય warnNoSplit(AstVar* varp, અચલ Ast_ગાંઠ* wherep, અચલ અક્ષર* reasonp) {
        // Only warn if user requested splitting
        જો (!varp->attrSplitVar()) પાછા;
        // A packed variable might still be split by V3BitBlast, which warns if not
        જો (isPacked(varp->dtypep())) પાછા;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, સાચું);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    સ્થિર તાર્કિક isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        જો (!varp->user1()) {
            અચલ તાર્કિક eligible = [&]() {
                // Check data type
                જો (!isSplittableType(varp->dtypep())) પાછા ખોટું;
                // Check properties
                અચલ અક્ષર* અચલ reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                જો (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                પાછા !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        પાછા varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(અચલ AstVarRef* refp) {
        AstVarScope* અચલ vscp = refp->varScopep();
        જો (!isEligible(vscp->varp())) પાછા nullptr;
        Place& place = m_state.places(vscp);
        જો (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.ધકેલો_back(&place);
        }
        પાછા &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    સ્થિર શૂન્ય block(Place* placep, અચલ Ast_ગાંઠ* wherep) {
        placep->splittable = ખોટું;
        // Warn on the variable only, a component is split as far as possible, as requested
        જો (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_ગાંઠ_પદાવલિ* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        જો (AstArraySel* અચલ selp = VN_CAST(exprp, ArraySel)) {
            જો (Place* અચલ fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                અચલ AstConst* અચલ bitp = VN_CAST(selp->bitp(), Const);
                જો (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* અચલ childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = સાચું;
                    chain.ધકેલો_back({selp, childp, 0});
                    પાછા childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            પાછા nullptr;
        }
        // Struct selects correspond one to one with a component select
        જો (AstStructSel* અચલ selp = VN_CAST(exprp, StructSel)) {
            જો (Place* અચલ fromp = resolveChain(selp->fromp(), chain)) {
                જો (!isUnpacked(fromp->dtypep)) પાછા nullptr;
                AstStructDType* અચલ structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                જો (!structp->user1SetOnce()) {
                    અચલ std::vector<Component>& compsr = dtypeComponents(structp);
                    માટે (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                અચલ Ast_ગાંઠ* અચલ memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* અચલ childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = સાચું;
                chain.ધકેલો_back({selp, childp, 0});
                પાછા childp;
            }
            પાછા nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        જો (AstSel* અચલ selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            જો (!placep) {
                iterateConst(selp->lsbp());
                પાછા nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            અચલ AstConst* અચલ lsbp = VN_CAST(selp->lsbp(), Const);
            જો (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                પાછા nullptr;
            }
            // Descend into the Place containing the bits, until exact
            પૂર્ણાંક lsb = lsbp->toSInt();
            પૂર્ણાંક msb = lsb + selp->widthConst() - 1;
            જ્યારે (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                જો (!isPacked(placep->dtypep)) પાછા nullptr;
                અચલ size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                જો (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    પાછા nullptr;
                }
                placep->wantsSplit = સાચું;
                અચલ Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.ધકેલો_back({selp, placep, lsb});
            }
            પાછા placep;
        }
        // Base case
        જો (AstVarRef* અચલ refp = VN_CAST(exprp, VarRef)) પાછા placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        પાછા nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    શૂન્ય referencedWhole(Ast_ગાંઠ_પદાવલિ* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        જો (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            પાછા;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    શૂન્ય visitSelect(Ast_ગાંઠ_પદાવલિ* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* અચલ placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        જો (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        જો (!chain.empty()) m_state.chains.ધકેલો_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    સ્થિર તાર્કિક hasReversedRange(અચલ Ast_ગાંઠ_DType* aDTypep, અચલ Ast_ગાંઠ_DType* bDTypep) {
        અચલ AstUnpackArrayDType* અચલ aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        અચલ AstUnpackArrayDType* અચલ bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        જો (!aArrayp || !bArrayp) પાછા ખોટું;
        જો (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) પાછા સાચું;
        પાછા hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    શૂન્ય visitAssignment(Ast_ગાંઠ_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        અચલ તાર્કિક reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        જો (!m_assignp) પાછા;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp();
        Place* અચલ lPlacep = lhsp->user1u().to<Place*>();
        Place* અચલ rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        અચલ તાર્કિક splittable = [&]() {
            // Assignment between two Places is splittable
            જો (lPlacep && rPlacep) પાછા સાચું;
            // If the LHS is a Place, the RHS must be a splittable expression
            જો (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                જો (isPacked(lPlacep->dtypep)) પાછા સાચું;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                પાછા isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            જો (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                જો (isPacked(rPlacep->dtypep)) પાછા સાચું;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                પાછા isCheap(lhsp);
            }
            // Otherwise not splittable
            પાછા ખોટું;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        જો (!splittable) {
            જો (rPlacep) block(rPlacep, rhsp);
            જો (lPlacep) block(lPlacep, lhsp);
            પાછા;
        }

        // If both sides are places, record the whole copies for the Decision phase
        જો (lPlacep && rPlacep) {
            અચલ પૂર્ણાંક width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.ધકેલો_back({rPlacep, 0, 0, width});
            rPlacep->copies.ધકેલો_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.ધકેલો_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_DType*) override {}  // No references in data types
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    શૂન્ય visit(AstArraySel* nodep) override { visitSelect(nodep); }
    શૂન્ય visit(AstStructSel* nodep) override { visitSelect(nodep); }
    શૂન્ય visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    શૂન્ય visit(AstAssign* nodep) override { visitAssignment(nodep); }
    શૂન્ય visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    શૂન્ય visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    શૂન્ય visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        જો (Place* અચલ placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* અચલ varp = nodep->varp();
        જો (!isEligible(varp)) પાછા;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        માટે (Place* અચલ placep : m_state.rootps) {
            જો (!isEligible(placep->vscp->varp())) placep->splittable = ખોટું;
        }
    }

જાહેર:
    સ્થિર શૂન્ય apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

વર્ગ DecomposeDecision final : જાહેર DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    અચલ VNUser1InUse m_user1InUse;

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
    સ્થિર તાર્કિક canSplit(અચલ Place* placep) {
        // Not if not splittable
        જો (!placep->splittable) પાછા ખોટું;
        // Not if already split
        જો (placep->split) પાછા ખોટું;
        // Original variable or its parent was already split
        પાછા !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    શૂન્ય enqueue(Place* placep) {
        જો (placep->queued || !placep->wantsSplit || !canSplit(placep)) પાછા;
        placep->queued = સાચું;
        m_worklist.ધકેલો_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    શૂન્ય createComponents(Place* placep) {
        AstVarScope* અચલ vscp = placep->vscp;
        AstVar* અચલ varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        જો (varps.empty()) {
            અચલ તાર્કિક packed = isPacked(varp->dtypep());
            જો (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } નહીંતર {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_લીટી* અચલ flp = varp->fileline();
            અચલ VVarType varType = varp->varType();
            અચલ std::string name = varp->name();
            માટે (અચલ Component& comp : dtypeComponents(varp->dtypep())) {
                અચલ std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_ગાંઠ::encode_સંખ્યા(comp.declIdx) + "__KET__";
                AstVar* અચલ newp = નવું AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.ધકેલો_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* અચલ scopep = vscp->scopep();
        File_લીટી* અચલ flp = vscp->fileline();
        માટે (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* અચલ newp = નવું AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    શૂન્ય addCopy(Place* placep, અચલ Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        જો (!placep->splittable) પાછા;
        // Add the copy to the Place
        placep->copies.ધકેલો_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        જો (!isPacked(placep->dtypep)) પાછા;
        જો (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = સાચું;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    શૂન્ય newCopy(Place* ap, પૂર્ણાંક aLsb, Place* bp, પૂર્ણાંક bLsb, પૂર્ણાંક width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    શૂન્ય splitCopy(Place* placep, અચલ Copy& copy) {
        Place* અચલ otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        જો (isPacked(placep->dtypep)) {
            અચલ std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            અચલ પૂર્ણાંક lsb = copy.lsb;
            અચલ પૂર્ણાંક msb = copy.lsb + copy.width - 1;
            અચલ size_t first = componentIndex(placep->dtypep, lsb);
            માટે (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                અચલ Component& comp = comps[i];
                અચલ પૂર્ણાંક partLsb = std::max(lsb, comp.lsb);
                અચલ પૂર્ણાંક partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            પાછા;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = સાચું;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        અચલ size_t size = dtypeComponents(placep->dtypep).size();
        માટે (size_t i = 0; i < size; ++i) {
            Place* અચલ ap = componentOf(placep, i);
            જો (!ap->splittable) ચાલુ;
            Place* અચલ bp = componentOf(otherp, i);
            જો (!bp->splittable) ચાલુ;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    શૂન્ય splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = સાચું;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        અચલ size_t size = dtypeComponents(placep->dtypep).size();
        માટે (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        માટે (અચલ Copy& copy : placep->copies) {
            જો (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        માટે (Place* અચલ placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        જ્યારે (!m_worklist.empty()) {
            Place* અચલ placep = m_worklist.back();
            m_worklist.કાઢો_back();
            placep->queued = ખોટું;
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

જાહેર:
    સ્થિર શૂન્ય apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

વર્ગ DecomposeRewrite final : જાહેર VNDeleter, જાહેર DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    અચલ VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    સ્થિર AstVarRef* newRef(File_લીટી* flp, અચલ Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        પાછા નવું AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    શૂન્ય rewriteChain(અચલ std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        અચલ Select* selectp = nullptr;
        માટે (અચલ Select& select : chain) {
            જો (select.placep->parentp->split) selectp = &select;
        }
        જો (!selectp) પાછા;
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        અચલ VAccess access = [&]() {
            અચલ Ast_ગાંઠ_પદાવલિ* rootp = chain.front().exprp;
            જ્યારે (સાચું) {
                જો (અચલ AstArraySel* અચલ selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } નહીંતર જો (અચલ AstStructSel* અચલ selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } નહીંતર જો (અચલ AstSel* અચલ selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } નહીંતર {
                    પાછા VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_લીટી* અચલ flp = exprp->fileline();
        Ast_ગાંઠ_પદાવલિ* newp = newRef(flp, selectp->placep, access);
        જો (અચલ AstSel* અચલ selp = VN_CAST(exprp, Sel)) {
            અચલ પૂર્ણાંક width = selp->widthConst();
            જો (width != newp->width()) newp = નવું AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_ગાંઠ_પદાવલિ* newSelect(Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_DType* dtypep, size_t idx) {
        File_લીટી* અચલ flp = fromp->fileline();
        અચલ Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        જો (isPacked(dtypep)) {
            AstSel* અચલ selp = નવું AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            પાછા selp;
        }
        // If unpacked array, it's an ArraySel
        જો (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            અચલ પૂર્ણાંક i = static_cast<પૂર્ણાંક>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            પાછા નવું AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* અચલ selp = નવું AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        પાછા selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    સ્થિર std::vector<Ast_ગાંઠ_પદાવલિ*> concatTerms(Ast_ગાંઠ_પદાવલિ* nodep) {
        std::vector<Ast_ગાંઠ_પદાવલિ*> termps;
        std::vector<Ast_ગાંઠ_પદાવલિ*> ઢગલો{nodep};
        જ્યારે (!ઢગલો.empty()) {
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = ઢગલો.back();
            ઢગલો.કાઢો_back();
            જો (AstConcat* અચલ concatp = VN_CAST(exprp, Concat)) {
                ઢગલો.ધકેલો_back(concatp->lhsp());
                ઢગલો.ધકેલો_back(concatp->rhsp());
                ચાલુ;
            }
            જો (AstReplicate* અચલ repp = VN_CAST(exprp, Replicate)) {
                અચલ AstConst* અચલ countp = VN_AS(repp->countp(), Const);
                માટે (uint32_t i = 0; i < countp->toUInt(); ++i) ઢગલો.ધકેલો_back(repp->srcp());
                ચાલુ;
            }
            termps.ધકેલો_back(exprp);
        }
        પાછા termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    સ્થિર તાર્કિક isSliceable(અચલ Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (અચલ AstSel* અચલ selp = VN_CAST(nodep, Sel)) {
            પાછા VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        જો (અચલ AstConcat* અચલ catp = VN_CAST(nodep, Concat)) {
            પાછા isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        જો (અચલ AstCond* અચલ condp = VN_CAST(nodep, Cond)) {
            પાછા isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        જો (અચલ AstAnd* અચલ andp = VN_CAST(nodep, And)) {
            પાછા isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        જો (અચલ AstOr* અચલ orp = VN_CAST(nodep, Or)) {
            પાછા isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        જો (અચલ AstXor* અચલ xorp = VN_CAST(nodep, Xor)) {
            પાછા isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        જો (અચલ AstExtend* અચલ extp = VN_CAST(nodep, Extend)) {
            પાછા isSliceable(extp->lhsp());
        }
        જો (અચલ AstNot* અચલ notp = VN_CAST(nodep, Not)) {  //
            પાછા isSliceable(notp->lhsp());
        }
        જો (અચલ AstReplicate* અચલ repp = VN_CAST(nodep, Replicate)) {
            પાછા isSliceable(repp->srcp());
        }
        પાછા isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    સ્થિર Ast_ગાંઠ_પદાવલિ* newSlice(Ast_ગાંઠ_પદાવલિ* nodep, પૂર્ણાંક lsb, પૂર્ણાંક width) {
        File_લીટી* અચલ flp = nodep->fileline();
        // A constant, as a new constant of the bits
        જો (અચલ AstConst* અચલ constp = VN_CAST(nodep, Const)) {
            V3_સંખ્યા અંક{nodep, width};
            અંક.opSel(constp->અંક(), lsb + width - 1, lsb);
            પાછા નવું AstConst{flp, અંક};
        }
        // A constant bit select, from the bits of what it selects from
        જો (AstSel* અચલ selp = VN_CAST(nodep, Sel)) {
            જો (અચલ AstConst* અચલ lsbp = VN_CAST(selp->lsbp(), Const)) {
                પાછા newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        જો (AstConcat* અચલ catp = VN_CAST(nodep, Concat)) {
            અચલ પૂર્ણાંક rWidth = catp->rhsp()->width();
            અચલ પૂર્ણાંક msb = lsb + width - 1;
            જો (msb < rWidth) પાછા newSlice(catp->rhsp(), lsb, width);
            જો (lsb >= rWidth) પાછા newSlice(catp->lhsp(), lsb - rWidth, width);
            પાછા નવું AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        જો (AstCond* અચલ condp = VN_CAST(nodep, Cond)) {
            પાછા નવું AstCond{flp, condp->condp()->clone_વૃક્ષ_Pure(ખોટું),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        જો (AstAnd* અચલ andp = VN_CAST(nodep, And)) {
            પાછા નવું AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        જો (AstOr* અચલ orp = VN_CAST(nodep, Or)) {
            પાછા નવું AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        જો (AstXor* અચલ xorp = VN_CAST(nodep, Xor)) {
            પાછા નવું AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        જો (AstExtend* અચલ extp = VN_CAST(nodep, Extend)) {
            અચલ પૂર્ણાંક srcWidth = extp->lhsp()->width();
            અચલ પૂર્ણાંક msb = lsb + width - 1;
            જો (msb < srcWidth) પાછા newSlice(extp->lhsp(), lsb, width);
            જો (lsb >= srcWidth) પાછા નવું AstConst{flp, AstConst::Widthed_કિંમત{}, width, 0};
            પાછા નવું AstConcat{
                flp, નવું AstConst{flp, AstConst::Widthed_કિંમત{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        જો (AstNot* અચલ notp = VN_CAST(nodep, Not)) {
            પાછા નવું AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        જો (AstReplicate* અચલ repp = VN_CAST(nodep, Replicate)) {
            Ast_ગાંઠ_પદાવલિ* અચલ srcp = repp->srcp();
            અચલ પૂર્ણાંક srcWidth = srcp->width();
            અચલ પૂર્ણાંક msb = lsb + width - 1;
            Ast_ગાંઠ_પદાવલિ* resultp = nullptr;
            માટે (પૂર્ણાંક copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                અચલ પૂર્ણાંક partLsb = std::max(lsb, copyLsb) - copyLsb;
                અચલ પૂર્ણાંક partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_ગાંઠ_પદાવલિ* અચલ bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? નવું AstConcat{flp, bitsp, resultp} : bitsp;
            }
            પાછા resultp;
        }
        Ast_ગાંઠ_પદાવલિ* અચલ clonep = nodep->clone_વૃક્ષ_Pure(ખોટું);
        જો (lsb == 0 && width == nodep->width()) પાછા clonep;
        પાછા નવું AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_ગાંઠ_પદાવલિ*> newAssignRhsps(Ast_ગાંઠ_પદાવલિ* nodep, Ast_ગાંઠ_DType* dtypep) {
        અચલ std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_ગાંઠ_પદાવલિ*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        જો (AstCReset* અચલ cresetp = VN_CAST(nodep, CReset)) {
            માટે (અચલ Component& comp : compsr) {
                AstCReset* અચલ newp = cresetp->clone_વૃક્ષ(ખોટું);
                newp->dtypep(comp.dtypep);
                valueps.ધકેલો_back(newp);
            }
            પાછા valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        જો (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            પૂર્ણાંક lsb = 0;  // LSB of the current term
            માટે (Ast_ગાંઠ_પદાવલિ* અચલ termp : concatTerms(nodep)) {
                અચલ પૂર્ણાંક msb = lsb + termp->width() - 1;
                અચલ size_t first = componentIndex(dtypep, lsb);
                માટે (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    અચલ Component& comp = compsr[i];
                    અચલ પૂર્ણાંક partLsb = std::max(lsb, comp.lsb);
                    અચલ પૂર્ણાંક partMsb = std::min(msb, comp.msb);
                    File_લીટી* અચલ flp = termp->fileline();
                    Ast_ગાંઠ_પદાવલિ* અચલ sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? નવું AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            પાછા valueps;
        }

        // Otherwise unpacked, so select each component
        માટે (size_t i = 0; i < compsr.size(); ++i) {
            valueps.ધકેલો_back(newSelect(nodep->clone_વૃક્ષ(ખોટું), dtypep, i));
        }
        પાછા valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_ગાંઠ_પદાવલિ* newBits(File_લીટી* flp, અચલ Place* placep, પૂર્ણાંક lsb, પૂર્ણાંક msb) {
        Ast_ગાંઠ_DType* અચલ dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        જો (!placep->split) {
            AstVarRef* અચલ refp = newRef(flp, placep, VAccess::READ);
            જો (lsb == 0 && msb == dtypep->width() - 1) પાછા refp;
            પાછા નવું AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        અચલ std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_ગાંઠ_પદાવલિ* resultp = nullptr;
        અચલ size_t first = componentIndex(dtypep, lsb);
        માટે (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            અચલ Component& comp = comps[i];
            અચલ પૂર્ણાંક partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            અચલ પૂર્ણાંક partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* અચલ childp = placep->childrenp.at(i).get();
            Ast_ગાંઠ_પદાવલિ* અચલ bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? નવું AstConcat{flp, bitsp, resultp} : bitsp;
        }
        પાછા resultp;
    }

    સ્થિર શૂન્ય addNewAssign(Ast_ગાંઠ_Assign* origp, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    શૂન્ય expandSS(Ast_ગાંઠ_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_લીટી* અચલ flp = origp->fileline();
        અચલ std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        જો (isPacked(rPlacep->dtypep)) {
            માટે (size_t i = 0; i < compsr.size(); ++i) {
                Place* અચલ lChildp = lPlacep->childrenp.at(i).get();
                Ast_ગાંઠ_પદાવલિ* અચલ rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } નહીંતર {
            માટે (size_t i = 0; i < compsr.size(); ++i) {
                Place* અચલ lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    શૂન્ય expandSN(Ast_ગાંઠ_Assign* origp, Place* lPlacep, Ast_ગાંઠ_પદાવલિ* rhsp) {
        અચલ std::vector<Ast_ગાંઠ_પદાવલિ*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        માટે (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(ધકેલો_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    શૂન્ય expandNS(Ast_ગાંઠ_Assign* origp, Ast_ગાંઠ_પદાવલિ* lhsp, Place* rPlacep) {
        File_લીટી* અચલ flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        જો (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            પાછા;
        }
        // Unpacked of the same shape, component-wise
        Ast_ગાંઠ_DType* અચલ dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        અચલ તાર્કિક readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            અચલ VNUser2InUse user2InUse;
            પાછા lhsp->exists([](અચલ AstVarRef* refp) {
                AstVarScope* અચલ vscp = refp->varScopep();
                જો (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                જો (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                પાછા vscp->user2() == 3;
            });
        }();
        જો (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* અચલ scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* અચલ vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, નવું AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, નવું AstVarRef{flp, vscp, VAccess::READ});
            પાછા;
        }
        અચલ size_t size = dtypeComponents(dtypep).size();
        માટે (size_t i = 0; i < size; ++i) {
            Ast_ગાંઠ_પદાવલિ* અચલ newLhsp = newSelect(lhsp->clone_વૃક્ષ(ખોટું), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(ધકેલો_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    શૂન્ય expand(Ast_ગાંઠ_Assign* origp,  //
                Place* lPlacep, Ast_ગાંઠ_પદાવલિ* lhsp,  //
                Place* rPlacep, Ast_ગાંઠ_પદાવલિ* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_લીટી* અચલ flp = origp->fileline();
        // A Place not split is referenced
        જો (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        જો (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        જો (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } નહીંતર જો (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } નહીંતર જો (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } નહીંતર {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    તાર્કિક isSplitBetween(અચલ Place* placep, પૂર્ણાંક lsb, પૂર્ણાંક msb) {
        જ્યારે (placep->split) {
            અચલ size_t idx = componentIndex(placep->dtypep, lsb);
            જો (idx != componentIndex(placep->dtypep, msb)) પાછા સાચું;
            અચલ Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        પાછા ખોટું;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    શૂન્ય hoistTerms(Ast_ગાંઠ_Assign* origp, અચલ Place* lPlacep) {
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = origp->rhsp();
        અચલ std::vector<Ast_ગાંઠ_પદાવલિ*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        અચલ VNUser2InUse user2InUse;
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* અચલ scopep = lPlacep->rootp->vscp->scopep();
        પૂર્ણાંક lsb = 0;  // LSB of the current term
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ termp : termps) {
            અચલ પૂર્ણાંક tLsb = lsb;
            અચલ પૂર્ણાંક tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            જો (!termp->user2()) ચાલુ;
            // Cheap to select from
            જો (isCheap(termp)) {
                termp->user2(0);
                ચાલુ;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            જો (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                ચાલુ;
            }
            // Bits can be selected without evaluating it more than once
            જો (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                ચાલુ;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_લીટી* અચલ flp = termp->fileline();
            AstVarScope* અચલ vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(નવું AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* અચલ tmpRefp = નવું AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(નવું AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    સ્થિર શૂન્ય markComponents(અચલ Place* placep) {
        માટે (અચલ std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(સાચું);
            જો (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    શૂન્ય rewriteAssignment(અચલ Assignment& assignment) {
        Place* અચલ lPlacep = assignment.lPlacep;
        Place* અચલ rPlacep = assignment.rPlacep;
        અચલ તાર્કિક lSplit = lPlacep && lPlacep->split;
        અચલ તાર્કિક rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        જો (!lSplit && !rSplit) પાછા;

        Ast_ગાંઠ_Assign* અચલ origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        જો (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            અચલ VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = origp->rhsp();
            જો (rhsp->exists([](અચલ AstVarRef* refp) { પાછા refp->varScopep()->user2(); })) {
                File_લીટી* અચલ flp = rhsp->fileline();
                AstScope* અચલ scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* અચલ vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(નવું AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* અચલ tmpRefp = નવું AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(નવું AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        જો (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = origp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        જો (lSplit) VL_DO_DANGLING(ધકેલો_Deletep(lhsp), lhsp);
        જો (rSplit) VL_DO_DANGLING(ધકેલો_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ધકેલો_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    સ્થિર AstActive* comboActive(AstScope* scopep) {
        જો (!scopep->user1p()) scopep->user1p(scopep->comboActivep(સાચું));
        પાછા VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    શૂન્ય driveFromComponents(Place* placep, AstActive* activep) {
        જો (!placep->split) પાછા;
        AstVarScope* અચલ vscp = placep->vscp;
        File_લીટી* અચલ flp = vscp->fileline();
        માટે (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* અચલ childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_ગાંઠ_પદાવલિ* અચલ lhsRefp = નવું AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(નવું AstAlways{નવું AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        માટે (અચલ std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        માટે (અચલ Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        માટે (Place* અચલ placep : m_state.rootps) {
            જો (!placep->split) ચાલુ;
            અચલ AstVarScope* અચલ vscp = placep->vscp;
            // Only traced ones at this point
            જો (!(vscp->varp()->isTrace() && vscp->isTrace())) ચાલુ;
            AstActive* અચલ activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

જાહેર:
    સ્થિર શૂન્ય apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

શૂન્ય V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("decompose", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
