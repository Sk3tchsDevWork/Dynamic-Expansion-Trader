modded class MissionServer
{
	RTTConfigData m_RTTConfigData;
	ref array<RTTraderDataCfg> m_RTTraderData;

	// NEW: per-trader state
	ref array<bool> m_RTT_Active;
	ref array<EntityAI> m_RTT_TraderEntity;
	ref array<int> m_RTT_MarkerId;
	ref array<ref Timer> m_RTT_DespawnTimer;

	void MissionServer()
	{
		GetRTTConfig();
	}

	override void OnInit()
	{		
		super.OnInit();

		m_RTTConfigData = GetRTTConfig().GetConfigData();
		m_RTTraderData = m_RTTConfigData.GetRTTraderData();

		// init per-trader arrays
		int n = m_RTTraderData.Count();
		m_RTT_Active = new array<bool>;
		m_RTT_TraderEntity = new array<EntityAI>;
		m_RTT_MarkerId = new array<int>;
		m_RTT_DespawnTimer = new array<ref Timer>;
		for (int k = 0; k < n; k++)
		{
			m_RTT_Active.Insert(false);
			m_RTT_TraderEntity.Insert(null);
			m_RTT_MarkerId.Insert(-1);
			m_RTT_DespawnTimer.Insert(new Timer(CALL_CATEGORY_SYSTEM));
		}

		// keep your existing per-trader timers
		for (int it = 0; it < n; it++)
		{
			int time = m_RTTraderData.Get(it).GetMoveTime();
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RandomTrader, time, true, it);
		}
	}

	// ---- Expansion helpers ----
	private void RTT_RegisterTraderWithExpansion(EntityAI trader)
	{
		// Adjust to your Expansion version if needed:
		// auto market = ExpansionMarketModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarketModule));
		// if (market)
		//     market.BindTraderEntityToProfile(trader, m_RTTConfigData.Market.MarketTraderID, m_RTTConfigData.Market.UseExistingMarketProfile);
	}

	private int RTT_CreateServerMarker(vector pos, string subtitle)
	{
		int id = -1;
		// auto markers = ExpansionMarkerModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarkerModule));
		// if (markers && m_RTTConfigData.Marker.ShowOnMap)
		//     id = markers.CreateServerMarker(m_RTTConfigData.Marker.Title, subtitle, pos, m_RTTConfigData.Marker.Icon, m_RTTConfigData.Marker.ColorARGB, m_RTTConfigData.Marker.Is3D);
		return id;
	}

	private void RTT_RemoveServerMarker(int id)
	{
		// auto markers = ExpansionMarkerModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarkerModule));
		// if (markers && id >= 0) markers.RemoveServerMarker(id);
	}

	// ---- spawn/despawn ----
	private void RTT_SpawnExpansionTraderAt(int traderIdx, vector pos, vector ori, string moveMsg)
	{
		// Spawn Expansion Trader NPC
		EntityAI npc = EntityAI.Cast(GetGame().CreateObjectEx(m_RTTConfigData.Market.TraderClassName, pos, ECE_ALIGN_TO_SURFACE));
		if (npc)
		{
			npc.SetYawPitchRoll(ori);
			RTT_RegisterTraderWithExpansion(npc);

			// Marker
			int mk = RTT_CreateServerMarker(pos, "");

			// Store state
			m_RTT_TraderEntity[traderIdx] = npc;
			m_RTT_MarkerId[traderIdx] = mk;
			m_RTT_Active[traderIdx] = true;

			// Notify players (reuse your message broadcast)
			for (int j = 0; j < m_Players.Count(); j++)
			{
				PlayerBase player = PlayerBase.Cast(m_Players.Get(j));
				if (!player) continue;
				player.m_Trader_RecievedAllData = false; // keep your existing flag path
				Param1<string> m_GlobalMessage = new Param1<string>(moveMsg);
				GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, m_GlobalMessage, true, player.GetIdentity());
			}

			// Schedule despawn after DurationMinutes
			int ms = m_RTTConfigData.DurationMinutes * 60 * 1000;
			m_RTT_DespawnTimer[traderIdx].Run(ms / 1000.0, this, "RTT_DespawnExpansionTrader", new Param1<int>(traderIdx), false);
		}
	}

	void RTT_DespawnExpansionTrader(Param1<int> p)
	{
		int traderIdx = p.param1;

		// Remove marker
		if (m_RTT_MarkerId[traderIdx] >= 0)
		{
			RTT_RemoveServerMarker(m_RTT_MarkerId[traderIdx]);
			m_RTT_MarkerId[traderIdx] = -1;
		}

		// Remove NPC
		EntityAI npc = m_RTT_TraderEntity[traderIdx];
		if (npc)
		{
			GetGame().ObjectDelete(npc);
			m_RTT_TraderEntity[traderIdx] = null;
		}

		// Mark inactive so the next MoveTime tick can spawn a fresh one
		m_RTT_Active[traderIdx] = false;
	}

	// ---- your existing tick; now spawns Expansion trader if not active ----
	void RandomTrader(int traderIdentifier)
	{
		RTTraderDataCfg trader = m_RTTraderData.Get(traderIdentifier);
		string moveMsg = trader.GetMoveMessage();
		ref array<vector> positions = trader.GetPosition();
		ref array<vector> orientations = trader.GetOrientation();

		int count = positions.Count();
		if (count == 0) return;

		int pick = Math.RandomInt(0, count);
		vector pos = positions.Get(pick);
		vector ori = orientations.Get(pick);

		// If a trader is still active (within DurationMinutes window), do nothing this tick.
		if (m_RTT_Active[traderIdentifier])
			return;

		// Spawn new Expansion trader for the window
		RTT_SpawnExpansionTraderAt(traderIdentifier, pos, ori, moveMsg);
	}

	// Keep your existing sync behavior (unchanged)
	void UpdateToPlayerTraderData(PlayerBase player)
	{
		for (int it = 0; it < m_RTTraderData.Count(); it++)
		{
			RTTraderDataCfg traderData = m_RTTraderData.Get(it);
			// you previously synced m_Trader_TraderPositions[...] into player flags here.
			// That path belongs to the other trader mod; keeping the call to ensure no NPEs elsewhere.
			player.m_Trader_RecievedAllData = false;
		}
	}

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		if (player) UpdateToPlayerTraderData(player);
		super.InvokeOnConnect(player, identity);
	}

	override void OnClientRespawnEvent(PlayerIdentity identity, PlayerBase player)
	{
		if (player) UpdateToPlayerTraderData(player);
		super.OnClientRespawnEvent(identity, player);
	}

	void OnClientReconnectEvent(PlayerIdentity identity, PlayerBase player)
	{
		if (player) UpdateToPlayerTraderData(player);
		super.OnClientReconnectEvent(identity, player);
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		for (int j = 0; j < m_Players.Count(); j++)
		{
			PlayerBase player = PlayerBase.Cast(m_Players.Get(j));
			if (!player) continue;
			if (!player.m_Trader_RecievedAllData)
				sendTraderDataToPlayer(player);
		}
	}
}
