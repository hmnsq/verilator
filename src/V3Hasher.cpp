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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Hasher.h"

#अन्तर्भाव <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

वर्ग HasherVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    ध्रुव तार्किक m_cache_अन्तर्_User4;  // Use user4 to cache each V3Hash?
    std::समुच्चयः<Ast_ग्रन्थिः*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    शून्य guardRecursion(Ast_ग्रन्थिः* ध्रुव nodep, std::function<शून्य()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        यदि (!m_cache_अन्तर्_User4) {
            यदि (m_visited.find(nodep) != m_visited.अन्तः_2()) {
                m_hash += V3Hash{nodep->name()};
                प्रत्यागम;
            }
            m_visited.निवेशय(nodep);
        }
        f();
    }

    V3Hash hash_ग्रन्थिः_And_Iterate(Ast_ग्रन्थिः* nodep, तार्किक hashDType, तार्किक hashChildren,
                              std::function<शून्य()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        यदि (m_cache_अन्तर्_User4 && nodep->user4()) {
            प्रत्यागम V3Hash{static_cast<uint32_t>(nodep->user4())};
        } अन्यथा {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            यदि (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            यदि (hashChildren) iterateChildrenConst(nodep);  // Children
            यदि (m_cache_अन्तर्_User4) nodep->user4(m_hash.मानम्());
            प्रत्यागम m_hash;
        }
    }

    // VISITORS

    constexpr स्थिर तार्किक HASH_DTYPE = सत्य;
    constexpr स्थिर तार्किक HASH_CHILDREN = सत्य;

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

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
#यदि VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#यदि_अन्तः
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    शून्य दर्शय(Ast_ग्रन्थिः_Array_DType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->वामतः();
            m_hash += nodep->दक्षिणतः();
        });
    }
    शून्य दर्शय(Ast_ग्रन्थिः_UOr_Struct_DType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, असत्य, [एतत्, nodep]() {  //
            m_hash += nodep->unique_अङ्कः();
        });
    }
    शून्य दर्शय(AstParamTypeDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    शून्य दर्शय(AstMemberDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(AstDefImplicitDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->unique_अङ्कः();
        });
    }
    शून्य दर्शय(AstAssocArrayDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    शून्य दर्शय(AstBracketArrayDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य दर्शय(AstDynArrayDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य दर्शय(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य दर्शय(AstWildcardArrayDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य दर्शय(AstBasicDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().वामतः();
            m_hash += nodep->nrange().दक्षिणतः();
        });
    }
    शून्य दर्शय(AstCDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(AstConstDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य दर्शय(AstClassRefDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    शून्य दर्शय(AstIfaceRefDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    शून्य दर्शय(AstQueueDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य दर्शय(AstRefDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    शून्य दर्शय(Ast_प्रवाहः_DType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstVoidDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstEnumDType* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, असत्य, [एतत्, nodep]() {  //
            m_hash += nodep->unique_अङ्कः();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstSel* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->विस्तारः_Const();
        });
    }
    शून्य दर्शय(AstConst* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->अङ्कः().गन्तव्यम्_Hash();
        });
    }
    शून्य दर्शय(AstNullCheck* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstCCast* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->आकार();
        });
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            यदि (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } अन्यथा {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    शून्य दर्शय(AstVarXRef* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(AstFScanF* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    शून्य दर्शय(AstSScanF* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    शून्य दर्शय(Ast_Addr_सम्बन्धि_CFunc* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    शून्य दर्शय(AstTriggered* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            प्रति (ध्रुव uint32_t सूचकांक : nodep->indices()) m_hash += सूचकांक;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    शून्य दर्शय(AstCaseItem* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    शून्य दर्शय(AstCMethodHard* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->विधानम्();
        });
    }
    शून्य दर्शय(AstCAwait* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    शून्य दर्शय(AstCLocalScope* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstCoverInc* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    शून्य दर्शय(AstDisplay* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    शून्य दर्शय(AstMonitorOff* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    शून्य दर्शय(AstJumpGo* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, असत्य, []() {});
    }
    शून्य दर्शय(AstTraceInc* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cover_Or_Assert* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    शून्य दर्शय(Ast_ग्रन्थिः_Range* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, असत्य, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Pre_Sel* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstClassExtends* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(Ast_Foreach_शीर्षकम्* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstDefParam* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstArg* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstParseRef* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(AstClassOrPackageRef* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    शून्य दर्शय(AstSenItem* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    शून्य दर्शय(Ast_Sen_वृक्षः* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstSFormatF* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    शून्य दर्शय(AstElabDisplay* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cons_Item* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ध्रुव AstUnpackArrayDType* ध्रुव dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ असत्य,
                                     [एतत्, nodep, dtypep]() {
                                         ध्रुव uint32_t आकार = dtypep->elementsConst();
                                         प्रति (uint32_t n = 0; n < आकार; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    शून्य दर्शय(AstPragma* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    शून्य दर्शय(Ast_Attr_सम्बन्धि* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    शून्य दर्शय(Ast_ग्रन्थिः_File* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        guardRecursion(nodep, [एतत्, nodep]() {  //
            m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                यदि (m_cache_अन्तर्_User4) nodep->user4(V3Hash{1}.मानम्());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    शून्य दर्शय(AstVar* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    शून्य दर्शय(AstScope* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, असत्य, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    शून्य दर्शय(AstVarScope* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    शून्य दर्शय(AstEnumItem* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(AstTypedef* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(AstTypedefFwd* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(Ast_सक्रिय* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    शून्य दर्शय(AstCell* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    शून्य दर्शय(AstCellInline* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [एतत्, nodep]() { m_hash += nodep->name(); });
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        guardRecursion(nodep, [एतत्, nodep]() {  //
            m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                यदि (m_cache_अन्तर्_User4) nodep->user4(V3Hash{1}.मानम्());
            });
        });
    }
    शून्य दर्शय(AstModport* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(AstModportVarRef* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    शून्य दर्शय(AstModportFTaskRef* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstBegin* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य दर्शय(AstFork* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->तात्कालिकम्_Start();
        });
    }
    शून्य दर्शय(AstPin* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_अङ्कः();
        });
    }
    शून्य दर्शय(AstText* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    शून्य दर्शय(Ast_Text_खण्डम्* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->उपसर्गः();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    शून्य दर्शय(AstCStmt* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstCStmtUser* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstCExpr* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstCExprUser* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य दर्शय(AstWith* nodep) override {
        m_hash += hash_ग्रन्थिः_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट HasherVisitor(Ast_ग्रन्थिः* nodep)
        : m_cache_अन्तर्_User4{सत्य} {
        iterateConst(nodep);
    }
    वर्ग Uncached {};
    HasherVisitor(ध्रुव Ast_ग्रन्थिः* nodep, Uncached)
        : m_cache_अन्तर्_User4{असत्य} {
        iterateConst(const_cast<Ast_ग्रन्थिः*>(nodep));
    }
    V3Hash finalHash() ध्रुव { प्रत्यागम m_hash; }
    ~HasherVisitor() override = स्वभाव;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::संक्रियकः()(Ast_ग्रन्थिः* nodep) ध्रुव {
    यदि (!nodep->user4()) HasherVisitor{nodep};
    प्रत्यागम V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_ग्रन्थिः* nodep) ध्रुव {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    प्रत्यागम V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ध्रुव Ast_ग्रन्थिः* nodep) {
    ध्रुव HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    प्रत्यागम visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

आकार_t V3HasherUncachedHash(ध्रुव Ast_ग्रन्थिः& ग्रन्थिः) {
    प्रत्यागम static_cast<आकार_t>(V3Hasher::uncachedHash(&ग्रन्थिः).मानम्());
}
