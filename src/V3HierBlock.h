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

#अगर_अघोषित VERILATOR_V3HIERBLOCK_H_
#घोषणा VERILATOR_V3HIERBLOCK_H_

#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Graph.h"

#समावेश <map>
#समावेश <set>
#समावेश <string>
#समावेश <unordered_map>
#समावेश <unordered_set>
#समावेश <utility>
#समावेश <vector>

class AstNetlist;
class Ast_गाँठ_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3HierGraph final : खुला V3Graph {
खुला:
    V3HierGraph() = शेष;
    ~V3HierGraph() = शेष;
    VL_UNCOPYABLE(V3HierGraph);
    VL_UNMOVABLE(V3HierGraph);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    अटल VStringList sourceFiles(स्थिर string& शीर्ष_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    व्योम writeCommandArgsFiles(bool forMkJson) स्थिर VL_MT_DISABLED;
    व्योम writeParametersFiles() स्थिर VL_MT_DISABLED;
    अटल string शीर्ष_Command_Args_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3HierBlock final : खुला V3GraphVertex {
    VL_RTTI_IMPL(V3HierBlock, V3GraphVertex)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::vector<StrGParam>;

    // MEMBERS
    स्थिर AstModule* स्थिर m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    स्थिर std::vector<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    स्थिर std::vector<AstParamTypeDType*> m_typeParams;

    // METHODS
    अटल StrGParams stringifyParams(स्थिर std::vector<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

खुला:
    // CONSTRUCTORs
    V3HierBlock(V3HierGraph* graphp, स्थिर AstModule* modp, स्थिर std::vector<AstVar*>& params,
                स्थिर std::vector<AstParamTypeDType*>& typeParams)
        : V3GraphVertex{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3HierBlock() VL_MT_DISABLED = शेष;
    VL_UNCOPYABLE(V3HierBlock);
    VL_UNMOVABLE(V3HierBlock);

    स्थिर AstModule* modp() स्थिर { वापस m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    अटल bool stringParamPassable(स्थिर string& मूल्य) VL_PURE;

    // For emitting Makefile and build definition JSON
    VStringList commandArgs(bool forMkJson) स्थिर VL_MT_DISABLED;
    VStringList hierBlockArgs() स्थिर VL_MT_DISABLED;
    string hierPrefix() स्थिर VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, स्थिर अक्षर* prefix,
                            स्थिर अक्षर* suffix) स्थिर VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) स्थिर VL_MT_DISABLED;
    string hierMkFilename(bool withDir) स्थिर VL_MT_DISABLED;
    string hierLibFilename(bool withDir) स्थिर VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) स्थिर VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() स्थिर VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    व्योम writeCommandArgsFile(bool forMkJson) स्थिर VL_MT_DISABLED;
    व्योम writeParametersFile() स्थिर VL_MT_DISABLED;
    string commandArgsFilename(bool forMkJson) स्थिर VL_MT_DISABLED;
    string typeParametersFilename() स्थिर VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() स्थिर override { वापस m_modp->prettyNameQ(); }
    std::string dotShape() स्थिर override { वापस "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3Hierarchical final {
खुला:
    अटल व्योम createGraph(AstNetlist* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // guard
