/* editmenu.c */
/* Copyright 1999 Bob van der Poel. See COPYING for details */

#include "ved.h"
#include "attrs.h"

static MENUITEM fileitems[] =
{
	{"New File", NULL, 'N', file_new},
	{"Append File", NULL, 'G', file_append},
	{"Save buffer", NULL, 'S', xsave_buffer},
	{"Close", NULL, 'C', quit},
	{"eXit", NULL, 'X', quit_all}
};

static MENUITEM blockitems[] =
{
	{"Copy", NULL, 'C', block_copy},
	{"Delete", NULL, 'D', block_delete},
	{"Move", NULL, 'M', block_move},
	{"Save", NULL, 'S', block_save},
	{"Paste", NULL, 'P', block_paste},
	{"cuT", NULL, 'T', block_cut},
	{"Get", NULL, 'G', block_get},
	{"Yget", NULL, 'Y', block_yget},
	{"eraZe Markers", NULL, 'Z', block_delmarks},
	{"sOrt", NULL, 'O', block_sort},
	{"double block 2", NULL, '2', block_double},
	{"Unformat block", NULL, 'U', block_unformat},
	{"Wordwrap block", NULL, 'W', block_wordwrap},
	{"move to Beginning", NULL, 'B', block_start},
	{"move to End", NULL, 'E', block_end},
	{"copy to buFfer", NULL, 'F', block_to_buffer}
};

static MENUITEM searchitems[] =
{
	{"Jump", NULL, 'J', jump},
	{"Find", NULL, 'F', find},
	{"Find Next", NULL, 'N', find_next},
	{"Find Back", NULL, 'B', find_back},
	{"Find Prior", NULL, 'P', find_prior}
};


static MENUITEM optionitems[] =
{
	{NULL, menutext_wordwrap, 'W', toggle_wordwrap},
	{NULL, menutext_crdisplay, 'O', toggle_crdisplay},
	{NULL, menutext_overwrite, 'M', toggle_overwrite},
	{NULL, menutext_autoindent, 'I', toggle_autoindent},
	{NULL, menutext_cmode, 'C', toggle_cmode},
	{NULL, menutext_casematch, 'A', toggle_casematch},
	{NULL, menutext_autonum, 'N', toggle_autonum},
	{"set taB width", NULL, 'B', set_tabwidth},
	{"wilDcard character", NULL, 'D', set_wildcard},
	{"seT number", NULL, 'T', autonumber_set},
	{"Keybindings", NULL, 'K', set_keybindings},
	{"Load options", NULL, 'L', load_rc},
	{"Save options", NULL, 'S', create_rc},
	{"Enter RC command", NULL, 'E', set_rc_line}
};

static MENUITEM windowitems[] =
{
	{"Make new window", NULL, 'M', windows_new},
	{"File new window", NULL, 'F', windows_file},
	{"Select window", NULL, 'S', windows_raise}
#ifdef NCURSES_VERSION
	,

	{"display All windows", NULL, 'A', windows_showall},
	{"Resize window", NULL, 'R', resize_window},
	{"moVe window", NULL, 'V', move_window}
#endif
};

static MENUITEM spellitems[] =
{
	{"Lookup current word", NULL, 'l', speller_word},
	{"Spellcheck buffer", NULL, 's', speller_file},
	{"spellcheck Block", NULL, 'b', speller_block},
	{"Write new wordlist", NULL, 'w', speller_save_new_words}
};


static MENUITEM helpitems[] =
{
	{"Block", NULL, 'B', help_block},
	{"Cursor", NULL, 'C', help_cursor},
	{"Delete", NULL, 'D', help_delete},
	{"dIsplay", NULL, 'I', help_display},
	{"Edit", NULL, 'E', help_edit},
	{"File", NULL, 'F', help_file},
	{"Search", NULL, 'S', help_search},
	{"Macros", NULL, 'M', help_macros},
	{"menUs", NULL, 'U', help_menus},
	{"misC", NULL, 'C', help_misc},
	{"sPell", NULL, 'P', help_spell},
	{"About", NULL, 'A', about_ved}
};

static MENUPULL pull_menus[] =
{
	{"File", fileitems, arraysize(fileitems), 'f'},
	{"Block", blockitems, arraysize(blockitems), 'b'},
	{"Search", searchitems, arraysize(searchitems), 's'},
	{"Options", optionitems, arraysize(optionitems), 'o'},
	{"Windows", windowitems, arraysize(windowitems), 'w'},
	{"speLL", spellitems, arraysize(spellitems), 'l'},
	{"Help", helpitems, arraysize(helpitems), 'h'}
};


static MENU menu =
{
    pull_menus, arraysize(pull_menus), EDITOR_MENU, EDITOR_MENU_HIGHLIGHT
};


void
create_edit_menu(WINDOW * w)
{
	menu_create(w, &menu);
}

MENU *
get_edit_menu_ptr(void)
{
	return &menu;
}

void
menu0_enable(void)
{
	menu_line_do(bf->mw, get_edit_menu_ptr(), pull_menus[0].shortcut);
}

void
menu1_enable(void)
{
	menu_line_do(bf->mw, get_edit_menu_ptr(), pull_menus[1].shortcut);
}


void
menu2_enable(void)
{
	menu_line_do(bf->mw, get_edit_menu_ptr(), pull_menus[2].shortcut);
}

void
menu3_enable(void)
{
	menu_line_do(bf->mw, get_edit_menu_ptr(), pull_menus[3].shortcut);
}

void
menu4_enable(void)
{
	menu_line_do(bf->mw, get_edit_menu_ptr(), pull_menus[4].shortcut);
}

void
menu5_enable(void)
{
	menu_line_do(bf->mw, get_edit_menu_ptr(), pull_menus[5].shortcut);
}


void
menu6_enable(void)
{
	menu_line_do(bf->mw, get_edit_menu_ptr(), pull_menus[6].shortcut);
}


/* EOF */
