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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Decompose.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3MemberMap.h"
#समावेश "V3SharedTmps.h"
#समावेश "V3Stats.h"

#समावेश <algorithm>
#समावेश <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

श्रेणी DecomposeBase VL_NOT_FINAL {
रक्षित:
    // TYPES

    // A component (element or member) of an aggregate type
    काष्ठा Component final {
        Ast_गाँठ_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        पूर्णांक declIdx;  // The declared index of the element, if an array
        पूर्णांक lsb;  // LSB in the whole value, if packed
        पूर्णांक msb;  // MSB in the whole value, if packed
    };

    काष्ठा Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    काष्ठा Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        पूर्णांक lsb;  // The first bit covered of this end
        पूर्णांक otherLsb;  // The first bit covered of the other end
        पूर्णांक width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    काष्ठा Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_गाँठ_DType* dtypep = nullptr;  // The data type of the variable or component
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
    काष्ठा Select final {
        Ast_गाँठ_व्यंजक* exprp;  // The select
        Place* placep;  // The component
        पूर्णांक lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    काष्ठा Assignment final {
        Ast_गाँठ_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

खुला:
    // The state of the pass, shared by the steps
    श्रेणी State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        स्थिर VNUser4InUse m_user4InUse;

    खुला:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_गाँठ_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

रक्षित:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    अटल bool isUnpacked(स्थिर Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        अगर (VN_IS(dtypep, UnpackArrayDType)) वापस true;
        स्थिर AstStructDType* स्थिर structp = VN_CAST(dtypep, StructDType);
        वापस structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    अटल bool isPacked(स्थिर Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        अगर (स्थिर AstPackArrayDType* स्थिर arrayp = VN_CAST(dtypep, PackArrayDType)) {
            वापस arrayp->subDTypep()->width() > 1;
        }
        स्थिर AstStructDType* स्थिर structp = VN_CAST(dtypep, StructDType);
        वापस structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    अटल bool isSplittableType(स्थिर Ast_गाँठ_DType* dtypep) {
        अगर (isPacked(dtypep)) वापस v3Global.opt.fDecomposePacked();
        अगर (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            अगर (स्थिर AstStructDType* स्थिर structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                क्रम (स्थिर AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    अगर (mp->valuep()) वापस false;
                }
            }
            वापस v3Global.opt.fDecomposeUnpacked();
        }
        वापस false;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    अटल Ast_गाँठ_DType* dtypeOf(स्थिर Ast_गाँठ_व्यंजक* nodep) {
        अगर (स्थिर AstVarRef* स्थिर refp = VN_CAST(nodep, VarRef)) वापस refp->varp()->dtypep();
        वापस nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    स्थिर std::vector<Component>& dtypeComponents(Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        अगर (compsr.empty()) {
            अगर (स्थिर Ast_गाँठ_Array_DType* स्थिर arrayp = VN_CAST(dtypep, गाँठ_Array_DType)) {
                स्थिर bool packed = isPacked(dtypep);
                स्थिर VNumRange range = arrayp->declRange();
                स्थिर bool rev = packed && range.ascending();  // Only for naming components
                Ast_गाँठ_DType* स्थिर subp = arrayp->subDTypep();
                क्रम (पूर्णांक i = 0; i < range.elements(); ++i) {
                    स्थिर पूर्णांक lsb = packed ? i * subp->width() : 0;
                    स्थिर पूर्णांक msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    स्थिर पूर्णांक declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.धकेलो_back({subp, nullptr, declIdx, lsb, msb});
                }
            } अन्यथा {
                स्थिर AstStructDType* स्थिर structp = VN_AS(dtypep, StructDType);
                स्थिर bool packed = isPacked(dtypep);
                क्रम (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    स्थिर पूर्णांक lsb = packed ? mp->lsb() : 0;
                    स्थिर पूर्णांक msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.धकेलो_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        वापस compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    माप_प्रकार component_सूचकांक(Ast_गाँठ_DType* dtypep, पूर्णांक bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        स्थिर std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        स्थिर स्वतः it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](स्थिर Component& comp, पूर्णांक b) {  //
                                             वापस b > comp.msb;
                                         });
        वापस it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, माप_प्रकार idx) {
        स्थिर std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        अगर (placep->childrenp.empty()) placep->childrenp.resize(comps.आकार());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        अगर (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        वापस childpr.लो();
    }

    // Is 'nodep' cheap to clone
    अटल bool isCheap(स्थिर Ast_गाँठ_व्यंजक* nodep) {
        // Constants are cheap
        अगर (VN_IS(nodep, Const)) वापस true;
        // So are resets
        अगर (VN_IS(nodep, CReset)) वापस true;
        // Variable references are cheap for the most part
        अगर (स्थिर AstVarRef* स्थिर refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            अगर (refp->varp()->isForced()) वापस false;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            अगर (refp->varp()->isSc()) वापस false;
            // Otherwise the path rooted here is cheap
            वापस true;
        }
        // Bit selects only with a constant LSB
        अगर (स्थिर AstSel* स्थिर selp = VN_CAST(nodep, Sel)) {
            वापस VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        अगर (स्थिर AstArraySel* स्थिर selp = VN_CAST(nodep, ArraySel)) {
            वापस isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        अगर (स्थिर AstStructSel* स्थिर selp = VN_CAST(nodep, StructSel)) {  //
            वापस isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        अगर (स्थिर AstExtend* स्थिर extp = VN_CAST(nodep, Extend)) {  //
            वापस isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        वापस false;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

श्रेणी DecomposeRecord final : खुला VNVisitorConst, खुला DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    स्थिर VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_गाँठ_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    अटल व्योम warnNoSplit(AstVar* varp, स्थिर Ast_गाँठ* wherep, स्थिर अक्षर* reasonp) {
        // Only warn if user requested splitting
        अगर (!varp->attrSplitVar()) वापस;
        // A packed variable might still be split by V3BitBlast, which warns if not
        अगर (isPacked(varp->dtypep())) वापस;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, true);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    अटल bool isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        अगर (!varp->user1()) {
            स्थिर bool eligible = [&]() {
                // Check data type
                अगर (!isSplittableType(varp->dtypep())) वापस false;
                // Check properties
                स्थिर अक्षर* स्थिर reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                अगर (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                वापस !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        वापस varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(स्थिर AstVarRef* refp) {
        AstVarScope* स्थिर vscp = refp->varScopep();
        अगर (!isEligible(vscp->varp())) वापस nullptr;
        Place& place = m_state.places(vscp);
        अगर (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.धकेलो_back(&place);
        }
        वापस &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    अटल व्योम block(Place* placep, स्थिर Ast_गाँठ* wherep) {
        placep->splittable = false;
        // Warn on the variable only, a component is split as far as possible, as requested
        अगर (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_गाँठ_व्यंजक* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        अगर (AstArraySel* स्थिर selp = VN_CAST(exprp, ArraySel)) {
            अगर (Place* स्थिर fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                स्थिर AstConst* स्थिर bitp = VN_CAST(selp->bitp(), Const);
                अगर (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).आकार()) {
                    Place* स्थिर childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = true;
                    chain.धकेलो_back({selp, childp, 0});
                    वापस childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            वापस nullptr;
        }
        // Struct selects correspond one to one with a component select
        अगर (AstStructSel* स्थिर selp = VN_CAST(exprp, StructSel)) {
            अगर (Place* स्थिर fromp = resolveChain(selp->fromp(), chain)) {
                अगर (!isUnpacked(fromp->dtypep)) वापस nullptr;
                AstStructDType* स्थिर structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                अगर (!structp->user1SetOnce()) {
                    स्थिर std::vector<Component>& compsr = dtypeComponents(structp);
                    क्रम (माप_प्रकार i = 0; i < compsr.आकार(); ++i) compsr[i].memberp->user1(i);
                }
                स्थिर Ast_गाँठ* स्थिर memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* स्थिर childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = true;
                chain.धकेलो_back({selp, childp, 0});
                वापस childp;
            }
            वापस nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        अगर (AstSel* स्थिर selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            अगर (!placep) {
                iterateConst(selp->lsbp());
                वापस nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            स्थिर AstConst* स्थिर lsbp = VN_CAST(selp->lsbp(), Const);
            अगर (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                वापस nullptr;
            }
            // Descend into the Place containing the bits, until exact
            पूर्णांक lsb = lsbp->toSInt();
            पूर्णांक msb = lsb + selp->widthConst() - 1;
            जबतक (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                अगर (!isPacked(placep->dtypep)) वापस nullptr;
                स्थिर माप_प्रकार idx = component_सूचकांक(placep->dtypep, lsb);
                // Crossing components prevents splitting
                अगर (idx != component_सूचकांक(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    वापस nullptr;
                }
                placep->wantsSplit = true;
                स्थिर Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.धकेलो_back({selp, placep, lsb});
            }
            वापस placep;
        }
        // Base case
        अगर (AstVarRef* स्थिर refp = VN_CAST(exprp, VarRef)) वापस placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        वापस nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    व्योम referencedWhole(Ast_गाँठ_व्यंजक* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        अगर (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            वापस;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    व्योम visitSelect(Ast_गाँठ_व्यंजक* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* स्थिर placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        अगर (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        अगर (!chain.empty()) m_state.chains.धकेलो_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    अटल bool hasReversedRange(स्थिर Ast_गाँठ_DType* aDTypep, स्थिर Ast_गाँठ_DType* bDTypep) {
        स्थिर AstUnpackArrayDType* स्थिर aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        स्थिर AstUnpackArrayDType* स्थिर bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        अगर (!aArrayp || !bArrayp) वापस false;
        अगर (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) वापस true;
        वापस hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    व्योम visitAssignment(Ast_गाँठ_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        स्थिर bool reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        अगर (!m_assignp) वापस;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp();
        Place* स्थिर lPlacep = lhsp->user1u().to<Place*>();
        Place* स्थिर rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        स्थिर bool splittable = [&]() {
            // Assignment between two Places is splittable
            अगर (lPlacep && rPlacep) वापस true;
            // If the LHS is a Place, the RHS must be a splittable expression
            अगर (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                अगर (isPacked(lPlacep->dtypep)) वापस true;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                वापस isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            अगर (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                अगर (isPacked(rPlacep->dtypep)) वापस true;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                वापस isCheap(lhsp);
            }
            // Otherwise not splittable
            वापस false;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        अगर (!splittable) {
            अगर (rPlacep) block(rPlacep, rhsp);
            अगर (lPlacep) block(lPlacep, lhsp);
            वापस;
        }

        // If both sides are places, record the whole copies for the Decision phase
        अगर (lPlacep && rPlacep) {
            स्थिर पूर्णांक width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.धकेलो_back({rPlacep, 0, 0, width});
            rPlacep->copies.धकेलो_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.धकेलो_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_DType*) override {}  // No references in data types
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    व्योम visit(AstArraySel* nodep) override { visitSelect(nodep); }
    व्योम visit(AstStructSel* nodep) override { visitSelect(nodep); }
    व्योम visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    व्योम visit(AstAssign* nodep) override { visitAssignment(nodep); }
    व्योम visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    व्योम visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    व्योम visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        अगर (Place* स्थिर placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    व्योम visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* स्थिर varp = nodep->varp();
        अगर (!isEligible(varp)) वापस;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        क्रम (Place* स्थिर placep : m_state.rootps) {
            अगर (!isEligible(placep->vscp->varp())) placep->splittable = false;
        }
    }

खुला:
    अटल व्योम apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

श्रेणी DecomposeDecision final : खुला DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    स्थिर VNUser1InUse m_user1InUse;

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
    अटल bool canSplit(स्थिर Place* placep) {
        // Not if not splittable
        अगर (!placep->splittable) वापस false;
        // Not if already split
        अगर (placep->split) वापस false;
        // Original variable or its parent was already split
        वापस !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    व्योम enqueue(Place* placep) {
        अगर (placep->queued || !placep->wantsSplit || !canSplit(placep)) वापस;
        placep->queued = true;
        m_worklist.धकेलो_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    व्योम createComponents(Place* placep) {
        AstVarScope* स्थिर vscp = placep->vscp;
        AstVar* स्थिर varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        अगर (varps.empty()) {
            स्थिर bool packed = isPacked(varp->dtypep());
            अगर (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } अन्यथा {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_पंक्ति* स्थिर flp = varp->fileline();
            स्थिर VVarType varType = varp->varType();
            स्थिर std::string name = varp->name();
            क्रम (स्थिर Component& comp : dtypeComponents(varp->dtypep())) {
                स्थिर std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_गाँठ::encode_संख्या(comp.declIdx) + "__KET__";
                AstVar* स्थिर newp = नया AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.धकेलो_back(newp);
                varp->add_Here_This_As_अग्र(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* स्थिर scopep = vscp->scopep();
        File_पंक्ति* स्थिर flp = vscp->fileline();
        क्रम (माप_प्रकार i = 0; i < varps.आकार(); ++i) {
            AstVarScope* स्थिर newp = नया AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->add_Here_This_As_अग्र(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    व्योम addCopy(Place* placep, स्थिर Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        अगर (!placep->splittable) वापस;
        // Add the copy to the Place
        placep->copies.धकेलो_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        अगर (!isPacked(placep->dtypep)) वापस;
        अगर (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = true;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    व्योम newCopy(Place* ap, पूर्णांक aLsb, Place* bp, पूर्णांक bLsb, पूर्णांक width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    व्योम splitCopy(Place* placep, स्थिर Copy& copy) {
        Place* स्थिर otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        अगर (isPacked(placep->dtypep)) {
            स्थिर std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            स्थिर पूर्णांक lsb = copy.lsb;
            स्थिर पूर्णांक msb = copy.lsb + copy.width - 1;
            स्थिर माप_प्रकार first = component_सूचकांक(placep->dtypep, lsb);
            क्रम (माप_प्रकार i = first; i < comps.आकार() && comps[i].lsb <= msb; ++i) {
                स्थिर Component& comp = comps[i];
                स्थिर पूर्णांक partLsb = std::max(lsb, comp.lsb);
                स्थिर पूर्णांक partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            वापस;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = true;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        स्थिर माप_प्रकार आकार = dtypeComponents(placep->dtypep).आकार();
        क्रम (माप_प्रकार i = 0; i < आकार; ++i) {
            Place* स्थिर ap = componentOf(placep, i);
            अगर (!ap->splittable) जारी;
            Place* स्थिर bp = componentOf(otherp, i);
            अगर (!bp->splittable) जारी;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    व्योम splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = true;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        स्थिर माप_प्रकार आकार = dtypeComponents(placep->dtypep).आकार();
        क्रम (माप_प्रकार i = 0; i < आकार; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        क्रम (स्थिर Copy& copy : placep->copies) {
            अगर (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.सफा();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        क्रम (Place* स्थिर placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        जबतक (!m_worklist.empty()) {
            Place* स्थिर placep = m_worklist.back();
            m_worklist.निकालो_back();
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

खुला:
    अटल व्योम apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

श्रेणी DecomposeRewrite final : खुला VNDeleter, खुला DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    स्थिर VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    अटल AstVarRef* newRef(File_पंक्ति* flp, स्थिर Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        वापस नया AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    व्योम rewriteChain(स्थिर std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        स्थिर Select* selectp = nullptr;
        क्रम (स्थिर Select& select : chain) {
            अगर (select.placep->parentp->split) selectp = &select;
        }
        अगर (!selectp) वापस;
        Ast_गाँठ_व्यंजक* स्थिर exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        स्थिर VAccess access = [&]() {
            स्थिर Ast_गाँठ_व्यंजक* rootp = chain.front().exprp;
            जबतक (true) {
                अगर (स्थिर AstArraySel* स्थिर selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } अन्यथा अगर (स्थिर AstStructSel* स्थिर selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } अन्यथा अगर (स्थिर AstSel* स्थिर selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } अन्यथा {
                    वापस VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_पंक्ति* स्थिर flp = exprp->fileline();
        Ast_गाँठ_व्यंजक* newp = newRef(flp, selectp->placep, access);
        अगर (स्थिर AstSel* स्थिर selp = VN_CAST(exprp, Sel)) {
            स्थिर पूर्णांक width = selp->widthConst();
            अगर (width != newp->width()) newp = नया AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_गाँठ_व्यंजक* newSelect(Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_DType* dtypep, माप_प्रकार idx) {
        File_पंक्ति* स्थिर flp = fromp->fileline();
        स्थिर Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        अगर (isPacked(dtypep)) {
            AstSel* स्थिर selp = नया AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            वापस selp;
        }
        // If unpacked array, it's an ArraySel
        अगर (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            स्थिर पूर्णांक i = static_cast<पूर्णांक>(idx);
            UASSERT_OBJ(static_cast<माप_प्रकार>(i) == idx, fromp, "Index overflow");
            वापस नया AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* स्थिर selp = नया AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        वापस selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    अटल std::vector<Ast_गाँठ_व्यंजक*> concatTerms(Ast_गाँठ_व्यंजक* nodep) {
        std::vector<Ast_गाँठ_व्यंजक*> termps;
        std::vector<Ast_गाँठ_व्यंजक*> ढेर{nodep};
        जबतक (!ढेर.empty()) {
            Ast_गाँठ_व्यंजक* स्थिर exprp = ढेर.back();
            ढेर.निकालो_back();
            अगर (AstConcat* स्थिर concatp = VN_CAST(exprp, Concat)) {
                ढेर.धकेलो_back(concatp->lhsp());
                ढेर.धकेलो_back(concatp->rhsp());
                जारी;
            }
            अगर (AstReplicate* स्थिर repp = VN_CAST(exprp, Replicate)) {
                स्थिर AstConst* स्थिर countp = VN_AS(repp->countp(), Const);
                क्रम (uint32_t i = 0; i < countp->toUInt(); ++i) ढेर.धकेलो_back(repp->srcp());
                जारी;
            }
            termps.धकेलो_back(exprp);
        }
        वापस termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    अटल bool isSliceable(स्थिर Ast_गाँठ_व्यंजक* nodep) {
        अगर (स्थिर AstSel* स्थिर selp = VN_CAST(nodep, Sel)) {
            वापस VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        अगर (स्थिर AstConcat* स्थिर catp = VN_CAST(nodep, Concat)) {
            वापस isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        अगर (स्थिर AstCond* स्थिर condp = VN_CAST(nodep, Cond)) {
            वापस isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        अगर (स्थिर AstAnd* स्थिर andp = VN_CAST(nodep, And)) {
            वापस isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        अगर (स्थिर AstOr* स्थिर orp = VN_CAST(nodep, Or)) {
            वापस isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        अगर (स्थिर AstXor* स्थिर xorp = VN_CAST(nodep, Xor)) {
            वापस isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        अगर (स्थिर AstExtend* स्थिर extp = VN_CAST(nodep, Extend)) {
            वापस isSliceable(extp->lhsp());
        }
        अगर (स्थिर AstNot* स्थिर notp = VN_CAST(nodep, Not)) {  //
            वापस isSliceable(notp->lhsp());
        }
        अगर (स्थिर AstReplicate* स्थिर repp = VN_CAST(nodep, Replicate)) {
            वापस isSliceable(repp->srcp());
        }
        वापस isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    अटल Ast_गाँठ_व्यंजक* newSlice(Ast_गाँठ_व्यंजक* nodep, पूर्णांक lsb, पूर्णांक width) {
        File_पंक्ति* स्थिर flp = nodep->fileline();
        // A constant, as a new constant of the bits
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep, Const)) {
            V3_संख्या अंक{nodep, width};
            अंक.opSel(constp->अंक(), lsb + width - 1, lsb);
            वापस नया AstConst{flp, अंक};
        }
        // A constant bit select, from the bits of what it selects from
        अगर (AstSel* स्थिर selp = VN_CAST(nodep, Sel)) {
            अगर (स्थिर AstConst* स्थिर lsbp = VN_CAST(selp->lsbp(), Const)) {
                वापस newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        अगर (AstConcat* स्थिर catp = VN_CAST(nodep, Concat)) {
            स्थिर पूर्णांक rWidth = catp->rhsp()->width();
            स्थिर पूर्णांक msb = lsb + width - 1;
            अगर (msb < rWidth) वापस newSlice(catp->rhsp(), lsb, width);
            अगर (lsb >= rWidth) वापस newSlice(catp->lhsp(), lsb - rWidth, width);
            वापस नया AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        अगर (AstCond* स्थिर condp = VN_CAST(nodep, Cond)) {
            वापस नया AstCond{flp, condp->condp()->clone_वृक्ष_Pure(false),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        अगर (AstAnd* स्थिर andp = VN_CAST(nodep, And)) {
            वापस नया AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        अगर (AstOr* स्थिर orp = VN_CAST(nodep, Or)) {
            वापस नया AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        अगर (AstXor* स्थिर xorp = VN_CAST(nodep, Xor)) {
            वापस नया AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        अगर (AstExtend* स्थिर extp = VN_CAST(nodep, Extend)) {
            स्थिर पूर्णांक srcWidth = extp->lhsp()->width();
            स्थिर पूर्णांक msb = lsb + width - 1;
            अगर (msb < srcWidth) वापस newSlice(extp->lhsp(), lsb, width);
            अगर (lsb >= srcWidth) वापस नया AstConst{flp, AstConst::Widthed_मूल्य{}, width, 0};
            वापस नया AstConcat{
                flp, नया AstConst{flp, AstConst::Widthed_मूल्य{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        अगर (AstNot* स्थिर notp = VN_CAST(nodep, Not)) {
            वापस नया AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        अगर (AstReplicate* स्थिर repp = VN_CAST(nodep, Replicate)) {
            Ast_गाँठ_व्यंजक* स्थिर srcp = repp->srcp();
            स्थिर पूर्णांक srcWidth = srcp->width();
            स्थिर पूर्णांक msb = lsb + width - 1;
            Ast_गाँठ_व्यंजक* resultp = nullptr;
            क्रम (पूर्णांक copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                स्थिर पूर्णांक partLsb = std::max(lsb, copyLsb) - copyLsb;
                स्थिर पूर्णांक partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_गाँठ_व्यंजक* स्थिर bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? नया AstConcat{flp, bitsp, resultp} : bitsp;
            }
            वापस resultp;
        }
        Ast_गाँठ_व्यंजक* स्थिर clonep = nodep->clone_वृक्ष_Pure(false);
        अगर (lsb == 0 && width == nodep->width()) वापस clonep;
        वापस नया AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_गाँठ_व्यंजक*> newAssignRhsps(Ast_गाँठ_व्यंजक* nodep, Ast_गाँठ_DType* dtypep) {
        स्थिर std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_गाँठ_व्यंजक*> valueps;
        valueps.reserve(compsr.आकार());

        // If CReset, it is duplicated for each component
        अगर (AstCReset* स्थिर cresetp = VN_CAST(nodep, CReset)) {
            क्रम (स्थिर Component& comp : compsr) {
                AstCReset* स्थिर newp = cresetp->clone_वृक्ष(false);
                newp->dtypep(comp.dtypep);
                valueps.धकेलो_back(newp);
            }
            वापस valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        अगर (isPacked(dtypep)) {
            valueps.resize(compsr.आकार(), nullptr);
            पूर्णांक lsb = 0;  // LSB of the current term
            क्रम (Ast_गाँठ_व्यंजक* स्थिर termp : concatTerms(nodep)) {
                स्थिर पूर्णांक msb = lsb + termp->width() - 1;
                स्थिर माप_प्रकार first = component_सूचकांक(dtypep, lsb);
                क्रम (माप_प्रकार i = first; i < compsr.आकार() && compsr[i].lsb <= msb; ++i) {
                    स्थिर Component& comp = compsr[i];
                    स्थिर पूर्णांक partLsb = std::max(lsb, comp.lsb);
                    स्थिर पूर्णांक partMsb = std::min(msb, comp.msb);
                    File_पंक्ति* स्थिर flp = termp->fileline();
                    Ast_गाँठ_व्यंजक* स्थिर sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? नया AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            वापस valueps;
        }

        // Otherwise unpacked, so select each component
        क्रम (माप_प्रकार i = 0; i < compsr.आकार(); ++i) {
            valueps.धकेलो_back(newSelect(nodep->clone_वृक्ष(false), dtypep, i));
        }
        वापस valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_गाँठ_व्यंजक* newBits(File_पंक्ति* flp, स्थिर Place* placep, पूर्णांक lsb, पूर्णांक msb) {
        Ast_गाँठ_DType* स्थिर dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        अगर (!placep->split) {
            AstVarRef* स्थिर refp = newRef(flp, placep, VAccess::READ);
            अगर (lsb == 0 && msb == dtypep->width() - 1) वापस refp;
            वापस नया AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        स्थिर std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_गाँठ_व्यंजक* resultp = nullptr;
        स्थिर माप_प्रकार first = component_सूचकांक(dtypep, lsb);
        क्रम (माप_प्रकार i = first; i < comps.आकार() && comps[i].lsb <= msb; ++i) {
            स्थिर Component& comp = comps[i];
            स्थिर पूर्णांक partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            स्थिर पूर्णांक partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* स्थिर childp = placep->childrenp.at(i).लो();
            Ast_गाँठ_व्यंजक* स्थिर bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? नया AstConcat{flp, bitsp, resultp} : bitsp;
        }
        वापस resultp;
    }

    अटल व्योम addNewAssign(Ast_गाँठ_Assign* origp, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp) {
        origp->add_Here_This_As_अग्र(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    व्योम expandSS(Ast_गाँठ_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_पंक्ति* स्थिर flp = origp->fileline();
        स्थिर std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        अगर (isPacked(rPlacep->dtypep)) {
            क्रम (माप_प्रकार i = 0; i < compsr.आकार(); ++i) {
                Place* स्थिर lChildp = lPlacep->childrenp.at(i).लो();
                Ast_गाँठ_व्यंजक* स्थिर rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } अन्यथा {
            क्रम (माप_प्रकार i = 0; i < compsr.आकार(); ++i) {
                Place* स्थिर lChildp = lPlacep->childrenp.at(i).लो();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).लो(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    व्योम expandSN(Ast_गाँठ_Assign* origp, Place* lPlacep, Ast_गाँठ_व्यंजक* rhsp) {
        स्थिर std::vector<Ast_गाँठ_व्यंजक*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        क्रम (माप_प्रकार i = 0; i < valueps.आकार(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).लो(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(धकेलो_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    व्योम expandNS(Ast_गाँठ_Assign* origp, Ast_गाँठ_व्यंजक* lhsp, Place* rPlacep) {
        File_पंक्ति* स्थिर flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        अगर (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            वापस;
        }
        // Unpacked of the same shape, component-wise
        Ast_गाँठ_DType* स्थिर dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        स्थिर bool readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            स्थिर VNUser2InUse user2InUse;
            वापस lhsp->exists([](स्थिर AstVarRef* refp) {
                AstVarScope* स्थिर vscp = refp->varScopep();
                अगर (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                अगर (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                वापस vscp->user2() == 3;
            });
        }();
        अगर (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* स्थिर scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* स्थिर vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, नया AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, नया AstVarRef{flp, vscp, VAccess::READ});
            वापस;
        }
        स्थिर माप_प्रकार आकार = dtypeComponents(dtypep).आकार();
        क्रम (माप_प्रकार i = 0; i < आकार; ++i) {
            Ast_गाँठ_व्यंजक* स्थिर newLhsp = newSelect(lhsp->clone_वृक्ष(false), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).लो(), nullptr);
        }
        VL_DO_DANGLING(धकेलो_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    व्योम expand(Ast_गाँठ_Assign* origp,  //
                Place* lPlacep, Ast_गाँठ_व्यंजक* lhsp,  //
                Place* rPlacep, Ast_गाँठ_व्यंजक* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_पंक्ति* स्थिर flp = origp->fileline();
        // A Place not split is referenced
        अगर (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        अगर (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        अगर (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } अन्यथा अगर (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } अन्यथा अगर (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } अन्यथा {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    bool isSplitBetween(स्थिर Place* placep, पूर्णांक lsb, पूर्णांक msb) {
        जबतक (placep->split) {
            स्थिर माप_प्रकार idx = component_सूचकांक(placep->dtypep, lsb);
            अगर (idx != component_सूचकांक(placep->dtypep, msb)) वापस true;
            स्थिर Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).लो();
        }
        वापस false;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    व्योम hoistTerms(Ast_गाँठ_Assign* origp, स्थिर Place* lPlacep) {
        Ast_गाँठ_व्यंजक* स्थिर rhsp = origp->rhsp();
        स्थिर std::vector<Ast_गाँठ_व्यंजक*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        स्थिर VNUser2InUse user2InUse;
        क्रम (Ast_गाँठ_व्यंजक* स्थिर termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* स्थिर scopep = lPlacep->rootp->vscp->scopep();
        पूर्णांक lsb = 0;  // LSB of the current term
        क्रम (Ast_गाँठ_व्यंजक* स्थिर termp : termps) {
            स्थिर पूर्णांक tLsb = lsb;
            स्थिर पूर्णांक tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            अगर (!termp->user2()) जारी;
            // Cheap to select from
            अगर (isCheap(termp)) {
                termp->user2(0);
                जारी;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            अगर (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                जारी;
            }
            // Bits can be selected without evaluating it more than once
            अगर (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                जारी;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_पंक्ति* स्थिर flp = termp->fileline();
            AstVarScope* स्थिर vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(नया AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* स्थिर tmpRefp = नया AstVarRef{flp, vscp, VAccess::WRITE};
            origp->add_Here_This_As_अग्र(नया AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    अटल व्योम markComponents(स्थिर Place* placep) {
        क्रम (स्थिर std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(true);
            अगर (childp->split) markComponents(childp.लो());
        }
    }

    // Expand the assignment, if a side is split
    व्योम rewriteAssignment(स्थिर Assignment& assignment) {
        Place* स्थिर lPlacep = assignment.lPlacep;
        Place* स्थिर rPlacep = assignment.rPlacep;
        स्थिर bool lSplit = lPlacep && lPlacep->split;
        स्थिर bool rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        अगर (!lSplit && !rSplit) वापस;

        Ast_गाँठ_Assign* स्थिर origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        अगर (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            स्थिर VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_गाँठ_व्यंजक* स्थिर rhsp = origp->rhsp();
            अगर (rhsp->exists([](स्थिर AstVarRef* refp) { वापस refp->varScopep()->user2(); })) {
                File_पंक्ति* स्थिर flp = rhsp->fileline();
                AstScope* स्थिर scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* स्थिर vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(नया AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* स्थिर tmpRefp = नया AstVarRef{flp, vscp, VAccess::WRITE};
                origp->add_Here_This_As_अग्र(नया AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        अगर (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_गाँठ_व्यंजक* स्थिर lhsp = origp->lhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        अगर (lSplit) VL_DO_DANGLING(धकेलो_Deletep(lhsp), lhsp);
        अगर (rSplit) VL_DO_DANGLING(धकेलो_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(धकेलो_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    अटल AstActive* comboActive(AstScope* scopep) {
        अगर (!scopep->user1p()) scopep->user1p(scopep->comboActivep(true));
        वापस VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    व्योम driveFromComponents(Place* placep, AstActive* activep) {
        अगर (!placep->split) वापस;
        AstVarScope* स्थिर vscp = placep->vscp;
        File_पंक्ति* स्थिर flp = vscp->fileline();
        क्रम (माप_प्रकार i = 0; i < placep->childrenp.आकार(); ++i) {
            Place* स्थिर childp = placep->childrenp[i].लो();
            driveFromComponents(childp, activep);
            Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_गाँठ_व्यंजक* स्थिर lhsRefp = नया AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_गाँठ_व्यंजक* स्थिर lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(नया AstAlways{नया AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        क्रम (स्थिर std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        क्रम (स्थिर Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        क्रम (Place* स्थिर placep : m_state.rootps) {
            अगर (!placep->split) जारी;
            स्थिर AstVarScope* स्थिर vscp = placep->vscp;
            // Only traced ones at this point
            अगर (!(vscp->varp()->isTrace() && vscp->isTrace())) जारी;
            AstActive* स्थिर activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

खुला:
    अटल व्योम apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

व्योम V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("decompose", 0, dump_वृक्ष_Either_Level() >= 3);
}
