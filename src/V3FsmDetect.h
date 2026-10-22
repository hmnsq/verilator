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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3FSMDETECT_H_
#وصف_ڪريو VERILATOR_V3FSMDETECT_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class AstNetlist;

class V3FsmDetect final {
عام:
    // Detect FSMs while the original clocked/case structure is still visible,
    // then immediately lower the recovered graphs into concrete coverage
    // instrumentation as a second local phase in the same pass.
    ساكن فراغ detect(AstNetlist* rootp) VL_MT_DISABLED;
};

#جيڪڏهن_پورو
