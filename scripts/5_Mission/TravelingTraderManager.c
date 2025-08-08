class TT_SpawnedSet
{
    ref array<Object> spawnedObjects;
    int markerId;
    EntityAI trader;

    void TT_SpawnedSet()
    {
        spawnedObjects = new array<Object>;
        markerId = -1;
        trader = null;
    }
}

class TravelingTraderManager
{
    private TT_Config m_Config;
    private bool m_Running;
    private ref Timer m_IntervalTimer;
    private ref Timer m_DespawnTimer;
    private string m_LastLocationId = "";
    private TT_SpawnedSet m_Active;

    void TravelingTraderManager()
    {
        m_Config = TT_Config.Load();
        m_IntervalTimer = new Timer(CALL_CATEGORY_SYSTEM);
        m_DespawnTimer  = new Timer(CALL_CATEGORY_SYSTEM);
        m_Active = null;
    }

    void Start()
    {
        if (!m_Config || !m_Config.enabled) return;
        if (m_Running) return;
        m_Running = true;
        m_IntervalTimer.Run(m_Config.interval_minutes * 60, this, "OnInterval", NULL, true);
    }

    void Stop()
    {
        m_Running = false;
        m_IntervalTimer.Stop();
        CleanupActive();
    }

    private void OnInterval()
    {
        if (!m_Config.enabled) return;
        if (m_Active) return;

        TT_Location loc = PickLocation();
        if (!loc) return;

        m_Active = new TT_SpawnedSet();

        if (loc.mapFile != "" && loc.mapFile != string.Empty)
        {
            SpawnMapComposition(loc.mapFile, m_Active.spawnedObjects);
        }

        vector pos = Vector(loc.position[0], loc.position[1], loc.position[2]);
        vector ypr = Vector(loc.orientation[0], loc.orientation[1], loc.orientation[2]);

        EntityAI trader = SpawnTrader(m_Config.market.traderClassName, pos, ypr);
        m_Active.trader = trader;

        ApplyLoadout(trader, m_Config.market.traderLoadout);

        RegisterTraderWithMarket(trader, m_Config.market.marketTraderID, m_Config.market.useExistingMarketProfile);

        m_Active.markerId = CreateServerMarker(m_Config.marker, loc.label, pos);

        m_DespawnTimer.Run(m_Config.duration_minutes * 60, this, "OnDespawn", NULL, false);

        m_LastLocationId = loc.id;
    }

    private void OnDespawn()
    {
        CleanupActive();
    }

    private void CleanupActive()
    {
        if (!m_Active) return;

        if (m_Active.markerId >= 0)
        {
            RemoveServerMarker(m_Active.markerId);
            m_Active.markerId = -1;
        }

        if (m_Active.trader)
        {
            GetGame().ObjectDelete(m_Active.trader);
            m_Active.trader = null;
        }

        foreach (Object o : m_Active.spawnedObjects)
        {
            if (o) GetGame().ObjectDelete(o);
        }
        m_Active.spawnedObjects.Clear();

        m_Active = null;
    }

    private TT_Location PickLocation()
    {
        if (!m_Config.locations || m_Config.locations.Count() == 0) return null;

        array<int> candidates = new array<int>();
        for (int i = 0; i < m_Config.locations.Count(); i++)
        {
            if (m_Config.rotation_avoid_last && m_Config.locations[i].id == m_LastLocationId)
                continue;
            candidates.Insert(i);
        }

        if (candidates.Count() == 0)
        {
            for (int j = 0; j < m_Config.locations.Count(); j++)
                candidates.Insert(j);
        }

        int pick = candidates.GetRandomElement();
        return m_Config.locations[pick];
    }

    private void SpawnMapComposition(string fileName, notnull array<Object> outSpawned)
    {
        string path = string.Format("$profile:TravelingTrader/maps/%1", fileName);
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

    private EntityAI SpawnTrader(string traderClassName, vector pos, vector ypr)
    {
        EntityAI npc = EntityAI.Cast(GetGame().CreateObjectEx(traderClassName, pos, ECE_ALIGN_TO_SURFACE));
        if (npc) npc.SetYawPitchRoll(ypr);
        return npc;
    }

    private void ApplyLoadout(EntityAI trader, notnull array<string> items)
    {
        if (!trader) return;
        foreach (string item : items)
        {
            trader.GetInventory().CreateInInventory(item);
        }
    }

    // ---- Expansion integration (adjust to your Expansion version) ----
    private void RegisterTraderWithMarket(EntityAI trader, string marketTraderID, bool useExistingProfile)
    {
        // Example pseudocode:
        // auto m = ExpansionMarketModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarketModule));
        // if (m) m.BindTraderEntityToProfile(trader, marketTraderID, useExistingProfile);
    }

    private int CreateServerMarker(TT_Marker m, string subtitle, vector pos)
    {
        // Example pseudocode:
        // auto markers = ExpansionMarkerModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarkerModule));
        // if (markers && m.showOnMap)
        //     return markers.CreateServerMarker(m.title, subtitle, pos, m.icon, m.colorARGB, m.is3D);
        return -1;
    }

    private void RemoveServerMarker(int id)
    {
        // Example pseudocode:
        // auto markers = ExpansionMarkerModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarkerModule));
        // if (markers) markers.RemoveServerMarker(id);
    }
}

// Bootstrap in mission server init
modded class MissionServer
{
    protected ref TravelingTraderManager m_TT;

    override void OnInit()
    {
        super.OnInit();
        m_TT = new TravelingTraderManager();
        m_TT.Start();
    }

    override void OnMissionFinish()
    {
        if (m_TT) m_TT.Stop();
        super.OnMissionFinish();
    }
}
