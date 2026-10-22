// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode hash computation
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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3Hasher.h"

#அந்தர்பாவ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

வகுப்பு HasherVisitor final : பொது VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    மாறிலி தருக்க m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_முனை*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    வெறுமை guardRecursion(Ast_முனை* மாறிலி nodep, std::function<வெறுமை()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        எனில் (!m_cacheInUser4) {
            எனில் (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                திரும்பு;
            }
            m_visited.செருகு(nodep);
        }
        f();
    }

    V3Hash hash_முனை_And_Iterate(Ast_முனை* nodep, தருக்க hashDType, தருக்க hashChildren,
                              std::function<வெறுமை()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        எனில் (m_cacheInUser4 && nodep->user4()) {
            திரும்பு V3Hash{static_cast<uint32_t>(nodep->user4())};
        } இல்லையெனில் {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            எனில் (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            எனில் (hashChildren) iterateChildrenConst(nodep);  // Children
            எனில் (m_cacheInUser4) nodep->user4(m_hash.மதிப்பு());
            திரும்பு m_hash;
        }
    }

    // VISITORS

    constexpr நிலையான தருக்க HASH_DTYPE = உண்மை;
    constexpr நிலையான தருக்க HASH_CHILDREN = உண்மை;

    // Each visitor below contributes to the hash any node-specific content
    // that is not dependent on either of the following, as these are
    // included by default by hashNode:
    // - Node type (as given by AstNode::type().hash())
    // - Node dtype (unless !hashDType)
    // - child nodes (unless !hashChildren)
    //
    // The hash must be stable, which means in particular it cannot rely on
    // pointer values, or any other value that might differ between separate
    // invocations of Verilator over the same design.
    //
    // Note there is a circularity problem where some child nodes can back
    // to their ancestral nodes via member pointers, which can lead to an
    // infinite traversal. To break this, nodes that are subject to such
    // referencing and represent code which can reasonably be assumed not to
    // be equivalent to any other code, are hashed either by name (e.g.:
    // AstNodeModule), or by unique identifier (e.g.: AstNodeUOrStructDType).

    //------------------------------------------------------------
    // AstNode - Warns to help find missing cases

    வெறுமை visit(Ast_முனை* nodep) override {
#எனில் VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#endif
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    வெறுமை visit(Ast_முனை_Array_DType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->இடது();
            m_hash += nodep->வலது();
        });
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், பொய், [இது, nodep]() {  //
            m_hash += nodep->unique_இலக்கம்();
        });
    }
    வெறுமை visit(AstParamTypeDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    வெறுமை visit(AstMemberDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->unique_இலக்கம்();
        });
    }
    வெறுமை visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    வெறுமை visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    வெறுமை visit(AstDynArrayDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    வெறுமை visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    வெறுமை visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    வெறுமை visit(AstBasicDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().இடது();
            m_hash += nodep->nrange().வலது();
        });
    }
    வெறுமை visit(AstCDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstConstDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    வெறுமை visit(AstClassRefDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    வெறுமை visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    வெறுமை visit(AstQueueDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    வெறுமை visit(AstRefDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    வெறுமை visit(AstStreamDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstVoidDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstEnumDType* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், பொய், [இது, nodep]() {  //
            m_hash += nodep->unique_இலக்கம்();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    வெறுமை visit(Ast_முனை_கோவை* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstSel* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    வெறுமை visit(AstConst* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->இலக்கம்().toHash();
        });
    }
    வெறுமை visit(AstNullCheck* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstCCast* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->size();
        });
    }
    வெறுமை visit(AstVarRef* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {
            எனில் (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } இல்லையெனில் {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    வெறுமை visit(AstVarXRef* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstFScanF* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    வெறுமை visit(AstSScanF* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    வெறுமை visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    வெறுமை visit(AstTriggered* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            க்கு (மாறிலி uint32_t index : nodep->indices()) m_hash += index;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, []() {});
    }
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    வெறுமை visit(AstCaseItem* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, []() {});
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    வெறுமை visit(AstCMethodHard* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    வெறுமை visit(AstCAwait* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    வெறுமை visit(AstCLocalScope* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstCoverInc* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    வெறுமை visit(AstDisplay* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    வெறுமை visit(AstMonitorOff* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    வெறுமை visit(AstJumpGo* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், பொய், []() {});
    }
    வெறுமை visit(AstTraceInc* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    வெறுமை visit(Ast_முனை_Cover_Or_Assert* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    வெறுமை visit(Ast_முனை_Range* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, பொய், [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(Ast_முனை_Pre_Sel* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstClassExtends* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstForeachHeader* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstDefParam* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstArg* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstParseRef* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    வெறுமை visit(AstSenItem* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    வெறுமை visit(Ast_Sen_மரம்* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstSFormatF* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    வெறுமை visit(AstElabDisplay* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    வெறுமை visit(Ast_முனை_Cons_Item* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        மாறிலி AstUnpackArrayDType* மாறிலி dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ பொய்,
                                     [இது, nodep, dtypep]() {
                                         மாறிலி uint32_t size = dtypep->elementsConst();
                                         க்கு (uint32_t n = 0; n < size; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    வெறுமை visit(AstPragma* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    வெறுமை visit(AstAttrOf* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    வெறுமை visit(Ast_முனை_File* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [இது, nodep]() {  //
            m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                எனில் (m_cacheInUser4) nodep->user4(V3Hash{1}.மதிப்பு());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    வெறுமை visit(AstVar* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    வெறுமை visit(AstScope* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, பொய், [இது, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    வெறுமை visit(AstVarScope* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    வெறுமை visit(AstEnumItem* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstTypedef* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstTypedefFwd* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstActive* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    வெறுமை visit(AstCell* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    வெறுமை visit(AstCellInline* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [இது, nodep]() { m_hash += nodep->name(); });
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        guardRecursion(nodep, [இது, nodep]() {  //
            m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                எனில் (m_cacheInUser4) nodep->user4(V3Hash{1}.மதிப்பு());
            });
        });
    }
    வெறுமை visit(AstModport* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstModportVarRef* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    வெறுமை visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstBegin* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    வெறுமை visit(AstFork* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    வெறுமை visit(AstPin* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [இது, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_இலக்கம்();
        });
    }
    வெறுமை visit(AstText* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    வெறுமை visit(AstTextBlock* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, [இது, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    வெறுமை visit(AstCStmt* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstCStmtUser* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, பொய், HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstCExpr* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstCExprUser* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    வெறுமை visit(AstWith* nodep) override {
        m_hash += hash_முனை_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

பொது:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_முனை* nodep)
        : m_cacheInUser4{உண்மை} {
        iterateConst(nodep);
    }
    வகுப்பு Uncached {};
    HasherVisitor(மாறிலி Ast_முனை* nodep, Uncached)
        : m_cacheInUser4{பொய்} {
        iterateConst(const_cast<Ast_முனை*>(nodep));
    }
    V3Hash finalHash() மாறிலி { திரும்பு m_hash; }
    ~HasherVisitor() override = இயல்பு;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::operator()(Ast_முனை* nodep) மாறிலி {
    எனில் (!nodep->user4()) HasherVisitor{nodep};
    திரும்பு V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_முனை* nodep) மாறிலி {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    திரும்பு V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(மாறிலி Ast_முனை* nodep) {
    மாறிலி HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    திரும்பு visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

size_t V3HasherUncachedHash(மாறிலி Ast_முனை& முனை) {
    திரும்பு static_cast<size_t>(V3Hasher::uncachedHash(&முனை).மதிப்பு());
}
