class TT_Location
{
    string id;
    string label;
    ref array<float> position;
    ref array<float> orientation;
    string mapFile;
    void TT_Location()
    {
        position = new array<float>;
        orientation = new array<float>;
    }
}

class TT_Marker
{
    string title;
    string icon;
    int colorARGB;
    bool is3D;
    bool showOnMap;
}

class TT_Market
{
    string traderClassName;
    string traderFaction;
    ref array<string> traderLoadout;
    string marketTraderID;
    bool useExistingMarketProfile;
    void TT_Market()
    {
        traderLoadout = new array<string>;
    }
}

class TT_Config
{
    bool enabled = true;
    int interval_minutes = 45;
    int duration_minutes = 30;
    ref TT_Market market;
    ref TT_Marker marker;
    ref array<TT_Location> locations;
    bool rotation_avoid_last = true;

    void TT_Config()
    {
        market = new TT_Market();
        marker = new TT_Marker();
        locations = new array<TT_Location>;
    }

    static string Path()
    {
        return "$profile:TravelingTrader/TravelingTraderConfig.json";
    }

    static TT_Config Load()
    {
        TT_Config cfg;
        if (!FileExist("$profile:TravelingTrader"))
            MakeDirectory("$profile:TravelingTrader");

        if (FileExist(Path()))
        {
            JsonFileLoader<TT_Config>.JsonLoadFile(Path(), cfg);
        }
        else
        {
            cfg = new TT_Config();
            JsonFileLoader<TT_Config>.JsonSaveFile(Path(), cfg);
        }
        return cfg;
    }
}
