
modded class MissionServer
{
	RTTConfigData m_RTTConfigData;
	ref array<RTTraderDataCfg> m_RTTraderData;

	ref array<bool> m_RTT_Active;
	ref array<EntityAI> m_RTT_TraderEntity;
	ref array<int> m_RTT_MarkerId;
	ref array<ref Timer> m_RTT_DespawnTimer;
	ref array<ref array<Object>> m_RTT_SpawnedObjects;

	void MissionServer()
	{
		GetRTTConfig();
	}

	override void OnInit()
	{		
		super.OnInit();

		m_RTTConfigData = GetRTTConfig().GetConfigData();
		m_RTTraderData = m_RTTConfigData.GetRTTraderData();

		int n = m_RTTraderData.Count();
		m_RTT_Active = new array<bool>;
		m_RTT_TraderEntity = new array<EntityAI>;
		m_RTT_MarkerId = new array<int>;
		m_RTT_DespawnTimer = new array<ref Timer>;
		m_RTT_SpawnedObjects = new array<ref array<Object>>;

		for (int k = 0; k < n; k++)
		{
			m_RTT_Active.Insert(false);
			m_RTT_TraderEntity.Insert(null);
			m_RTT_MarkerId.Insert(-1);
			m_RTT_DespawnTimer.Insert(new Timer(CALL_CATEGORY_SYSTEM));
			m_RTT_SpawnedObjects.Insert(new array<Object>);
		}

		for (int it = 0; it < n; it++)
		{
			int time = m_RTTraderData.Get(it).GetMoveTime();
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RandomTrader, time, true, it);
		}
	}

	private void RTT_RegisterTraderWithExpansion(EntityAI trader)
	{
		auto market = ExpansionMarketModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarketModule));
		if (market)
		{
			market.BindTraderEntityToProfile(trader, m_RTTConfigData.Market.MarketTraderID, m_RTTConfigData.Market.UseExistingMarketProfile);
		}
	}

	private int RTT_CreateServerMarker(vector pos, string subtitle)
	{
		int id = -1;
		auto markers = ExpansionMarkerModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarkerModule));
		if (markers && m_RTTConfigData.Marker.ShowOnMap)
		{
			id = markers.CreateServerMarker(m_RTTConfigData.Marker.Title, subtitle, pos, m_RTTConfigData.Marker.Icon, m_RTTConfigData.Marker.ColorARGB, m_RTTConfigData.Marker.Is3D);
		}
		return id;
	}

	private void RTT_RemoveServerMarker(int id)
	{
		auto markers = ExpansionMarkerModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarkerModule));
		if (markers && id >= 0)
			markers.RemoveServerMarker(id);
	}

	private string RTT_MapPath(string mapFile)
	{
		string profilePath = RTT_CONFIG_FOLDER + RTT_MAPS_SUBFOLDER + mapFile;
		if (FileExist(profilePath)) return profilePath;
		string modPath = string.Format("$mission:RTT/maps/%1", mapFile);
		return modPath;
	}

	private void RTT_SpawnMapComposition(string mapFile, notnull array<Object> outSpawned)
	{
		if (mapFile == "" || mapFile == string.Empty) return;
		string path = RTT_MapPath(mapFile);
		FileHandle fh = OpenFile(path, FileMode.READ);
		if (!fh) return;

		string line;
		while (FGets(fh, line) > 0)
		{
			line.TrimInPlace();
			if (line == "" || line[0] == "#".ToAscii()) continue;

			TStringArray tok = new TStringArray;
			line.Split(" ", tok);

			if (tok.Count() < 7) continue;

			string cls = tok[0];
			float px = tok[1].ToFloat();
			float py = tok[2].ToFloat();
			float pz = tok[3].ToFloat();
			float yaw = tok[4].ToFloat();
			float pitch = tok[5].ToFloat();
			float roll = tok[6].ToFloat();

			vector pos = Vector(px, py, pz);
			vector ypr = Vector(yaw, pitch, roll);

			Object obj = GetGame().CreateObjectEx(cls, pos, ECE_PLACE_ON_SURFACE | ECE_KEEPHEIGHT);
			if (obj)
			{
				obj.SetYawPitchRoll(ypr);
				outSpawned.Insert(obj);
			}
		}
		CloseFile(fh);
	}

	private void RTT_SpawnExpansionTraderAt(int traderIdx, int locIdx, vector pos, vector ori, string moveMsg)
	{
		ref array<Object> spawned = m_RTT_SpawnedObjects[traderIdx];
		spawned.Clear();

		ref array<string> mapFiles = m_RTTraderData[traderIdx].GetMapFiles();
		string perLocMap = "";
		if (mapFiles && mapFiles.Count() > locIdx) perLocMap = mapFiles.Get(locIdx);
		string mapToLoad = perLocMap;
		if (mapToLoad == "" || mapToLoad == string.Empty) mapToLoad = m_RTTConfigData.DefaultMapFile;
		if (mapToLoad != "" && mapToLoad != string.Empty)
		{
			RTT_SpawnMapComposition(mapToLoad, spawned);
		}

		EntityAI npc = EntityAI.Cast(GetGame().CreateObjectEx(m_RTTConfigData.Market.TraderClassName, pos, ECE_ALIGN_TO_SURFACE));
		if (npc)
		{
			npc.SetYawPitchRoll(ori);
			RTT_RegisterTraderWithExpansion(npc);

			int mk = RTT_CreateServerMarker(pos, "");

			m_RTT_TraderEntity[traderIdx] = npc;
			m_RTT_MarkerId[traderIdx] = mk;
			m_RTT_Active[traderIdx] = true;

			for (int j = 0; j < m_Players.Count(); j++)
			{
				PlayerBase player = PlayerBase.Cast(m_Players.Get(j));
				if (!player) continue;
				player.m_Trader_RecievedAllData = false;
				Param1<string> m_GlobalMessage = new Param1<string>(moveMsg);
				GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, m_GlobalMessage, true, player.GetIdentity());
			}

			int ms = m_RTTConfigData.DurationMinutes * 60 * 1000;
			m_RTT_DespawnTimer[traderIdx].Run(ms / 1000.0, this, "RTT_DespawnExpansionTrader", new Param1<int>(traderIdx), false);
		}
	}

	void RTT_DespawnExpansionTrader(Param1<int> p)
	{
		int traderIdx = p.param1;

		if (m_RTT_MarkerId[traderIdx] >= 0)
		{
			RTT_RemoveServerMarker(m_RTT_MarkerId[traderIdx]);
			m_RTT_MarkerId[traderIdx] = -1;
		}

		EntityAI npc = m_RTT_TraderEntity[traderIdx];
		if (npc)
		{
			GetGame().ObjectDelete(npc);
			m_RTT_TraderEntity[traderIdx] = null;
		}

		ref array<Object> spawned = m_RTT_SpawnedObjects[traderIdx];
		for (int i = 0; i < spawned.Count(); i++)
		{
			if (spawned[i]) GetGame().ObjectDelete(spawned[i]);
		}
		spawned.Clear();

		m_RTT_Active[traderIdx] = false;
	}

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

		if (m_RTT_Active[traderIdentifier])
			return;

		RTT_SpawnExpansionTraderAt(traderIdentifier, pick, pos, ori, moveMsg);
	}

	void UpdateToPlayerTraderData(PlayerBase player)
	{
		for (int it = 0; it < m_RTTraderData.Count(); it++)
		{
			RTTraderDataCfg traderData = m_RTTraderData.Get(it);
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
