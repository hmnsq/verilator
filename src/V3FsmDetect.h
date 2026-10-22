// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: FSM coverage detect pass
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3FSMDETECT_H_
#define VERILATOR_V3FSMDETECT_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

class AstNetlist;

class V3FsmDetect final {
பொது:
    // Detect FSMs while the original clocked/case structure is still visible,
    // then immediately lower the recovered graphs into concrete coverage
    // instrumentation as a second local phase in the same pass.
    நிலையான வெறுமை detect(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif
