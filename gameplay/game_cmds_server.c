/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2023-2024 starfrost

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <game_local.h>

/*
==============================================================================

PACKET FILTERING / USER BANS


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

typedef struct
{
	uint32_t	mask;
	uint32_t	compare;
} ipfilter_t;

#define	MAX_IPFILTERS	1024

ipfilter_t	ipfilters[MAX_IPFILTERS];
int32_t		numipfilters;

/*
=================
StringToFilter
=================
*/
static bool StringToFilter(char* s, ipfilter_t* f)
{
	char	num[128];
	int32_t	i, j;
	uint8_t	b[4];
	uint8_t	m[4];

	for (i = 0; i < 4; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}

	for (i = 0; i < 4; i++)
	{
		if (*s < '0' || *s > '9')
		{
			gi.cprintf(NULL, PRINT_HIGH, "Bad filter address: %s\n", s);
			return false;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = *(uint32_t*)m;
	f->compare = *(uint32_t*)b;

	return true;
}

/*
=================
SV_FilterPacket
=================
*/
bool Server_IsClientAllowed(char* from)
{
	int32_t		i;
	uint32_t	in;
	uint8_t		m[4];
	char* p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i] * 10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}

	in = *(uint32_t*)m;

	for (i = 0; i < numipfilters; i++)
		if ((in & ipfilters[i].mask) == ipfilters[i].compare)
			return (int32_t)filterban->value;

	return (int32_t)!filterban->value;
}


/*
=================
SV_AddIP_f
=================
*/
void Server_CommandAddIP()
{
	int32_t	i;

	if (gi.Cmd_Argc() < 3)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Usage:  addip <ip-mask>\n");
		return;
	}

	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break;	
	// free spot
	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			gi.cprintf(NULL, PRINT_HIGH, "IP filter list is full\n");
			return;
		}
		numipfilters++;
	}

	if (!StringToFilter(gi.Cmd_Argv(2), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
=================
SV_RemoveIP_f
=================
*/
void Server_CommandRemoveIP()
{
	ipfilter_t	f;
	int32_t		i, j;

	if (gi.Cmd_Argc() < 3)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Usage:  sv removeip <ip-mask>\n");
		return;
	}

	if (!StringToFilter(gi.Cmd_Argv(2), &f))
		return;

	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].mask == f.mask
			&& ipfilters[i].compare == f.compare)
		{
			for (j = i + 1; j < numipfilters; j++)
				ipfilters[j - 1] = ipfilters[j];
			numipfilters--;
			gi.cprintf(NULL, PRINT_HIGH, "Removed.\n");
			return;
		}
	gi.cprintf(NULL, PRINT_HIGH, "Didn't find %s.\n", gi.Cmd_Argv(2));
}

/*
=================
SV_ListIP_f
=================
*/
void Server_CommandListIP()
{
	int32_t 	i;
	uint8_t	b[4];

	gi.cprintf(NULL, PRINT_HIGH, "Filter list:\n");

	for (i = 0; i < numipfilters; i++)
	{
		*(uint32_t*)b = ipfilters[i].compare;
		gi.cprintf(NULL, PRINT_HIGH, "%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

/*
=================
SV_WriteIP_f
=================
*/
void SVCmd_WriteIP_f()
{
	FILE*	f;
	char	name[MAX_OSPATH];
	uint8_t	b[4];
	int32_t	i;
	cvar_t* game;

	game = gi.Cvar_Get("game_asset_path", "", 0);

	if (!*game->string)
		sprintf(name, "%s/listip.cfg", GAME_NAME);
	else
		sprintf(name, "%s/listip.cfg", game->string);

	gi.cprintf(NULL, PRINT_HIGH, "Writing %s.\n", name);

	f = fopen(name, "wb");
	if (!f)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Couldn't open %s\n", name);
		return;
	}

	fprintf(f, "set filterban %d\n", (int32_t)filterban->value);

	for (i = 0; i < numipfilters; i++)
	{
		*(uint32_t*)b = ipfilters[i].compare;
		fprintf(f, "sv addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}

	fclose(f);
}

/*
=================
ServerCommand

ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() / gi.argv() commands to get the rest
of the parameters
=================
*/
void	Server_Command()
{
	char* cmd;

	cmd = gi.Cmd_Argv(1);
	if (Q_stricmp(cmd, "addip") == 0)
		Server_CommandAddIP();
	else if (Q_stricmp(cmd, "removeip") == 0)
		Server_CommandRemoveIP();
	else if (Q_stricmp(cmd, "listip") == 0)
		Server_CommandListIP();
	else if (Q_stricmp(cmd, "writeip") == 0)
		SVCmd_WriteIP_f();
	else
		gi.cprintf(NULL, PRINT_HIGH, "Unknown server command \"%s\"\n", cmd);
}

