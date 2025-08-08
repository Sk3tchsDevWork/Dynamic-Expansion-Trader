ref RTTConfig m_RTTConfig;
static const string RTT_CONFIG_FOLDER = "$profile:RandomTeleportTrader/";
static const string RTT_CONFIG_FILENAME = "config.json";

class RTTMarkerCfg
{
	string Title = "Traveling Trader";
	string Icon = "Deliver";        // Expansion marker icon name
	int ColorARGB = 0xFFFF9900;     // ARGB
	bool Is3D = false;
	bool ShowOnMap = true;
}

class RTTMarketCfg
{
	string TraderClassName = "ExpansionTraderDenis.Weapons"; // Expansion NPC class to spawn
	string MarketTraderID = "TT_TRAVELING";                  // Existing market profile ID to bind
	bool   UseExistingMarketProfile = true;                  // true = reuse profile; false = attempt create (if you implement it)
}

class RTTraderDataCfg
{
	protected int ID;
	protected int MoveTime;              // interval between spawns (ms) – same as your current behavior
	protected string MoveMessage;
	protected ref array<string> Position;
	protected ref array<string> Orientation;

	void RTTraderDataCfg()
	{
		Position = new array<string>;
		Orientation = new array<string>;
	}
    
	int GetID() { return ID; };
	int GetMoveTime() { return MoveTime; };
	string GetMoveMessage() { return MoveMessage; };
	
	ref array<vector> GetPosition()
	{
		array<vector> rttpos = new array<vector>;
		for (int rti = 0; rti < Position.Count(); rti++)
			rttpos.Insert(Position.Get(rti).ToVector());
		return rttpos;
	};

	ref array<vector> GetOrientation()
	{
		array<vector> rttori = new array<vector>;
		for (int rti = 0; rti < Orientation.Count(); rti++)
			rttori.Insert(Orientation.Get(rti).ToVector());
		return rttori;
	};
}

class RTTConfigData
{
	// NEW:
	int DurationMinutes = 30;         // How long the trader stays before being removed
	ref RTTMarketCfg Market;
	ref RTTMarkerCfg Marker;

	// Existing:
	protected ref array<RTTraderDataCfg> RTTraderData;
	
	void RTTConfigData()
	{
		RTTraderData = new array<RTTraderDataCfg>;
		Market = new RTTMarketCfg();
		Marker = new RTTMarkerCfg();
	};

	ref array<RTTraderDataCfg> GetRTTraderData() { return RTTraderData; };
}

class RTTConfig
{
	protected static ref RTTConfigData m_RTTConfigData;
	
	void RTTConfig() { LoadConfig(); };
	
	RTTConfigData GetConfigData()
	{
		if (!m_RTTConfigData) LoadConfig();
		return m_RTTConfigData;
	};
	
	protected void LoadConfig()
	{
		if (!FileExist(RTT_CONFIG_FOLDER)) MakeDirectory(RTT_CONFIG_FOLDER);
		
		m_RTTConfigData = new RTTConfigData();
		
		if (FileExist(RTT_CONFIG_FOLDER + RTT_CONFIG_FILENAME))
			JsonFileLoader<RTTConfigData>.JsonLoadFile(RTT_CONFIG_FOLDER + RTT_CONFIG_FILENAME, m_RTTConfigData);
		else
			CreateDefaultConfig();

		// Back-compat defaults if older config lacks new fields
		if (!m_RTTConfigData.Market) m_RTTConfigData.Market = new RTTMarketCfg();
		if (!m_RTTConfigData.Marker) m_RTTConfigData.Marker = new RTTMarkerCfg();
		if (m_RTTConfigData.DurationMinutes <= 0) m_RTTConfigData.DurationMinutes = 30;
	};
	
	protected void CreateDefaultConfig()
	{
		m_RTTConfigData = new RTTConfigData();
		JsonFileLoader<RTTConfigData>.JsonSaveFile(RTT_CONFIG_FOLDER + RTT_CONFIG_FILENAME, m_RTTConfigData);
	};
};

ref RTTConfig GetRTTConfig()
{
	if(!m_RTTConfig) m_RTTConfig = new RTTConfig;
	return m_RTTConfig;
};
