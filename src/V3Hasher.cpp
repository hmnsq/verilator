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

#समावेश "V3PchAstMT.h"

#समावेश "V3Hasher.h"

#समावेश <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

श्रेणी HasherVisitor final : खुला VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    स्थिर bool m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_गाँठ*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    व्योम guardRecursion(Ast_गाँठ* स्थिर nodep, std::function<व्योम()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        अगर (!m_cacheInUser4) {
            अगर (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                वापस;
            }
            m_visited.जोड़ो(nodep);
        }
        f();
    }

    V3Hash hash_गाँठ_And_Iterate(Ast_गाँठ* nodep, bool hashDType, bool hashChildren,
                              std::function<व्योम()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        अगर (m_cacheInUser4 && nodep->user4()) {
            वापस V3Hash{static_cast<uint32_t>(nodep->user4())};
        } अन्यथा {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            अगर (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            अगर (hashChildren) iterateChildrenConst(nodep);  // Children
            अगर (m_cacheInUser4) nodep->user4(m_hash.मूल्य());
            वापस m_hash;
        }
    }

    // VISITORS

    constexpr अटल bool HASH_DTYPE = true;
    constexpr अटल bool HASH_CHILDREN = true;

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

    व्योम visit(Ast_गाँठ* nodep) override {
#अगर VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#पूर्ण_अगर
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    व्योम visit(Ast_गाँठ_Array_DType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->बाँए();
            m_hash += nodep->दाँए();
        });
    }
    व्योम visit(Ast_गाँठ_UOr_Struct_DType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, false, [यह, nodep]() {  //
            m_hash += nodep->unique_अंक();
        });
    }
    व्योम visit(AstParamTypeDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    व्योम visit(AstMemberDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->unique_अंक();
        });
    }
    व्योम visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    व्योम visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    व्योम visit(AstDynArrayDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    व्योम visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    व्योम visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    व्योम visit(AstBasicDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().बाँए();
            m_hash += nodep->nrange().दाँए();
        });
    }
    व्योम visit(AstCDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstConstDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    व्योम visit(AstClassRefDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    व्योम visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    व्योम visit(AstQueueDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    व्योम visit(AstRefDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    व्योम visit(AstStreamDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstVoidDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstEnumDType* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, false, [यह, nodep]() {  //
            m_hash += nodep->unique_अंक();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    व्योम visit(Ast_गाँठ_व्यंजक* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstSel* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    व्योम visit(AstConst* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->अंक().toHash();
        });
    }
    व्योम visit(AstNullCheck* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstCCast* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->आकार();
        });
    }
    व्योम visit(AstVarRef* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {
            अगर (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } अन्यथा {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    व्योम visit(AstVarXRef* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    व्योम visit(AstMemberSel* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstFScanF* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    व्योम visit(AstSScanF* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    व्योम visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    व्योम visit(AstTriggered* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            क्रम (स्थिर uint32_t सूचकांक_2 : nodep->indices()) m_hash += सूचकांक_2;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    व्योम visit(Ast_गाँठ_Stmt* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, []() {});
    }
    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    व्योम visit(AstCaseItem* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, []() {});
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    व्योम visit(AstCMethodHard* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    व्योम visit(AstCAwait* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    व्योम visit(AstCLocalScope* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstCoverInc* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    व्योम visit(AstDisplay* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    व्योम visit(AstMonitorOff* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    व्योम visit(AstJumpGo* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, false, []() {});
    }
    व्योम visit(AstTraceInc* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    व्योम visit(Ast_गाँठ_Cover_Or_Assert* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    व्योम visit(Ast_गाँठ_Range* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, false, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(Ast_गाँठ_Pre_Sel* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstClassExtends* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstForeachHeader* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstDefParam* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstArg* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstParseRef* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    व्योम visit(AstSenItem* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    व्योम visit(Ast_Sen_वृक्ष* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstSFormatF* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    व्योम visit(AstElabDisplay* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    व्योम visit(Ast_गाँठ_Cons_Item* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        स्थिर AstUnpackArrayDType* स्थिर dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ false,
                                     [यह, nodep, dtypep]() {
                                         स्थिर uint32_t आकार = dtypep->elementsConst();
                                         क्रम (uint32_t n = 0; n < आकार; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    व्योम visit(AstPragma* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    व्योम visit(AstAttrOf* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    व्योम visit(Ast_गाँठ_File* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [यह, nodep]() {  //
            m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                अगर (m_cacheInUser4) nodep->user4(V3Hash{1}.मूल्य());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    व्योम visit(AstVar* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    व्योम visit(AstScope* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, false, [यह, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    व्योम visit(AstVarScope* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    व्योम visit(AstEnumItem* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstTypedef* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstTypedefFwd* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstActive* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    व्योम visit(AstCell* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    व्योम visit(AstCellInline* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [यह, nodep]() { m_hash += nodep->name(); });
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        guardRecursion(nodep, [यह, nodep]() {  //
            m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                अगर (m_cacheInUser4) nodep->user4(V3Hash{1}.मूल्य());
            });
        });
    }
    व्योम visit(AstModport* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstModportVarRef* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    व्योम visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstBegin* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    व्योम visit(AstFork* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    व्योम visit(AstPin* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [यह, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_अंक();
        });
    }
    व्योम visit(AstText* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    व्योम visit(AstTextBlock* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, [यह, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    व्योम visit(AstCStmt* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstCStmtUser* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, false, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstCExpr* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstCExprUser* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    व्योम visit(AstWith* nodep) override {
        m_hash += hash_गाँठ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

खुला:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_गाँठ* nodep)
        : m_cacheInUser4{true} {
        iterateConst(nodep);
    }
    श्रेणी Uncached {};
    HasherVisitor(स्थिर Ast_गाँठ* nodep, Uncached)
        : m_cacheInUser4{false} {
        iterateConst(const_cast<Ast_गाँठ*>(nodep));
    }
    V3Hash finalHash() स्थिर { वापस m_hash; }
    ~HasherVisitor() override = शेष;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::चालक()(Ast_गाँठ* nodep) स्थिर {
    अगर (!nodep->user4()) HasherVisitor{nodep};
    वापस V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_गाँठ* nodep) स्थिर {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    वापस V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(स्थिर Ast_गाँठ* nodep) {
    स्थिर HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    वापस visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

माप_प्रकार V3HasherUncachedHash(स्थिर Ast_गाँठ& गाँठ) {
    वापस static_cast<माप_प्रकार>(V3Hasher::uncachedHash(&गाँठ).मूल्य());
}
