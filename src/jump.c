/**************************************************************************
 *   jump.c  --  This file is part of GNU nano.                           *
 *     this file: 2017 Andrew D'Angelo <dangeloandrew at outlook dot com> *
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

// TODO: add nanorc options to enable jumping inside words and setting `header_chars`
static char *header_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int max_depth = 10;
typedef struct _node {
	int y, x;
	int line, col;
	char c;
	struct _node *next;
} node_t;

// prompt for a single character of input (submits automatically)
char do_char_prompt(const char *msg);

// construct a copy of `node` and insert into the given linked-list
void emplace(node_t *node, node_t **head_ptr, node_t **tail_ptr);

// highlight all occurences of `c` in the given window according to the `header_chars`
// order. If a location is highlighted with the character 'a', a new linked-list
// node will be emplaced into `heads[(location of 'a' in header_chars)]`.
int do_highlight_char(WINDOW *win, char c, node_t **heads, node_t **tails);

// highlight all locations referred to by the nodes of `these`. New nodes will
// be emplaced into the `heads` array as in `do_highlight_char`.
int do_highlight_these(WINDOW *win, node_t *these, node_t **heads, node_t **tails);

// Restore all locations in the lists of `heads` to their original state and
// delete the linked-list nodes.
void cleanup_highlight(WINDOW *win, node_t **heads, node_t **tails);

// the main read-eval-jump loop:
void do_jump(void) {
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
	node_t *saved_head = NULL, *saved_tail = NULL; // when we recurse on chars
	for (int i = 0; i < strlen(header_chars); i++) {
		heads[i] = NULL; tails[i] = NULL;
	}

	int num_highlighted;
	int recursed = 0;

	int final_line = 0, final_col = 0;

	while (recursed < max_depth) {
		// give choices to narrow on
		if (!recursed) {
			num_highlighted = do_highlight_char(edit, head_char, heads, tails);
		} else {
			num_highlighted = do_highlight_these(edit, saved_head, heads, tails);
			while (saved_head != NULL) {
				node_t *trash = saved_head;
				saved_head = saved_head->next;
				free(trash);
			}
		}

		blank_statusbar();
		
		if (num_highlighted <= 0) { // picked a nonexistent char
			cleanup_highlight(edit, heads, tails);
			statusbar(_("jump-mode: No one found"));
			return;
		}
		
		if (num_highlighted == 1) { // picked the only occurrence
			if (!recursed) {
				statusbar(_("jump-mode: One candidate, move to it directly"));
			}
			final_line = heads[0]->line;
			final_col = heads[0]->col;
			break;
		}

		// have to narrow down some more
		char select_char = '\0';
		select_char = do_char_prompt(_("Select: "));
		blank_statusbar();
		
		if (select_char == '\0') { // user cancelled
			cleanup_highlight(edit, heads, tails);
			statusbar(_("Cancelled"));
			return;
		}

		// see what index the user picked
		int list_idx = (int)(strchr(header_chars, select_char) - header_chars);
		if ((list_idx < 0) || (list_idx >= num_highlighted)) {
			cleanup_highlight(edit, heads, tails);
			statusbar(_("jump-mode: No such position candidate"));
			return;
		}

		// get the list of chars corresponding to index
		node_t *list = heads[list_idx];
		if (num_highlighted > strlen(header_chars)) {
			do { // back up the list before cleaning this hilite
				emplace(list, &saved_head, &saved_tail);
				list = list->next;
			} while (list != NULL);
			cleanup_highlight(edit, heads, tails);
			
			recursed++;
			continue; // narrow down some more
		} else { // got a final result
			final_line = list->line;
			final_col = list->col;
			break;
		}
	}
	
	cleanup_highlight(edit, heads, tails);
	do_gotolinecolumn(final_line, final_col + 1, false, false);
	refresh_needed = TRUE;
}

void do_jump_void(void) {
	if (currmenu == MMAIN) {
		do_jump();
	} else {
		beep();
	}
}

char do_char_prompt(const char *msg) {
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

void emplace(node_t *node, node_t **head_ptr, node_t **tail_ptr) {
	node_t *head = *head_ptr;
	node_t *tail = *tail_ptr;
	if (head == NULL) {
		head = malloc(sizeof(node_t));
		*head_ptr = head;
		*tail_ptr = head;
		tail = head;
	} else {
		tail->next = malloc(sizeof(node_t));
		tail = tail->next;
		*tail_ptr = tail;
	}
	*tail = *node;
	tail->next = NULL;
}

int do_highlight_char(WINDOW *win, char c, node_t **heads, node_t **tails) {
	int p = 0, num_highlighted = 0;

	// highlight initial
	int y = 0, x = 0;
	int max_y = 0, max_x = 0;
	getmaxyx(win, max_y, max_x);
	filestruct *line_ptr = openfile->edittop;
	int line = openfile->edittop->lineno;
	int col = openfile->firstcolumn;
	char *at = &line_ptr->data[col];
	bool prev_was_space = true;
	
	while ((y <= max_y) && (x <= max_x)) {
		if (*at == '\0') { // reached the end of the line data
			line++; col = 0;
			y++; x = 0;
			if (line_ptr->next->lineno != line_ptr->lineno) {
				prev_was_space = true;
			}
			line_ptr = line_ptr->next;
			at = &line_ptr->data[col];
			continue;
		} else {
			if (tolower(*at) == c) {
				if (prev_was_space) {
					wattron(win, A_STANDOUT | A_BOLD);
					mvwaddch(win, y, x, header_chars[p]);

					node_t new = (node_t){y, x, line, col, *at, NULL};
					emplace(&new, &heads[p], &tails[p]);
				
					p++;
					if (header_chars[p] == '\0') { p = 0; }
					num_highlighted++;
				
					wattroff(win, A_STANDOUT | A_BOLD);
				}
			}
		}
		
		prev_was_space = (*at == ' ');
		size_t charwidth = 0;
		parse_mbchar(at, NULL, &charwidth);
		col += charwidth;
		x += charwidth;
		at++;
		if (x >= editwincols) {
			y++; x = 0;
			if (!ISSET(SOFTWRAP) && (*at != '\0')) { // reached the end of the screen but there's more
				prev_was_space = true;
				line_ptr = line_ptr->next;
				line++; col = 0;
				at = &line_ptr->data[col];
			}
		}
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

		emplace(these, &heads[p], &tails[p]);
		
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
