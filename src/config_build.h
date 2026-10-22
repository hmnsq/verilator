// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Configure source; system configuration
//
// This file is part of Verilator.
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

//**********************************************************************
//**** Version and host name

#समावेश "config_package.h"

//**********************************************************************
//**** Functions

//**********************************************************************
//**** Headers

//**********************************************************************
//**** Default environment

// Set defines to defaults for environment variables
// If set to "", this default is ignored and the user is expected
// to set them at Verilator runtime.

// clang-format off
#अगर_अघोषित DEFENV_SYSTEMC
# define DEFENV_SYSTEMC ""
#पूर्ण_अगर
#अगर_अघोषित DEFENV_SYSTEMC_ARCH
# define DEFENV_SYSTEMC_ARCH ""
#पूर्ण_अगर
#अगर_अघोषित DEFENV_SYSTEMC_INCLUDE
# define DEFENV_SYSTEMC_INCLUDE ""
#पूर्ण_अगर
#अगर_अघोषित DEFENV_SYSTEMC_LIBDIR
# define DEFENV_SYSTEMC_LIBDIR ""
#पूर्ण_अगर
#अगर_अघोषित DEFENV_VERILATOR_मूल
# define DEFENV_VERILATOR_मूल ""
#पूर्ण_अगर
#अगर_अघोषित DEFENV_VERILATOR_SOLVER
# define DEFENV_VERILATOR_SOLVER ""
#पूर्ण_अगर
// clang-format on

//**********************************************************************
//**** Compile options

#समावेश <cstdio>
#समावेश <cstdlib>
#समावेश <cstring>
#समावेश <iostream>
#समावेश <string>

#समावेश <sys/types.h>

// Avoid needing std:: prefixes on some very common items
using string = std::string;
using std::cout;
using std::endl;
using namespace std::literals;  // "<std::string literal>"s; see SF.7 core guideline

//**********************************************************************
//**** OS and compiler specifics

#समावेश "verilatedos.h"
