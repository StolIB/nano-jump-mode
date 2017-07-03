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

static char *header_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
typedef struct _node {
	int y, x;
	char c;
	struct _node *next;
} node_t;

int do_highlight_char(WINDOW *win, char c, node_t **heads, node_t **tails) {
	int p = 0, num_highlighted = 0;

	// highlight initial
	int y = 0, x = 0;
	char at[2];
	wmove(win, 0, 0);
	
	while (mvwinnstr(win, y, x, at, 1) > 0) {
		bool prev_was_space = true;
		while (mvwinnstr(win, y, x, at, 1) > 0) {
			if (tolower(at[0]) == c) {
				if (prev_was_space == true) {
					wattron(win, A_STANDOUT | A_BOLD);
					mvwaddch(win, y, x, header_chars[p]);

					if (heads[p] == NULL) {
						heads[p] = malloc(sizeof(node_t));
						tails[p] = heads[p];
					} else {
						tails[p]->next = malloc(sizeof(node_t));
						tails[p] = tails[p]->next;
					}
					tails[p]->y = y; tails[p]->x = x;
					tails[p]->c = at[0];
					tails[p]->next = NULL;
				
					p++;
					if (header_chars[p] == '\0') { p = 0; }
					num_highlighted++;
				
					wattroff(win, A_STANDOUT | A_BOLD);
				}
			}
			prev_was_space = (at[0] == ' ');
			x++;
		}
		x = 0;
		y++;
	}
	wrefresh(win);

	return num_highlighted;
}

int do_highlight_these(WINDOW *win, node_t *these, node_t **heads, node_t **tails) {
	int p = 0, num_highlighted = 0;

	// highlight initial
	wmove(win, 0, 0);

	while (these != NULL) {
	  wattron(win, A_STANDOUT | A_BOLD);
	  mvwaddch(win, these->y, these->x, header_chars[p]);

	  if (heads[p] == NULL) {
		heads[p] = malloc(sizeof(node_t));
		tails[p] = heads[p];
	  } else {
		tails[p]->next = malloc(sizeof(node_t));
		tails[p] = tails[p]->next;
	  }
	  tails[p]->y = these->y; tails[p]->x = these->x;
	  tails[p]->c = these->c;
	  tails[p]->next = NULL;
				
	  p++;
	  if (header_chars[p] == '\0') { p = 0; }
	  num_highlighted++;

	  these = these->next;
				
	  wattroff(win, A_STANDOUT | A_BOLD);
	}	  

	wrefresh(win);

	return num_highlighted;
}

void cleanup_highlight(WINDOW *win, node_t **heads, node_t **tails) {
	for (int i = 0; i < strlen(header_chars); i++) {
		node_t *cur = heads[i];
		while (cur != NULL) {
			node_t *trash = cur;

			// restore old character
			wattroff(win, A_STANDOUT | A_BOLD);
			mvwaddch(win, cur->y, cur->x, cur->c);
			
			cur = cur->next;
			free(trash);
		}
	}
	wrefresh(win);

	for (int i = 0; i < strlen(header_chars); i++) {
	  heads[i] = NULL; tails[i] = NULL;
	}
}

void do_jump(void) {
	int prev_y, prev_x;
	getyx(edit, prev_y, prev_x);
	int prev_line = openfile->current->lineno;
	int prev_col = xplustabs() + 1;
	
	char head_char = do_char_prompt(_("Head char: "));
	head_char = tolower(head_char);

	if (head_char == '\0') {
		statusbar(_("Cancelled"));
		return;
	} else if (head_char == '\1') {
		statusbar(_("jump-mode: Unprintable character"));
		return;
	} else if (head_char == ' ') {
		statusbar(_("jump-mode: Don't support jumping to 'space'"));
		return;
	}
	
	// setup
	node_t *heads[strlen(header_chars)];
	node_t *tails[strlen(header_chars)];
	for (int i = 0; i < strlen(header_chars); i++) {
		heads[i] = NULL; tails[i] = NULL;
	}

	int num_highlighted = do_highlight_char(edit, head_char, heads, tails);
	bool recursed = false;
 narrow:
	blank_statusbar();
	if (num_highlighted <= 0) {
		cleanup_highlight(edit, heads, tails);
		statusbar(_("jump-mode: No one found"));
		return;
	}

	char select_char = '\0';
	if (num_highlighted == 1) {
		select_char = header_chars[0];
		if (!recursed) {
		  statusbar(_("jump-mode: One candidate, move to it directly"));
		}
	} else {
	  select_char = do_char_prompt(_("Select: "));
	  blank_statusbar();
	}
	
	if (select_char == '\0') {
		cleanup_highlight(edit, heads, tails);
		statusbar(_("Cancelled"));
		return;
	}
	
	int list_idx = (int)(strchr(header_chars, select_char) - header_chars);
	if ((list_idx < 0) || (list_idx >= num_highlighted)) {
		cleanup_highlight(edit, heads, tails);
		statusbar(_("jump-mode: No such position candidate"));
		return;
	}
	
	node_t *list = heads[list_idx];

	if (num_highlighted > strlen(header_chars)) {
	  node_t *save = NULL, *cur = NULL;
	  save = malloc(sizeof(node_t)); cur = save;
	  do {
		cur->x = list->x; cur->y = list->y;
		cur->c = list->c;
		if (list->next != NULL) {
		  cur->next = malloc(sizeof(node_t));
		} else (cur->next = NULL);
		cur = cur->next; list = list->next;
	  } while (list != NULL);
	  cleanup_highlight(edit, heads, tails);
	  num_highlighted = do_highlight_these(edit, save, heads, tails);
	  while (save != NULL) {
		node_t *trash = save;
		save = save->next;
		free(trash);
	  }
	  recursed = true;
	  goto narrow;
	}
	
	int final_y = list->y, final_x = list->x;
	int delta_y = final_y - prev_y;
	int delta_x = final_x - prev_x;

	cleanup_highlight(edit, heads, tails);

	goto_line_posx(prev_line, prev_x + delta_x);
	if (delta_y > 0) { for (int i = 0; i < delta_y; i++) { do_down(false); } }
	if (delta_y < 0) { for (int i = 0; i > delta_y; i--) { do_up(false); } }
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
