# Traveling Trader (Expansion Market)

Spawns a roaming Expansion Market trader every **interval_minutes** at a random configured location, loads optional **.map** compositions, creates an **Expansion** map marker, and **cleans everything** after **duration_minutes**.

- Map-agnostic: locations are in a JSON under the server profile.
- .map files supported (simple parser included).
- Marker + assets are removed after despawn.
- Avoids repeating the last location.

## Install
1. Pack this folder as a PBO (or drop as a local mod in server).  
2. Start the server with `-mod=@TravelingTrader` (or include in your server preset).
3. On first run, the mod creates `$profile:TravelingTrader/TravelingTraderConfig.json` if missing.
4. Edit the JSON to set locations, trader class, etc.
5. Make sure Expansion is loaded if you enable markers.

> Note: Marker/Market hooks are left behind a thin wrapper. If your Expansion version uses different API names, adjust the two methods in **TravelingTraderManager.c**:
> - `CreateServerMarker(...)` / `RemoveServerMarker(...)`
> - `RegisterTraderWithMarket(...)`

## Config path
`$profile:TravelingTrader/TravelingTraderConfig.json`

## Example JSON
See `config/TravelingTraderConfig.json` in this package.

## .map files
Place them in `$profile:TravelingTrader/maps/` **or** copy examples from this mod's `maps/` into the profile path.  
The included loader expects each non-comment line as:
```
ClassName posX posY posZ yaw pitch roll
```

## Server notes
- This is script-only; no client assets.  
- Cleans trader, composition objects, and marker reliably on timer or on mission end.

## Troubleshooting
- **No marker appears**: verify Expansion is loaded and adjust the marker API calls in `CreateServerMarker/RemoveServerMarker` to match your Expansion version.  
- **Trader doesn't open market**: ensure `traderClassName` is a valid Expansion trader NPC and map it to your market profile via `RegisterTraderWithMarket`.  
- **Nothing spawns**: check `enabled`, ensure at least one location, and watch server logs for errors.
