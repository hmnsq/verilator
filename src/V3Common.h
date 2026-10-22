// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add common contents to modules
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

#ifndef VERILATOR_V3COMMON_H_
#define VERILATOR_V3COMMON_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

class Ast_ग्रन्थिः_DType;

//============================================================================

class V3Common final {
सार्वजनिक:
    स्थिर शून्य commonAll() VL_MT_DISABLED;
    स्थिर string makeToStringCall(Ast_ग्रन्थिः_DType* nodep, ध्रुव std::string& lhs);
};

#endif  // Guard
