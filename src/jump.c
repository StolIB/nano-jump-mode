/**************************************************************************
 *   jump.c  --  This file is part of GNU nano.                           *
 *                                                                        *
 *   Copyright (C) 2000-2011, 2013-2017 Free Software Foundation, Inc.    *
 *   Copyright (C) 2017 Rishabh Dave                                      *
 *   Copyright (C) 2014-2017 Benno Schulenberg                            *
 *                                                                        *
 *   GNU nano is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published    *
 *   by the Free Software Foundation, either version 3 of the License,    *
 *   or (at your option) any later version.                               *
 *                                                                        *
 *   GNU nano is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "proto.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

void do_jump_void(void) {
	if (currmenu == MMAIN) {
		statusbar(_("Entering jump-mode!"));
	} else {
		beep();
	}
}
