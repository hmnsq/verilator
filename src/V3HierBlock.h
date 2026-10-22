// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Plan for Hierarchical Verilation
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

#ifndef VERILATOR_V3HIERBLOCK_H_
#define VERILATOR_V3HIERBLOCK_H_

#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Graph.h"

#شامل <map>
#شامل <set>
#شامل <string>
#شامل <unordered_map>
#شامل <unordered_set>
#شامل <utility>
#شامل <vector>

class AstNetlist;
class Ast_گره_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3HierGraph final : عمومی V3Graph {
عمومی:
    V3HierGraph() = پیشفرض;
    ~V3HierGraph() = پیشفرض;
    VL_UNCOPYABLE(V3HierGraph);
    VL_UNMOVABLE(V3HierGraph);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    ایستا VStringList sourceFiles(ثابت string& بالاترین_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    تهی writeCommandArgsFiles(bool forMkJson) ثابت VL_MT_DISABLED;
    تهی writeParametersFiles() ثابت VL_MT_DISABLED;
    ایستا string بالاترین_Command_Args_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3HierBlock final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(V3HierBlock, V3GraphVertex)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::vector<StrGParam>;

    // MEMBERS
    ثابت AstModule* ثابت m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    ثابت std::vector<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    ثابت std::vector<AstParamTypeDType*> m_typeParams;

    // METHODS
    ایستا StrGParams stringifyParams(ثابت std::vector<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

عمومی:
    // CONSTRUCTORs
    V3HierBlock(V3HierGraph* graphp, ثابت AstModule* modp, ثابت std::vector<AstVar*>& params,
                ثابت std::vector<AstParamTypeDType*>& typeParams)
        : V3GraphVertex{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3HierBlock() VL_MT_DISABLED = پیشفرض;
    VL_UNCOPYABLE(V3HierBlock);
    VL_UNMOVABLE(V3HierBlock);

    ثابت AstModule* modp() ثابت { بازگشت m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    ایستا bool stringParamPassable(ثابت string& مقدار) VL_PURE;

    // For emitting Makefile and build definition JSON
    VStringList commandArgs(bool forMkJson) ثابت VL_MT_DISABLED;
    VStringList hierBlockArgs() ثابت VL_MT_DISABLED;
    string hierPrefix() ثابت VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, ثابت نویسه* prefix,
                            ثابت نویسه* suffix) ثابت VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) ثابت VL_MT_DISABLED;
    string hierMkFilename(bool withDir) ثابت VL_MT_DISABLED;
    string hierLibFilename(bool withDir) ثابت VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) ثابت VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() ثابت VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    تهی writeCommandArgsFile(bool forMkJson) ثابت VL_MT_DISABLED;
    تهی writeParametersFile() ثابت VL_MT_DISABLED;
    string commandArgsFilename(bool forMkJson) ثابت VL_MT_DISABLED;
    string typeParametersFilename() ثابت VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() ثابت override { بازگشت m_modp->prettyNameQ(); }
    std::string dotShape() ثابت override { بازگشت "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3Hierarchical final {
عمومی:
    ایستا تهی createGraph(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // guard
