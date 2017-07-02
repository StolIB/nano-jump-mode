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

void do_jump(void) {
	char c = do_char_prompt(_("Head char: "));

	if (c == '\0') {
		statusbar(_("Cancelled"));
		return;
	} else if (c == '\1') {
		statusbar(_("jump-mode: Unprintable character"));
		return;
	}

	char *r;
	asprintf(&r, "Got answer: %c", c);
	statusbar(_(r));
	free(r);
}

void do_jump_void(void) {
	if (currmenu == MMAIN) {
		do_jump();
	} else {
		beep();
	}
}

void jump_abort(void) {
	if (openfile->mark_set) {
		refresh_needed = TRUE;
	}
}

char do_char_prompt(const char *msg)
{
	char response = '\0';
	int width = 16;
	char *message = display_string(msg, 0, COLS, FALSE);

	int kbinput;
	functionptrtype func;

	if (!ISSET(NO_HELP)) {

		if (COLS < 32) {
			width = COLS / 2;
		}

		/* Clear the shortcut list from the bottom of the screen. */
		blank_bottombars();

		wmove(bottomwin, 1, 0);
		onekey("^C", _("Cancel"), width);
	}

	/* Color the statusbar over its full width and display the question. */
	wattron(bottomwin, interface_color_pair[TITLE_BAR]);
	blank_statusbar();
	mvwaddnstr(bottomwin, 0, 0, message, actual_x(message, COLS - 1));
	wattroff(bottomwin, interface_color_pair[TITLE_BAR]);

	wnoutrefresh(bottomwin);

	currmenu = MYESNO;
	kbinput = get_kbinput(bottomwin);

	func = func_from_key(&kbinput);

	if (func == do_cancel) {
		response = '\0';
	} else if ((' ' <= kbinput) && (kbinput <= '~')) {
		response = kbinput;
	} else {
		response = '\1';
	}
	
	free(message);

	return response;
}
