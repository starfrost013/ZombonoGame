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

bool	Pickup_Weapon(edict_t *ent, edict_t *other);
void	Use_Weapon(edict_t *ent, gitem_t *inv);
void	Drop_Weapon(edict_t *ent, gitem_t *inv);


void Weapon_Blaster(edict_t *ent);
void Weapon_Shotgun(edict_t *ent);
void Weapon_SuperShotgun(edict_t *ent);
void Weapon_Machinegun(edict_t *ent);
void Weapon_Chaingun(edict_t *ent);
void Weapon_HyperBlaster(edict_t *ent);
void Weapon_RocketLauncher(edict_t *ent);
void Weapon_Grenade(edict_t *ent);
void Weapon_GrenadeLauncher(edict_t *ent);
void Weapon_Railgun(edict_t *ent);
void Weapon_Bamfuslicator(edict_t *ent);
void Weapon_Tangfuslicator(edict_t* ent);

gitem_armor_t jacketarmor_info	= { 25,  50, .30, .00, ARMOR_JACKET};
gitem_armor_t combatarmor_info	= { 50, 100, .60, .30, ARMOR_COMBAT};
gitem_armor_t bodyarmor_info	= {100, 200, .80, .60, ARMOR_BODY};

#define HEALTH_IGNORE_MAX	1
#define HEALTH_TIMED		2

void Use_Quad (edict_t *ent, gitem_t *item);
static int	quad_drop_timeout_hack;

//======================================================================

/*
===============
GetItemByIndex
===============
*/
gitem_t	*GetItemByIndex (int32_t index)
{
	if (index == 0 || index >= game.num_items)
		return NULL;

	return &itemlist[index];
}


/*
===============
FindItemByClassname

===============
*/
gitem_t	*FindItemByClassname (char *classname)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->classname)
			continue;
		if (!Q_stricmp(it->classname, classname))
			return it;
	}

	return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t	*FindItem (char *pickup_name)
{
	if (!pickup_name)
		return NULL;

	int32_t	i;
	gitem_t	*it;

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->pickup_name)
			continue;
		if (!Q_stricmp(it->pickup_name, pickup_name))
			return it;
	}

	return NULL;
}

//======================================================================

void DoRespawn (edict_t *ent)
{
	if (ent->team)
	{
		edict_t	*master;
		int32_t	count;
		int32_t choice;

		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->chain, count++)
			;

		choice = rand() % count;

		for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
			;
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity (ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;
}

void SetRespawn (edict_t *ent, float delay)
{
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->nextthink = level.time + delay;
	ent->think = DoRespawn;
	gi.linkentity (ent);
}


//======================================================================

bool Pickup_Powerup (edict_t *ent, edict_t *other)
{
	loadout_entry_t* loadout_entry = Loadout_GetItem(other, ent->item->pickup_name);

	// if it's not there add it
	if (!loadout_entry)
		loadout_entry = Loadout_AddItem(other, ent->item->pickup_name, ent->item->icon, loadout_entry_type_ammo, 1);

	int32_t		     quantity = loadout_entry->amount;

	if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
		return false;

	loadout_entry->amount++;

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn(ent, ent->item->quantity);
	if (((int32_t)gameflags->value & GF_INSTANT_ITEMS) || ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM)))
	{
		if ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM))
			quad_drop_timeout_hack = (ent->nextthink - level.time) / FRAMETIME;
		ent->item->use(other, ent->item);
	}

	return true;
}

void Drop_General (edict_t *ent, gitem_t *item)
{
	loadout_entry_t* loadout_entry = Loadout_GetItem(ent, item->pickup_name);

	Drop_Item (ent, item);
	
	loadout_entry->amount--;

	if (loadout_entry->amount == 0)
		Loadout_DeleteItem(ent, item->pickup_name);
}


//======================================================================

bool Pickup_Adrenaline (edict_t *ent, edict_t *other)
{
	/*
	* this is a bad idea in multiplayer tbf
	if (!deathmatch->value)
		other->max_health += 1;
	*/

	if (other->health < other->max_health)
		other->health = other->max_health;

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

bool Pickup_AncientHead (edict_t *ent, edict_t *other)
{
	other->max_health += 2;

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

bool Pickup_Bandolier (edict_t *ent, edict_t *other)
{
	gitem_t	*item = FindItem("Bullets");
	loadout_entry_t* loadout_item = Loadout_GetItem(ent, "Bullets");

	if (other->client->pers.max_bullets < 250)
		other->client->pers.max_bullets = 250;
	if (other->client->pers.max_shells < 150)
		other->client->pers.max_shells = 150;
	if (other->client->pers.max_cells < 250)
		other->client->pers.max_cells = 250;
	if (other->client->pers.max_slugs < 75)
		other->client->pers.max_slugs = 75;

	if (item)
	{
		loadout_item->amount += item->quantity;

		if (loadout_item->amount > other->client->pers.max_bullets)
			loadout_item->amount = other->client->pers.max_bullets;
	}

	item = FindItem("Shells");
	if (item)
	{
		loadout_item->amount += item->quantity;
		if (loadout_item->amount > other->client->pers.max_shells)
			loadout_item->amount = other->client->pers.max_shells;
	}

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

bool Pickup_Pack (edict_t *ent, edict_t *other)
{
	gitem_t* item = FindItem("Bullets");
	loadout_entry_t* loadout_item_ptr = Loadout_GetItem(other, "Bullets");

	// quantity is later
	if (!loadout_item_ptr)
		loadout_item_ptr = Loadout_AddItem(other, item->pickup_name, item->icon, loadout_entry_type_ammo, 0);

	if (other->client->pers.max_bullets < 300)
		other->client->pers.max_bullets = 300;
	if (other->client->pers.max_shells < 200)
		other->client->pers.max_shells = 200;
	if (other->client->pers.max_rockets < 100)
		other->client->pers.max_rockets = 100;
	if (other->client->pers.max_grenades < 100)
		other->client->pers.max_grenades = 100;
	if (other->client->pers.max_cells < 300)
		other->client->pers.max_cells = 300;
	if (other->client->pers.max_slugs < 100)
		other->client->pers.max_slugs = 100;

	item = FindItem("Bullets");
	if (item)
	{
		loadout_item_ptr->amount += item->quantity;
		if (loadout_item_ptr->amount > other->client->pers.max_bullets)
			loadout_item_ptr->amount = other->client->pers.max_bullets;
	}

	item = FindItem("Shells");
	if (item)
	{
		loadout_item_ptr->amount += item->quantity;
		if (loadout_item_ptr->amount > other->client->pers.max_shells)
			loadout_item_ptr->amount = other->client->pers.max_shells;
	}

	item = FindItem("Cells");
	if (item)
	{
		loadout_item_ptr->amount += item->quantity;
		if (loadout_item_ptr->amount > other->client->pers.max_cells)
			loadout_item_ptr->amount = other->client->pers.max_cells;
	}

	item = FindItem("Grenades");
	if (item)
	{
		loadout_item_ptr->amount += item->quantity;
		if (loadout_item_ptr->amount > other->client->pers.max_grenades)
			loadout_item_ptr->amount = other->client->pers.max_grenades;
	}

	item = FindItem("Rockets");
	if (item)
	{
		loadout_item_ptr->amount += item->quantity;
		if (loadout_item_ptr->amount > other->client->pers.max_rockets)
			loadout_item_ptr->amount = other->client->pers.max_rockets;
	}

	item = FindItem("Slugs");
	if (item)
	{
		loadout_item_ptr->amount += item->quantity;
		if (loadout_item_ptr->amount > other->client->pers.max_slugs)
			loadout_item_ptr->amount = other->client->pers.max_slugs;
	}

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

//======================================================================

void Use_Quad (edict_t *ent, gitem_t *item)
{
	int		timeout;

	loadout_entry_t* entry_ptr = Loadout_GetItem(ent, "Quad Damage");
	entry_ptr->amount--;

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else
	{
		timeout = 300;
	}

	if (ent->client->quad_framenum > level.framenum)
		ent->client->quad_framenum += timeout;
	else
		ent->client->quad_framenum = level.framenum + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Breather (edict_t *ent, gitem_t *item)
{
	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, item->pickup_name);
	loadout_entry_ptr->amount--;

	if (ent->client->breather_framenum > level.framenum)
		ent->client->breather_framenum += 300;
	else
		ent->client->breather_framenum = level.framenum + 300;
}

//======================================================================

void Use_Envirosuit (edict_t *ent, gitem_t *item)
{
	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, item->pickup_name);
	loadout_entry_ptr->amount--;

	if (ent->client->enviro_framenum > level.framenum)
		ent->client->enviro_framenum += 300;
	else
		ent->client->enviro_framenum = level.framenum + 300;
}

//======================================================================

void	Use_Invulnerability (edict_t *ent, gitem_t *item)
{
	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, item->pickup_name);
	loadout_entry_ptr->amount--;

	if (ent->client->invincible_framenum > level.framenum)
		ent->client->invincible_framenum += 300;
	else
		ent->client->invincible_framenum = level.framenum + 300;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void	Use_Silencer (edict_t *ent, gitem_t *item)
{
	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, item->pickup_name);
	loadout_entry_ptr->amount--;
	ent->client->silencer_shots += 30;}

//======================================================================

bool Add_Ammo (edict_t *ent, gitem_t *item, int32_t count)
{
	int32_t		index;
	int32_t		max;

	loadout_entry_t* ammo_ptr = Loadout_GetItem(ent, item->pickup_name);

	if (ammo_ptr == NULL)
		ammo_ptr = Loadout_AddItem(ent, item->pickup_name, item->icon, loadout_entry_type_ammo, 0); // amount is added later

	if (!ent->client)
		return false;

	if (item->tag == AMMO_BULLETS)
		max = ent->client->pers.max_bullets;
	else if (item->tag == AMMO_SHELLS)
		max = ent->client->pers.max_shells;
	else if (item->tag == AMMO_ROCKETS)
		max = ent->client->pers.max_rockets;
	else if (item->tag == AMMO_GRENADES)
		max = ent->client->pers.max_grenades;
	else if (item->tag == AMMO_CELLS)
		max = ent->client->pers.max_cells;
	else if (item->tag == AMMO_SLUGS)
		max = ent->client->pers.max_slugs;
	else
		return false;

	index = ITEM_INDEX(item);

	if (ammo_ptr->amount == max)
		return false;

	ammo_ptr->amount += count;

	if (ammo_ptr->amount > max)
		ammo_ptr->amount = max;

	return true;
}

bool Pickup_Ammo (edict_t *ent, edict_t *other)
{
	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(other, ent->item->pickup_name);
	int32_t	oldcount;
	int32_t	count;
	bool	weapon;

	weapon = (ent->item->flags & IT_WEAPON);
	if ( (weapon) && ( (int32_t)gameflags->value & GF_INFINITE_AMMO ) )
		count = 1000;
	else if (ent->count)
		count = ent->count;
	else
		count = ent->item->quantity;

	if (loadout_entry_ptr != NULL)
	{
		oldcount = loadout_entry_ptr->amount;
	}
	else
	{
		oldcount = 0;
	}

	if (!Add_Ammo (other, ent->item, count))
		return false;

	if (weapon && !oldcount)
	{
		if (other->client->pers.weapon != ent->item)
			other->client->newweapon = ent->item;
	}

	if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
		SetRespawn (ent, 30);
	return true;
}

void Drop_Ammo (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;

	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, item->pickup_name);

	dropped = Drop_Item (ent, item);
	if (loadout_entry_ptr->amount >= item->quantity)
		dropped->count = item->quantity;
	else
		dropped->count = loadout_entry_ptr->amount;

	if (ent->client->pers.weapon && 
		ent->client->pers.weapon->tag == AMMO_GRENADES &&
		item->tag == AMMO_GRENADES &&
		loadout_entry_ptr->amount - dropped->count <= 0) {
		gi.cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		G_FreeEdict(dropped);
		return;
	}

	loadout_entry_ptr->amount -= dropped->count;

	// Remove the ammo if we don't have it
	if (loadout_entry_ptr->amount == 0)
	{
		Loadout_DeleteItem(ent, item->pickup_name);

		gi.WriteByte(svc_loadout_remove);
		gi.WriteString(item->pickup_name);
	}

}


//======================================================================

void MegaHealth_think (edict_t *self)
{
	if (self->owner->health > self->owner->max_health)
	{
		self->nextthink = level.time + 1;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & DROPPED_ITEM))
		SetRespawn (self, 20);
	else
		G_FreeEdict (self);
}

bool Pickup_Health (edict_t *ent, edict_t *other)
{
	if (!(ent->style & HEALTH_IGNORE_MAX))
		if (other->health >= other->max_health)
			return false;

	other->health += ent->count;

	if (!(ent->style & HEALTH_IGNORE_MAX))
	{
		if (other->health > other->max_health)
			other->health = other->max_health;
	}

	if (ent->style & HEALTH_TIMED)
	{
		ent->think = MegaHealth_think;
		ent->nextthink = level.time + 5;
		ent->owner = other;
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	else
	{
		if (!(ent->spawnflags & DROPPED_ITEM))
			SetRespawn (ent, 30);
	}

	return true;
}

//======================================================================

loadout_entry_t* GetCurrentArmor(edict_t* ent)
{
	if (!ent->client)
		return 0;

	loadout_entry_t* loadout_ptr_jacket = Loadout_GetItem(ent, "Jacket Armor");
	loadout_entry_t* loadout_ptr_combat = Loadout_GetItem(ent, "Combat Armor");
	loadout_entry_t* loadout_ptr_body = Loadout_GetItem(ent, "Body Armor");

	if (loadout_ptr_jacket == NULL
		|| loadout_ptr_combat == NULL
		|| loadout_ptr_body == NULL)
	{
		return 0;
	}

	if (loadout_ptr_jacket->amount > 0)
		return loadout_ptr_jacket;

	if (loadout_ptr_combat->amount > 0)
		return loadout_ptr_combat;

	if (loadout_ptr_body->amount > 0)
		return loadout_ptr_body;

	return 0;
}

bool Pickup_Armor (edict_t *ent, edict_t *other)
{
	loadout_entry_t*	loadout_ptr_old = GetCurrentArmor(other);
	gitem_t*			loadout_ptr_jacket = FindItem("Jacket Armor");
	gitem_t*			loadout_ptr_combat = FindItem("Combat Armor");
	loadout_entry_t*	loadout_ptr_new = Loadout_GetItem(other, ent->item->pickup_name);
	gitem_armor_t*		oldinfo;
	gitem_armor_t*		newinfo;
	int32_t				newcount;
	float				salvage;
	int32_t				salvagecount;

	// add new armor to loadout if it exists
	if (!loadout_ptr_new)
		loadout_ptr_new = Loadout_AddItem(other, ent->item->pickup_name, ent->item->icon, loadout_entry_type_armor, 0); // amount is set later

	// get info on new armor
	newinfo = (gitem_armor_t *)ent->item->info;

	// handle armor shards specially
	if (ent->item->tag == ARMOR_SHARD)
	{
		loadout_ptr_old->amount += 2;	
	}
	// if player has no armor, just use it
	else if (!loadout_ptr_old)
	{
		loadout_ptr_new->amount = newinfo->base_count;
	}

	// use the better armor
	else
	{
		// get info on old armor
		if (loadout_ptr_old == loadout_ptr_jacket)
			oldinfo = &jacketarmor_info;
		else if (loadout_ptr_old == loadout_ptr_combat)
			oldinfo = &combatarmor_info;
		else // (old_armor_index == body_armor_index)
			oldinfo = &bodyarmor_info;

		if (newinfo->normal_protection > oldinfo->normal_protection)
		{
			// calc new armor values
			salvage = oldinfo->normal_protection / newinfo->normal_protection;
			salvagecount = salvage * loadout_ptr_old->amount;
			newcount = newinfo->base_count + salvagecount;
			if (newcount > newinfo->max_count)
				newcount = newinfo->max_count;

			// zero count of old armor so it goes away
			loadout_ptr_old->amount = 0;

			// change armor to new item with computed value
			loadout_ptr_new->amount = newcount;
		}
		else
		{
			// calc new armor values
			salvage = newinfo->normal_protection / oldinfo->normal_protection;
			salvagecount = salvage * newinfo->base_count;
			newcount = loadout_ptr_old->amount + salvagecount;
			if (newcount > oldinfo->max_count)
				newcount = oldinfo->max_count;

			// if we're already maxed out then we don't need the new armor
			if (loadout_ptr_old->amount >= newcount)
				return false;

			// update current armor value
			loadout_ptr_old->amount = newcount;
		}
	}

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn (ent, 20);

	return true;
}

//======================================================================

int32_t GetCurrentPowerArmor (edict_t *ent)
{// Power screen, Power shield
	if (!ent->client)
		return POWER_ARMOR_NONE;

	loadout_entry_t* loadout_entry_power_screen = Loadout_GetItem(ent, "Power screen");
	loadout_entry_t* loadout_entry_power_shield = Loadout_GetItem(ent, "Power shield");

	if (!(ent->flags & FL_POWER_ARMOR))
		return POWER_ARMOR_NONE;

	if (loadout_entry_power_screen->amount > 0)
		return POWER_ARMOR_SHIELD;

	if (loadout_entry_power_shield->amount > 0)
		return POWER_ARMOR_SCREEN;

	return POWER_ARMOR_NONE;
}

void Use_PowerArmor (edict_t *ent, gitem_t *item)
{
	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, item->pickup_name);
	loadout_entry_t* loadout_entry_cells = Loadout_GetItem(ent, "cells");

	if (ent->flags & FL_POWER_ARMOR)
	{
		ent->flags &= ~FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
	}
	else
	{
		if (loadout_entry_cells->amount == 0);
		{
			gi.cprintf (ent, PRINT_HIGH, "No cells for power armor.\n");
			return;
		}
		ent->flags |= FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
	}
}

bool Pickup_PowerArmor (edict_t *ent, edict_t *other)
{
	loadout_entry_t*	loadout_entry_ptr = Loadout_GetItem(other, ent->item->pickup_name);

	if (!loadout_entry_ptr)
		loadout_entry_ptr = Loadout_AddItem(other, ent->item->pickup_name, ent->item->icon, loadout_entry_type_armor, 1);

	loadout_entry_ptr->amount++;

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn(ent, ent->item->quantity);
	// auto-use for DM only if we didn't already have one
	if (!loadout_entry_ptr)
		ent->item->use(other, ent->item);

	return true;
}

void Drop_PowerArmor (edict_t *ent, gitem_t *item)
{
	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, item->pickup_name);

	if (!loadout_entry_ptr)
	{
		Com_Printf("ERROR: Drop_PowerArmor: Tried to drop power armor we don't have???? Added to prevent a crash");
		loadout_entry_ptr = Loadout_AddItem(ent, item->pickup_name, item->icon, loadout_entry_type_armor, 1);
	}

	if ((ent->flags & FL_POWER_ARMOR) && (loadout_entry_ptr->amount == 1))
		Use_PowerArmor (ent, item);
	Drop_General (ent, item);
}

//======================================================================

/*
===============
Touch_Item
===============
*/
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	bool	taken;

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup
	if (!ent->item->pickup)
		return;		// not a grabbable item?

	taken = ent->item->pickup(ent, other);

	if (taken)
	{
		// flash the screen
		other->client->bonus_alpha = 0.25;	

		// show icon and name on status bar
		other->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent->item->icon);
		other->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS+ITEM_INDEX(ent->item);
		other->client->pickup_msg_time = level.time + 3.0;

		// change selected item
		if (ent->item->use)
			other->client->pers.selected_item = other->client->ps.stats[STAT_SELECTED_ITEM] = ITEM_INDEX(ent->item);

		if (ent->item->pickup == Pickup_Health)
		{
			if (ent->count == 2)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/s_health.wav"), 1, ATTN_NORM, 0);
			else if (2 < ent->count <= 10)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_NORM, 0);
			else if (10 < ent->count <= 25)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/l_health.wav"), 1, ATTN_NORM, 0);
			else // (ent->count == 100)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/m_health.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent->item->pickup_sound)
		{
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent->item->pickup_sound), 1, ATTN_NORM, 0);
		}
	}

	

	if (!(ent->spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets (ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;

	if ((ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
	{
		if (ent->flags & FL_RESPAWN)
			ent->flags &= ~FL_RESPAWN;
		else
			G_FreeEdict (ent);
	}
}

//======================================================================

static void drop_temp_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	Touch_Item (ent, other, plane, surf);
}

static void drop_make_touchable (edict_t *ent)
{
	ent->touch = Touch_Item;

	ent->nextthink = level.time + 29;
	ent->think = G_FreeEdict;
}

edict_t *Drop_Item (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	vec3_t	forward, right;
	vec3_t	offset;

	dropped = G_Spawn();

	dropped->classname = item->classname;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.effects = item->world_model_flags;
	dropped->s.renderfx = RF_GLOW;
	VectorSet (dropped->mins, -15, -15, -15);
	VectorSet (dropped->maxs, 15, 15, 15);
	gi.setmodel (dropped, dropped->item->world_model);
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;  
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	if (ent->client)
	{
		trace_t	trace;

		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource (ent->s.origin, offset, forward, right, dropped->s.origin);
		trace = gi.trace (ent->s.origin, dropped->mins, dropped->maxs,
			dropped->s.origin, ent, CONTENTS_SOLID);
		VectorCopy (trace.endpos, dropped->s.origin);
	}
	else
	{
		AngleVectors (ent->s.angles, forward, right, NULL);
		VectorCopy (ent->s.origin, dropped->s.origin);
	}

	VectorScale (forward, 100, dropped->velocity);
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1;

	gi.linkentity (dropped);

	return dropped;
}

void Use_Item (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->svflags &= ~SVF_NOCLIENT;
	ent->use = NULL;

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->touch = Touch_Item;
	}

	gi.linkentity (ent);
}

//======================================================================

/*
================
droptofloor
================
*/
void droptofloor (edict_t *ent)
{
	trace_t		tr;
	vec3_t		dest;
	float		*v;

	v = tv(-15,-15,-15);
	VectorCopy (v, ent->mins);
	v = tv(15,15,15);
	VectorCopy (v, ent->maxs);

	if (ent->model)
		gi.setmodel (ent, ent->model);
	else
		gi.setmodel (ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;  
	ent->touch = Touch_Item;

	v = tv(0,0,-128);
	VectorAdd (ent->s.origin, v, dest);

	tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		gi.dprintf ("droptofloor: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		if (ent == ent->teammaster)
		{
			ent->nextthink = level.time + FRAMETIME;
			ent->think = DoRespawn;
		}
	}

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
		ent->s.effects &= ~EF_ROTATE;
		ent->s.renderfx &= ~RF_GLOW;
	}

	if (ent->spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		ent->use = Use_Item;
	}

	gi.linkentity (ent);
}


/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem (gitem_t *it)
{
	char	*s, *start;
	char	data[MAX_QPATH];
	int		len;
	gitem_t	*ammo;

	if (!it)
		return;

	if (it->pickup_sound)
		gi.soundindex (it->pickup_sound);
	if (it->world_model)
		gi.modelindex (it->world_model);
	if (it->view_model)
		gi.modelindex (it->view_model);
	if (it->icon)
		gi.imageindex (it->icon);

	// parse everything for its ammo
	if (it->ammo && it->ammo[0])
	{
		ammo = FindItem (it->ammo);
		if (ammo != it)
			PrecacheItem (ammo);
	}

	// parse the space seperated precache string for other items
	s = it->precaches;
	if (!s || !s[0])
		return;

	while (*s)
	{
		start = s;
		while (*s && *s != ' ')
			s++;

		len = s-start;
		if (len >= MAX_QPATH || len < 5)
			gi.error ("PrecacheItem: %s has bad precache string", it->classname);
		memcpy (data, start, len);
		data[len] = 0;
		if (*s)
			s++;

		// determine type based on extension
		if (!strcmp(data+len-3, "md2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "sp2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "wav"))
			gi.soundindex (data);
		if (!strcmp(data+len-3, "tga"))
			gi.imageindex (data);
	}
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might 
be on an entity that hasn't spawned yet. 
============
*/
void SpawnItem (edict_t *ent, gitem_t *item)
{
	PrecacheItem (item);

	if (ent->spawnflags)
	{
		if (strcmp(ent->classname, "key_power_cube") != 0)
		{
			ent->spawnflags = 0;
			gi.dprintf("%s at %s has invalid spawnflags set\n", ent->classname, vtos(ent->s.origin));
		}
	}

	if ((int32_t)gameflags->value & GF_NO_ARMOR)
	{
		if (item->pickup == Pickup_Armor || item->pickup == Pickup_PowerArmor)
		{
			G_FreeEdict(ent);
			return;
		}
	}
	if ((int32_t)gameflags->value & GF_NO_ITEMS)
	{
		if (item->pickup == Pickup_Powerup)
		{
			G_FreeEdict(ent);
			return;
		}
	}
	if ((int32_t)gameflags->value & GF_NO_HEALTH)
	{
		if (item->pickup == Pickup_Health || item->pickup == Pickup_Adrenaline || item->pickup == Pickup_AncientHead)
		{
			G_FreeEdict(ent);
			return;
		}
	}
	if ((int32_t)gameflags->value & GF_INFINITE_AMMO)
	{
		if ((item->flags == IT_AMMO))
		{
			G_FreeEdict(ent);
			return;
		}
	}

	ent->item = item;
	ent->nextthink = level.time + 2 * FRAMETIME;    // items start after other solids
	ent->think = droptofloor;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW;
	if (ent->model)
		gi.modelindex (ent->model);
}

//======================================================================


gitem_t	itemlist[] = 
{
	{
		NULL
	},	// leave index 0 alone

	//
	// ARMOR
	//

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_body", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/body/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/i_bodyarmor",
/* pickup */	"Body Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&bodyarmor_info,
		ARMOR_BODY,
/* precache */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) 
*/
	{
		"item_armor_combat", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/combat/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/i_combatarmor",
/* pickup */	"Combat Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&combatarmor_info,
		ARMOR_COMBAT,
/* precache */ ""
	},

/*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_jacket", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/jacket/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/i_jacketarmor",
/* pickup */	"Jacket Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&jacketarmor_info,
		ARMOR_JACKET,
/* precache */ ""
	},

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) 
*/
	{
		"item_armor_shard", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar2_pkup.wav",
		"models/items/armor/shard/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/i_jacketarmor",
/* pickup */	"Armor Shard",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		ARMOR_SHARD,
/* precache */ ""
	},


/*QUAKED item_power_screen (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_power_screen", 
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/screen/tris.md2", EF_ROTATE, 
		NULL,
/* icon */		"pics/i_powerscreen",
/* pickup */	"Power Screen",
/* width */		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_power_shield (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_power_shield",
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/shield/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/i_powershield",
/* pickup */	"Power Shield",
/* width */		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
/* precache */ "misc/power2.wav misc/power1.wav"
	},


	//
	// WEAPONS 
	//

/* Weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		"weapon_blaster", 
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Blaster,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_blast/tris.md2",
/* icon */		"pics/w_blaster",
/* pickup */	"Blaster",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_BLASTER,
		NULL,
		0,
/* precache */ "weapons/blastf1a.wav misc/lasfly.wav"
	},

/*QUAKED Weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"wapon_shotgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Shotgun,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg/tris.md2",
/* icon */		"pics/w_shotgun",
/* pickup */	"Shotgun",
		0,
		1,
		"Shells",
		IT_WEAPON,
		WEAP_SHOTGUN,
		NULL,
		0,
/* precache */ "weapons/shotgf1b.wav weapons/shotgr1b.wav"
	},

/*QUAKED Weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_supershotgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_SuperShotgun,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg2/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg2/tris.md2",
/* icon */		"pics/w_sshotgun",
/* pickup */	"Super Shotgun",
		0,
		2,
		"Shells",
		IT_WEAPON,
		WEAP_SUPERSHOTGUN,
		NULL,
		0,
/* precache */ "weapons/sshotf1b.wav"
	},

/*QUAKED Weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_machinegun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Machinegun,
		"misc/w_pkup.wav",
		"models/weapons/g_machn/tris.md2", EF_ROTATE,
		"models/weapons/v_machn/tris.md2",
/* icon */		"pics/w_machinegun",
/* pickup */	"Machinegun",
		0,
		1,
		"Bullets",
		IT_WEAPON,
		WEAP_MACHINEGUN,
		NULL,
		0,
/* precache */ "weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav weapons/machgf5b.wav"
	},

/*QUAKED Weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_chaingun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Chaingun,
		"misc/w_pkup.wav",
		"models/weapons/g_chain/tris.md2", EF_ROTATE,
		"models/weapons/v_chain/tris.md2",
/* icon */		"pics/w_chaingun",
/* pickup */	"Chaingun",
		0,
		1,
		"Bullets",
		IT_WEAPON,
		WEAP_CHAINGUN,
		NULL,
		0,
/* precache */ "weapons/chngnu1a.wav weapons/chngnl1a.wav weapons/machgf3b.wav` weapons/chngnd1a.wav"
	},

/*QUAKED Ammo_Grenades (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_grenades",
		Pickup_Ammo,
		Use_Weapon,
		Drop_Ammo,
		Weapon_Grenade,
		"misc/am_pkup.wav",
		"models/items/ammo/grenades/medium/tris.md2", 0,
		"models/weapons/v_handgr/tris.md2",
/* icon */		"pics/a_grenades",
/* pickup */	"Grenades",
/* width */		3,
		5,
		"grenades",
		IT_AMMO|IT_WEAPON,
		WEAP_GRENADES,
		NULL,
		AMMO_GRENADES,
/* precache */ "weapons/hgrent1a.wav weapons/hgrena1b.wav weapons/hgrenc1b.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav "
	},

/*QUAKED Weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_grenadelauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_GrenadeLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_launch/tris.md2", EF_ROTATE,
		"models/weapons/v_launch/tris.md2",
/* icon */		"pics/w_glauncher",
/* pickup */	"Grenade Launcher",
		0,
		1,
		"Grenades",
		IT_WEAPON,
		WEAP_GRENADELAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/grenade/tris.md2 weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav"
	},

/*QUAKED Weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_rocketlauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_RocketLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_rocket/tris.md2", EF_ROTATE,
		"models/weapons/v_rocket/tris.md2",
/* icon */		"pics/w_rlauncher",
/* pickup */	"Rocket Launcher",
		0,
		1,
		"Rockets",
		IT_WEAPON,
		WEAP_ROCKETLAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/rocket/tris.md2 weapons/rockfly.wav weapons/rocklf1a.wav weapons/rocklr1b.wav models/objects/debris2/tris.md2"
	},

/*QUAKED Weapon_hyperblaster (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_hyperblaster", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_HyperBlaster,
		"misc/w_pkup.wav",
		"models/weapons/g_hyperb/tris.md2", EF_ROTATE,
		"models/weapons/v_hyperb/tris.md2",
/* icon */		"pics/w_hyperblaster",
/* pickup */	"HyperBlaster",
		0,
		1,
		"Cells",
		IT_WEAPON,
		WEAP_HYPERBLASTER,
		NULL,
		0,
/* precache */ "weapons/hyprbu1a.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav weapons/hyprbd1a.wav misc/lasfly.wav"
	},

/*QUAKED Weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_railgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Railgun,
		"misc/w_pkup.wav",
		"models/weapons/g_rail/tris.md2", EF_ROTATE,
		"models/weapons/v_rail/tris.md2",
/* icon */		"pics/w_railgun",
/* pickup */	"Railgun",
		0,
		1,
		"Slugs",
		IT_WEAPON,
		WEAP_RAILGUN,
		NULL,
		0,
/* precache */ "weapons/rg_hum.wav"
	},

	// 
	// Director team weapons start here
	//

	{
		"weapon_bamfuslicator",
			Pickup_Weapon,
			Use_Weapon,
			Drop_Weapon,
			Weapon_Bamfuslicator,
			"misc/w_pkup.wav",
			"models/weapons/g_bamfuslicator/tris.md2", EF_ROTATE,
			"models/weapons/v_bamfuslicator/tris.md2",
			/* icon */		"pics/w_bamfuslicator",
			/* pickup */	"Director - Bamfuslicator",
			0,
			0, // 0 = infinite
			NULL, // Also means infinite
			IT_WEAPON,
			WEAP_BAMFUSLICATOR,
			NULL,
			0,
			/* precache */ "weapons/sshotf1b.wav",
			team_director,
	},

	{
		"weapon_planfuslicator",
			Pickup_Weapon,
			Use_Weapon,
			Drop_Weapon,
			Weapon_Bamfuslicator,
			"misc/w_pkup.wav",
			"models/weapons/g_shotg2/tris.md2", EF_ROTATE,
			"models/weapons/v_shotg2/tris.md2",
			/* icon */		"pics/w_planfuslicator",
			/* pickup */	"Director - Planfuslicator",
			0,
			0, // 0 = infinite
			NULL, // Also means infinite
			IT_WEAPON,
			WEAP_PLANFUSLICATOR,
			NULL,
			0,
			/* precache */ "weapons/sshotf1b.wav",
			team_director,
	},
	{
		"weapon_tangfuslicator",
			Pickup_Weapon,
			Use_Weapon,
			Drop_Weapon,
			Weapon_Tangfuslicator,
			"misc/w_pkup.wav",
			"models/weapons/g_tangfuslicator/tris.md2", EF_ROTATE,
			"models/weapons/v_tangfuslicator/tris.md2",
			/* icon */		"pics/w_tangfuslicator",
			/* pickup */	"Director - Tangfuslicator",
			0,
			0, // 0 = infinite
			NULL, // Also means infinite
			IT_WEAPON,
			WEAP_TANGFUSLICATOR,
			NULL,
			0,
			/* precache */ "weapons/sshotf1b.wav",
			team_director,
	},


	//
	// AMMO ITEMS
	//

/*QUAKED Ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_shells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/shells/medium/tris.md2", 0,
		NULL,
/* icon */		"pics/a_shells",
/* pickup */	"Shells",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SHELLS,
/* precache */ ""
	},

/*QUAKED Ammo_Bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_bullets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/bullets/medium/tris.md2", 0,
		NULL,
/* icon */		"pics/a_bullets",
/* pickup */	"Bullets",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_BULLETS,
/* precache */ ""
	},

/*QUAKED Ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_cells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/cells/medium/tris.md2", 0,
		NULL,
/* icon */		"pics/a_cells",
/* pickup */	"Cells",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_CELLS,
/* precache */ ""
	},

/*QUAKED Ammo_Rockets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_rockets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/rockets/medium/tris.md2", 0,
		NULL,
/* icon */		"pics/a_rockets",
/* pickup */	"Rockets",
/* width */		3,
		5,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_ROCKETS,
/* precache */ ""
	},

/*QUAKED Ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_slugs",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/slugs/medium/tris.md2", 0,
		NULL,
/* icon */		"pics/a_slugs",
/* pickup */	"Slugs",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SLUGS,
/* precache */ ""
	},


	//
	// POWERUP ITEMS
	//
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_quad", 
		Pickup_Powerup,
		Use_Quad,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/quaddama/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/p_quad",
/* pickup */	"Quad Damage",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/damage.wav items/damage2.wav items/damage3.wav"
	},

/*QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_invulnerability",
		Pickup_Powerup,
		Use_Invulnerability,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/invulner/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/p_invulnerability",
/* pickup */	"Invulnerability",
/* width */		2,
		300,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/protect.wav items/protect2.wav items/protect4.wav"
	},

/*QUAKED item_silencer (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_silencer",
		Pickup_Powerup,
		Use_Silencer,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/silencer/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/p_silencer",
/* pickup */	"Silencer",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_breather (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_breather",
		Pickup_Powerup,
		Use_Breather,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/breather/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/p_rebreather",
/* pickup */	"Rebreather",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/airout.wav"
	},

/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_enviro",
		Pickup_Powerup,
		Use_Envirosuit,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/enviro/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/p_envirosuit",
/* pickup */	"Environment Suit",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/airout.wav"
	},

/*QUAKED item_ancient_head (.3 .3 1) (-16 -16 -16) (16 16 16)
Special item that gives +2 to maximum health
*/
	{
		"item_ancient_head",
		Pickup_AncientHead,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/c_head/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/i_fixme",
/* pickup */	"Ancient Head",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16)
gives +1 to maximum health
*/
	{
		"item_adrenaline",
		Pickup_Adrenaline,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/adrenal/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/p_adrenaline",
/* pickup */	"Adrenaline",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_bandolier (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_bandolier",
		Pickup_Bandolier,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/band/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/p_bandolier",
/* pickup */	"Bandolier",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_pack (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_pack",
		Pickup_Pack,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/pack/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"pics/i_pack",
/* pickup */	"Ammo Pack",
/* width */		2,
		180,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

	{
		NULL,
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		NULL, 0,
		NULL,
		/* icon */		"pics/i_health",
		/* pickup */	"Health",
		/* width */		3,
				0,
				NULL,
				0,
				0,
				NULL,
				0,
				/* precache */ "items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav"
	},

	// end of list marker
	{NULL}
};


/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health (edict_t *self)
{
	if (((int32_t)gameflags->value & GF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/medium/tris.md2";
	self->count = 10;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/n_health.wav");
}

/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_small (edict_t *self)
{
	if (((int32_t)gameflags->value & GF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/stimpack/tris.md2";
	self->count = 2;
	SpawnItem (self, FindItem ("Health"));
	self->style = HEALTH_IGNORE_MAX;
	gi.soundindex ("items/s_health.wav");
}

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_large (edict_t *self)
{
	if (((int32_t)gameflags->value & GF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/large/tris.md2";
	self->count = 25;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/l_health.wav");
}

// 50 health pack
void SP_item_health_super(edict_t* self)
{
	if (((int32_t)gameflags->value & GF_NO_HEALTH))
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/items/healing/large/tris.md2";
	self->count = 50;
	SpawnItem(self, FindItem("Health"));
	gi.soundindex("items/m_health.wav");
}

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_mega (edict_t *self)
{
	if (((int32_t)gameflags->value & GF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/mega_h/tris.md2";
	self->count = 100;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/m_health.wav");
	self->style = HEALTH_IGNORE_MAX|HEALTH_TIMED;
}


void InitItems (void)
{
	game.num_items = sizeof(itemlist)/sizeof(itemlist[0]) - 1;
}



/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames (void)
{
	int		i;
	gitem_t	*it;

	for (i=0 ; i<game.num_items ; i++)
	{
		it = &itemlist[i];
		gi.configstring (CS_ITEMS+i, it->pickup_name);
	}
}
