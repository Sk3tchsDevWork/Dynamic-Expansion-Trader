class CfgPatches
{
    class TravelingTrader
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data","DZ_Scripts"};
    };
};

class CfgMods
{
    class TravelingTrader
    {
        dir = "TravelingTrader";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "TravelingTrader";
        credits = "";
        author = "ChatGPT + Kurt";
        authorID = "0";
        version = "1.0.0";
        type = "mod";
        dependencies[] = {"World","Mission"};
        class defs
        {
            class worldScriptModule
            {
                files[] = {"TravelingTrader/scripts/4_World"};
            };
            class missionScriptModule
            {
                files[] = {"TravelingTrader/scripts/5_Mission"};
            };
        };
    };
};
