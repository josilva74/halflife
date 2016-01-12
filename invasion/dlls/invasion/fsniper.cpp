/******************************************
*                                         *
*	Fichier fsniper, par Julien			  *
*                                         *
*	code du fusil de snipe				  *
*										  *
******************************************/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum fsniper_e
{
	FSNIPER_LONGIDLE = 0,
	FSNIPER_IDLE1,
	FSNIPER_RELOAD,
	FSNIPER_DEPLOY,
	FSNIPER_FIRE1,
	FSNIPER_FIDGET,
	FSNIPER_ZOOM,
};



LINK_ENTITY_TO_CLASS(weapon_fsniper, CFSniper);

void CFSniper::Spawn()
{
	pev->classname = MAKE_STRING("weapon_fsniper");
	Precache();
	SET_MODEL(ENT(pev), "models/w_fsniper.mdl");
	m_iId = WEAPON_FSNIPER;

	m_iDefaultAmmo = FSNIPER_DEFAULT_GIVE;

	m_flZoomDelta = 0.0f;

	FallInit();// get ready to fall down.
}


void CFSniper::Precache(void)
{
	PRECACHE_MODEL("models/v_fsniper.mdl");
	PRECACHE_MODEL("models/w_fsniper.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL("models/sniper_shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_fsniperclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("weapons/fsniper_shoot1.wav");
	PRECACHE_SOUND("weapons/fsniper_eject1.wav");
	PRECACHE_SOUND("weapons/fsniper_eject2.wav");
	PRECACHE_SOUND("weapons/fsniper_lens_open.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usFSniper = PRECACHE_EVENT(1, "events/fsniper.sc");
}

int CFSniper::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "fsniper";
	p->iMaxAmmo1 = FSNIPER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = FSNIPER_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_FSNIPER;
	p->iWeight = FSNIPER_WEIGHT;

	return 1;
}

int CFSniper::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();

		m_pPlayer->TextAmmo(TA_SNIPER);

		return TRUE;
	}
	return FALSE;
}

BOOL CFSniper::Deploy()
{
	BOOL bResult = DefaultDeploy("models/v_fsniper.mdl", "models/p_9mmAR.mdl", FSNIPER_DEPLOY, "fsniper");

	if (bResult)
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 36 / 16.0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 36 / 16.0;

		m_flZoomDelta = 0.0f;
	}

	return bResult;
}


void CFSniper::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_8DEGREES);
	Vector vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(0, 0, 0), 8192, BULLET_PLAYER_SNIPER, 1, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usFSniper);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFSniper, 27 / 25.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 1, 0);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(52 / 25.0);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 52 / 25.0;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CFSniper::SecondaryAttack(void)
{
	if (m_pPlayer->pev->fov == 0)
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 45;
	}
	else if (m_flZoomDelta + 0.2 < UTIL_WeaponTimeBase())
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
		m_flZoomDelta = gpGlobals->time + 0.3;
		m_flNextSecondaryAttack = GetNextAttackDelay(0.3f);
	}
	else if (m_pPlayer->pev->fov > 10)
	{
		m_pPlayer->pev->fov -= 1.5f;
		m_pPlayer->m_iFOV = m_pPlayer->pev->fov;
	}

	// m_flNextSecondaryAttack = GetNextAttackDelay(0.3f);
}

void CFSniper::Holster(int skiplocal /*= 0*/)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if (m_pPlayer->pev->fov != 0)
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
}

void CFSniper::Reload(void)
{
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;

	if (m_pPlayer->ammo_fsniper <= 0)
		return;

	DefaultReload(FSNIPER_MAX_CLIP, FSNIPER_RELOAD, 69 / 19.0);
}


void CFSniper::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 5))
	{
	case 0:
	case 1:
	case 2:
		iAnim = FSNIPER_LONGIDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 25 / 4.0;
		break;

	default:
	case 3:
	case 4:
		iAnim = FSNIPER_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 24 / 9.0;
		break;

	case 5:
		iAnim = FSNIPER_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 50 / 13.0;
		break;
	}

	SendWeaponAnim(iAnim);
}

class CFSniperAmmoClip : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_fsniperclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_fsniperclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_FSNIPERCLIP_GIVE, "fsniper", FSNIPER_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_fsniper, CFSniperAmmoClip);