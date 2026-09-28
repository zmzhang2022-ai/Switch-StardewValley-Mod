#pragma once

#include <array>
#include <cstdint>
#include <limits>

// Version-locked offsets for Stardew Valley 1.6.15.3 (Nintendo Switch).
// Build ID: A5C617C14A7F3F6620B3BC8136965A4822D32B9C
// Values are offsets from the ASLR base of the game's main NSO.
namespace StardewValley::Offsets {

inline constexpr char GameVersion[] = "1.6.15.3";
inline constexpr std::array<std::uint8_t, 20> BuildId{
    0xA5, 0xC6, 0x17, 0xC1, 0x4A, 0x7F, 0x3F, 0x66, 0x20, 0xB3,
    0xBC, 0x81, 0x36, 0x96, 0x5A, 0x48, 0x22, 0xD3, 0x2B, 0x9C,
};

inline constexpr std::uintptr_t kUnknown =
    std::numeric_limits<std::uintptr_t>::max();

// Verified from Microsoft.Xna.Framework.Game's Brute/AOT method metadata and
// its direct callers. ABI: void Game::Tick(Game* this).
inline constexpr std::uintptr_t MonoGameTick = 0x0007F890;

// DoUpdate is a valid method body, but normal Tick inlines the corresponding
// update path and does not directly call it. It is retained for analysis only.
inline constexpr std::uintptr_t MonoGameDoUpdate = 0x00080280;

// Verified static Game1/GameLocation getters. The first two are static getters;
// GameLocation.get_Objects is a simple instance getter.
inline constexpr std::uintptr_t Game1GetPlayer = 0x0139C170;
// Farmer.UniqueMultiplayerID getter, used by the native
// Chest.GetItemsForPlayer wrapper below.  The getter body reads the NetLong
// at Farmer+0x320 and returns its value in X0.
inline constexpr std::uintptr_t FarmerGetUniqueMultiplayerId = 0x013180C0;
// Game1.get_locations() returns the live List<GameLocation> directly.
// The metadata initializer contains a field-record offset, not this list.
inline constexpr std::uintptr_t Game1GetLocations = 0x0139C3A0;
inline constexpr std::uintptr_t Game1GetCurrentLocation = 0x0139C3C0;
inline constexpr std::uintptr_t GameLocationGetObjects = 0x01425D60;

// Skull Cavern elevator support. These bodies and ABIs were recovered from
// the exact Switch AOT image named above; none of them are PC/SMAPI offsets.
// Game1.getLocationFromName(string) is the one-argument wrapper which passes
// false to the two-argument implementation.
inline constexpr std::uintptr_t Game1GetLocationFromName = 0x013C2740;
// ABI: void Game1.enterMine(int level, Nullable<bool>* forceDescending).
inline constexpr std::uintptr_t Game1EnterMine = 0x013D6C00;
// ABI: void Game1.warpFarmer(string locationName, int x, int y,
//                           int facingDirection, bool isStructure).
inline constexpr std::uintptr_t Game1WarpFarmer = 0x013C4280;
// Static Game1.exitActiveMenu(); called by the original MineElevatorMenu at
// main+0x01744120/0x01744170 after selecting a destination.
inline constexpr std::uintptr_t Game1ExitActiveMenu = 0x013B25D0;
// Returns zero outside MineShaft, otherwise MineShaft.mineLevel.
inline constexpr std::uintptr_t Game1GetCurrentMineLevel = 0x0139FA70;
// MineShaft.lowestLevelReached, used by the vanilla MineElevatorMenu.
inline constexpr std::uintptr_t MineShaftLowestLevelReached = 0x015DAD00;
inline constexpr std::uintptr_t MineShaftGetMineLevel = 0x015DAE50;
// MineShaft.prepareElevator(MineShaft* this).
inline constexpr std::uintptr_t MineShaftPrepareElevator = 0x015F0E70;
// GameLocation helper used by the original prepareElevator implementation.
// ABI: packed Point FindTile(location, int tileIndex, string layer,
//                            string tileSheetOrNull). A missing tile returns
// (-1,-1), packed as two signed 32-bit values.
inline constexpr std::uintptr_t GameLocationFindTile = 0x01AF9FA0;
// ABI: Tile* setMapTile(location, int x, int y, int tileIndex,
//                       string layer, string tileSheetId,
//                       string actionOrNull, bool copyProperties).
inline constexpr std::uintptr_t GameLocationSetMapTile = 0x0142B430;
// ABI: void setTileProperty(location, int x, int y, string layer,
//                           string propertyName, string propertyValue).
inline constexpr std::uintptr_t GameLocationSetTileProperty = 0x0142D5C0;
// Non-throwing map tile-sheet lookup used internally by Map.GetTileSheet.
// The public body at main+0x012EE1D0 throws when the ID is absent (as it is
// for "mine" in the SkullCave lobby). At main+0x012EE204 it calls this body
// and only enters the exception path when the returned pointer is null.
inline constexpr std::uintptr_t MapTryGetTileSheet = 0x01DA1F10;
// TileSheet(string id, Map map, string imageSource, Size sheetSize,
//           Size tileSize). Callers at main+0x01551410..0x01551424 prove the
// AArch64 argument order; passing null in X0 asks the managed body to allocate.
inline constexpr std::uintptr_t TileSheetConstructor = 0x01DACEC0;
// Map.AddTileSheet(TileSheet). Its registered AddTileSheet invoker at
// main+0x077A0780 calls this body with X0=Map and X1=TileSheet.
inline constexpr std::uintptr_t MapAddTileSheet = 0x01DA1FC0;
inline constexpr std::uintptr_t GameLocationMap = 0x98;

// xTile's Map.GetTileSheet implementation above also proves:
// Map+0x28 -> Collection<TileSheet>, TileSheet+0x10 -> inherited Id.
// get_ImageSource's registered invoker at main+0x077A65D0 branches to
// main+0x01DACFF0, which reads TileSheet+0x30. String.Equals is the exact
// comparison body called by Map.GetTileSheet at main+0x01DA1F7C.
inline constexpr std::uintptr_t SystemStringEquals = 0x003878F0;
inline constexpr std::uintptr_t MapTileSheets = 0x28;
inline constexpr std::uintptr_t TileSheetId = 0x10;
inline constexpr std::uintptr_t TileSheetImageSource = 0x30;

// Vanilla MineElevatorMenu methods. The constructor calls
// min(lowestLevelReached, 120); the native mod supplies a relative Skull
// Cavern depth only while this constructor is running.
inline constexpr std::uintptr_t MineElevatorMenuConstructor = 0x01743AE0;
inline constexpr std::uintptr_t MineElevatorMenuReceiveLeftClick = 0x01744000;
// MineElevatorMenu.elevators is read by the original click/draw bodies at
// main+0x01744080 and main+0x0174443C. Each ClickableComponent's public name
// field is read at +0x20 and converted to the destination level.
inline constexpr std::uintptr_t MineElevatorMenuElevators = 0x68;
inline constexpr std::uintptr_t ClickableComponentName = 0x20;
inline constexpr std::uintptr_t ClickableComponentBounds = 0x10;
inline constexpr std::uintptr_t SystemInt32ToString = 0x002025E0;

// Brute/AOT method-registration invokers for receiveGamePadButton and
// receiveScrollWheelAction dispatch through these virtual slots. The concrete
// function pointers are deliberately read from the live MineElevatorMenu
// object; no unverified direct function address is assumed.
inline constexpr std::uintptr_t MenuReceiveGamePadButtonSlot = 0x60;
inline constexpr std::uintptr_t MenuReceiveGamePadButtonAdjust = 0x68;
inline constexpr std::uintptr_t MenuReceiveScrollWheelSlot = 0x210;
inline constexpr std::uintptr_t MenuReceiveScrollWheelAdjust = 0x218;
inline constexpr std::uintptr_t MainTextSize = 0x079B1430;

// Rooted managed-string slots. Runtime access is:
//   storage = *(main + slot); managedString = *storage.
// These are initialized by the game's own AOT startup code.
inline constexpr std::uintptr_t StringSlotBuildings = 0x0DFD4078;
inline constexpr std::uintptr_t StringSlotAction = 0x0DFD2800;
inline constexpr std::uintptr_t StringSlotMineElevator = 0x0DFDD3A0;
inline constexpr std::uintptr_t StringSlotSkullCave = 0x0DFE2C50;
inline constexpr std::uintptr_t StringSlotMineTileSheet = 0x0DFDD2F0;
inline constexpr std::uintptr_t StringSlotBack = 0x0DFD3560;
inline constexpr std::uintptr_t StringSlotFront = 0x0DFD9130;
// Exact rooted literals used by the shipped xTile maps. These let the mod
// reuse the lobby/floor's already loaded mine artwork rather than loading or
// distributing a replacement texture.
inline constexpr std::uintptr_t StringSlotMineImageSource = 0x0DFDCC20;
inline constexpr std::uintptr_t StringSlotMineDarkImageSource = 0x0DFDCC28;
inline constexpr std::uintptr_t StringSlotMineDesertImageSource = 0x0DFDCC30;
inline constexpr std::uintptr_t StringSlotMineSlimeImageSource = 0x0DFDCC58;

// Wear More Rings (fixed four-slot implementation).  Every symbol in this
// block was recovered from the same 1.6.15.3 AOT image.  InventoryPage's
// registered virtual invokers at main+0x073CE4E4/+0x073CE8A4/+0x073CEA88
// dispatch through the bodies below; their direct prologues and returns are
// main+0x01704280..0x01706254, +0x01706A00..0x01707398, and
// +0x01707480..0x017088A4 respectively.  The draw hook must target
// +0x01707480: +0x017074AC is after the callee-saved-register prologue and
// hooking there lets the original epilogue bypass the trampoline callback,
// corrupting SP/LR.
inline constexpr std::uintptr_t InventoryPageConstructor = 0x01703090;
inline constexpr std::uintptr_t InventoryPageReceiveLeftClick = 0x01704280;
inline constexpr std::uintptr_t InventoryPagePerformHoverAction = 0x01706A00;
inline constexpr std::uintptr_t InventoryPageDraw = 0x01707480;
inline constexpr std::uintptr_t InventoryPageSetHeldItem = 0x01703DB0;
inline constexpr std::uintptr_t InventoryPageEquipmentIcons = 0x90;
inline constexpr std::uintptr_t InventoryPageHoverText = 0x70;
inline constexpr std::uintptr_t InventoryPageHoverTitle = 0x78;
inline constexpr std::uintptr_t InventoryPageHoverAmount = 0x80;
inline constexpr std::uintptr_t InventoryPageHoveredItem = 0x88;

// ClickableComponent metadata at main+0x073A6F94 proves this field layout.
// The constructor ABI is (this-or-null, Rectangle by value, managed name).
inline constexpr std::uintptr_t ClickableComponentConstructor = 0x016B0F40;
inline constexpr std::uintptr_t ClickableComponentScale = 0x30;
inline constexpr std::uintptr_t ClickableComponentItem = 0x38;
inline constexpr std::uintptr_t ClickableComponentVisible = 0x40;
inline constexpr std::uintptr_t ClickableComponentFullyImmutable = 0x45;
inline constexpr std::uintptr_t ClickableComponentMyId = 0x48;
inline constexpr std::uintptr_t ClickableComponentLeftNeighborId = 0x50;
inline constexpr std::uintptr_t ClickableComponentRightNeighborId = 0x54;
inline constexpr std::uintptr_t ClickableComponentUpNeighborId = 0x58;
inline constexpr std::uintptr_t ClickableComponentDownNeighborId = 0x5C;
inline constexpr std::uintptr_t ClickableComponentListAdd = 0x01661AC0;

// Original InventoryPage ring click path at main+0x01704668..0x01704718.
// The fields at +0x478/+0x480 contain NetRef pointers: the original performs
// ADD farmer, offset followed by LDR [field] at +0x017046AC. Farmer.Equip
// performs old onUnequip, the NetRef transaction, then new onEquip. It must be
// used instead of writing Farmer's equipment fields.
inline constexpr std::uintptr_t FarmerLeftRingRef = 0x478;
inline constexpr std::uintptr_t FarmerRightRingRef = 0x480;
inline constexpr std::uintptr_t FarmerGetCursorSlotItem = 0x01319120;
inline constexpr std::uintptr_t FarmerSetCursorSlotItem = 0x01319130;
inline constexpr std::uintptr_t FarmerEquipBody = 0x01213110;
inline constexpr std::uintptr_t NormalizeHeldItemBody = 0x01AEFD90;
inline constexpr std::uintptr_t PrepareUnequippedItemBody = 0x01AF0290;

// Brute's non-throwing isinst helper and the exact Ring/CombinedRing cached
// metadata initializers used by the original code.
inline constexpr std::uintptr_t IsInstanceOfType = 0x00000AF0;
inline constexpr std::uintptr_t TypeInitEnter = 0x079AE700;
inline constexpr std::uintptr_t TypeInitExit = 0x079AE710;
inline constexpr std::uintptr_t RingMetadataInitFlagSlot = 0x0DF0E0B0;
inline constexpr std::uintptr_t RingMetadataStorageSlot = 0x0DF0E0B8;
inline constexpr std::uintptr_t RingMetadataFactory = 0x074EA210;
inline constexpr std::uintptr_t CombinedRingMetadataInitFlagSlot = 0x0DF0DE80;
inline constexpr std::uintptr_t CombinedRingMetadataStorageSlot = 0x0DF0DE88;
inline constexpr std::uintptr_t CombinedRingMetadataFactory = 0x074D3E34;

// Ring.Combine at main+0x0198FEA0 calls this constructor.  CombinedRing's
// own methods prove the +0xD0 child collection layout.  In particular,
// onEquip at main+0x019681D0 and onUnequip at main+0x019682B0 iterate the
// collection's live Count, so a three-child container applies every effect.
inline constexpr std::uintptr_t CombinedRingConstructor = 0x019677D0;
inline constexpr std::uintptr_t CombinedRingChildren = 0xD0;
inline constexpr std::uintptr_t NetCollectionCountRef = 0x40;
inline constexpr std::uintptr_t NetCollectionItemsRef = 0x48;
inline constexpr std::uintptr_t NetCollectionAddSlot = 0x240;
inline constexpr std::uintptr_t NetCollectionAddAdjustment = 0x248;
inline constexpr std::uintptr_t NetCollectionElementAtBody = 0x0DC8A80;

// The vanilla equipment draw at main+0x01707854..0x017078DC uses this exact
// call sequence.  It is reproduced only for the two additional UI cells.
inline constexpr std::uintptr_t Game1MenuTextureStorageSlot = 0x0DFEF158;
inline constexpr std::uintptr_t Game1GetSourceRectForStandardTileSheet =
    0x013E6EF0;
inline constexpr std::uintptr_t ColorGetWhite = 0x00058CC0;
inline constexpr std::uintptr_t SpriteBatchDrawRectangle = 0x000BFF70;
inline constexpr std::uintptr_t ItemDrawInMenuSimple = 0x015100A0;
inline constexpr std::uintptr_t StringSlotLeftRing = 0x0DFDC040;
inline constexpr std::uintptr_t StringSlotRightRing = 0x0DFE1958;
// GameLocation.GetInstancedBuildingInteriors() is registered under the
// UTF-16 method name at main+0x09748CDE.  Its managed invoker at
// main+0x072CB750 is a direct branch to this body.  The implementation builds
// the result through GameLocation.ForEachInstancedInterior, so this is the
// original game's building-interior enumeration rather than a guessed
// Building/indoors field walk.
inline constexpr std::uintptr_t GameLocationGetInstancedBuildingInteriorsBody =
    0x0149FE40;

// Automate connectors and Fish Pond support (Stardew Valley 1.6.15.3,
// Build ID A5C617C14A7F3F6620B3BC8136965A4822D32B9C only).
//
// GameLocation.GetInstancedBuildingInteriors at main+0x0149FFB0 reads the
// location's NetCollection<Building> from +0x28.  Building field metadata and
// FishPond.doAction at main+0x0116FBA0 prove the NetInt/NetRef offsets below.
inline constexpr std::uintptr_t GameLocationBuildingsRef = 0x28;
inline constexpr std::uintptr_t BuildingTileXRef = 0x40;
inline constexpr std::uintptr_t BuildingTileYRef = 0x48;
inline constexpr std::uintptr_t BuildingTilesWideRef = 0x50;
inline constexpr std::uintptr_t BuildingTilesHighRef = 0x58;
inline constexpr std::uintptr_t BuildingDaysOfConstructionLeftRef = 0x70;
inline constexpr std::uintptr_t FishPondOutputRef = 0x168;

// FishPond.doAction's registered invoker at main+0x071CD94C dispatches
// through Building's +0x180/+0x188 virtual entries.  Comparing the concrete
// target to main+0x0116FBA0 gives a non-throwing FishPond type fingerprint.
inline constexpr std::uintptr_t FishPondDoActionBody = 0x0116FBA0;
inline constexpr std::uintptr_t BuildingDoActionVirtualSlot = 0x180;
inline constexpr std::uintptr_t BuildingDoActionThisAdjustment = 0x188;

// The original FishPond collection block at main+0x0116FCF4 calls the
// produced Object's sellToStorePrice(-1), multiplies by the two static
// HARVEST constants, then calls Farmer.gainExperience(skill=Fishing).
inline constexpr std::uintptr_t ItemSellToStorePriceVirtualSlot = 0x1A0;
inline constexpr std::uintptr_t ItemSellToStorePriceThisAdjustment = 0x1A8;
inline constexpr std::uintptr_t FarmerGainExperienceVirtualSlot = 0x410;
inline constexpr std::uintptr_t FarmerGainExperienceThisAdjustment = 0x418;
inline constexpr std::uintptr_t FishPondHarvestBaseExpStorageSlot = 0x0DFEFC78;
inline constexpr std::uintptr_t FishPondHarvestOutputExpMultiplierStorageSlot =
    0x0DFEFC80;

// Flooring.initNetFields' registered invoker dispatches through the terrain
// feature +0x90/+0x98 entries.  The concrete body at main+0x01A53B40 and
// Flooring.GetData at main+0x01A53C20 both read whichFloor's NetString from
// +0x40. Flooring.GetData returns the corresponding FloorsAndPathsData; the
// placement map builder at main+0x01A53E9C reads that data's ItemId from
// +0x18. PC Automate identifies connectors through this ItemId, not by
// assuming a particular whichFloor data key.
inline constexpr std::uintptr_t FlooringInitNetFieldsBody = 0x01A53B40;
inline constexpr std::uintptr_t TerrainFeatureInitNetFieldsVirtualSlot = 0x90;
inline constexpr std::uintptr_t TerrainFeatureInitNetFieldsThisAdjustment = 0x98;
inline constexpr std::uintptr_t FlooringWhichFloorRef = 0x40;
inline constexpr std::uintptr_t FlooringGetDataBody = 0x01A53C20;
inline constexpr std::uintptr_t FlooringDataItemId = 0x18;
// Flooring placement at main+0x019477F8..0x0194782C calls these two bodies:
// first build/get the global ItemId -> FloorsAndPaths data-key dictionary,
// then index it with Item.get_ItemId().  Comparing a live Flooring.whichFloor
// value with the result for ItemId "405" follows the game's own placement
// translation and avoids assuming either the internal data key or a metadata
// factory for the concrete terrain-feature subtype.
inline constexpr std::uintptr_t FlooringGetPlacementMapBody = 0x01A53D10;
inline constexpr std::uintptr_t FlooringPlacementMapGetItemBody = 0x0110C570;
// Flooring::.ctor at main+0x01A53724..0x01A53740 initializes the exact type
// metadata through this flag/storage/factory triple before allocating a new
// Flooring. Reading the game's initialized storage and passing it to
// main+0x00000AF0 reproduces C# `feature is Flooring` without constructing or
// caching a second metadata object.
inline constexpr std::uintptr_t FlooringMetadataInitFlagSlot = 0x0DF0F590;
inline constexpr std::uintptr_t FlooringMetadataStorageSlot = 0x0DF0F598;
inline constexpr std::uintptr_t FlooringMetadataFactory = 0x0754DE74;

// Game1 currentLocation uses this static-field slot. It is retained only for
// diagnostics/fallback; locations must be read through Game1GetLocations above.
inline constexpr std::uintptr_t Game1StaticFieldsRef = 0x0DFF0530;
// Game1.locations field inside the Game1 static-field block. This is only a
// fallback when the generated getter is temporarily unreadable.
inline constexpr std::uintptr_t Game1LocationsStaticOffset = 0x7E0;

// Stage 5/6 APIs. Chest.addItem has a static wrapper/ABI observation. A separate
// generic MachineInsertItem symbol was never identified; the confirmed
// Object.PlaceInMachine body below is used instead. Chest removal remains
// deliberately UNKNOWN.
inline constexpr std::uintptr_t ChestAddItem = 0x074CF99C;
inline constexpr std::uintptr_t ChestRemoveItem = kUnknown;
inline constexpr std::uintptr_t MachineInsertItem = kUnknown;
inline constexpr std::uintptr_t MachineCollectOutput = kUnknown;

// Read-only field observations for this exact Build ID.
inline constexpr std::uintptr_t ObjectHeldObjectRef = 0x138;
inline constexpr std::uintptr_t ObjectMinutesUntilReadyRef = 0x150;
// Object field metadata at main+0x074B4F80 identifies showNextIndex as the
// NetBool field at +0x120.  The neighbouring metadata records identify
// lastInputItem at +0x148 and TileLocation at +0x0A0.
inline constexpr std::uintptr_t ObjectShowNextIndexRef = 0x120;
inline constexpr std::uintptr_t ObjectLastInputItemRef = 0x148;
inline constexpr std::uintptr_t ObjectTileLocationRef = 0x0A0;
inline constexpr std::uintptr_t NetRefValue = 0x58;

// Stage 8 static reverse-engineering results. The method body is the normal
// Object.checkForAction path: with justChecking=false it transfers a ready
// machine output to the Farmer inventory, clears the held-object/action NetRefs,
// and invokes the object's post-action virtual callback. The body does not
// itself show a write to Object+0x150; timer/ready-field reset remains a separate
// observation. Its exact behavior has not yet been confirmed on hardware.
inline constexpr std::uintptr_t ObjectCheckForActionBody = 0x019A0AE0;
inline constexpr std::uintptr_t ObjectCheckForActionInvoker = 0x074BEBC0;
// The managed invoker at main+0x074BEBC0 dispatches the concrete object's
// bool checkForAction(Farmer*, bool) implementation through these method-table
// entries.  Calling ObjectCheckForActionBody directly bypasses overrides.
inline constexpr std::uintptr_t ObjectCheckForActionVirtualSlot = 0x6F0;
inline constexpr std::uintptr_t ObjectCheckForActionThisAdjustment = 0x6F8;

// Historical static call target observed inside checkForAction at
// main+0x19A0B88. It is no longer used by automation input: AttemptAutoLoad
// owns the complete inventory transaction.
inline constexpr std::uintptr_t FarmerAddItemToInventoryBoolBody = 0x0132DA20;

// Item.getOne() direct body. The body dispatches the concrete Item clone
// virtuals (slots +0x3F0/+0x400). Output collection uses it before Chest.addItem
// to preserve the stable sample needed by OutputCollected/tapper callbacks.
inline constexpr std::uintptr_t ItemGetOneBody = 0x015116C0;

// Item stack accessors confirmed by static AOT analysis for this Build ID.
// `Item.set_Stack` writes through the item's NetInt, so it preserves the
// game's normal replication/change-notification path.
inline constexpr std::uintptr_t ItemGetParentSheetIndexBody = 0x0150EA10;
// Stack is a separate NetInt from ParentSheetIndex in this build.
// These are the direct Stack property bodies.
inline constexpr std::uintptr_t ItemGetStackBody = 0x0150EB70;
inline constexpr std::uintptr_t ItemSetStackBody = 0x0150EB80;
inline constexpr std::uintptr_t ItemGetItemIdBody = 0x0150EA40;
// Legacy managed invoker retained for reference only; callers must use the
// direct Stack property body above.
inline constexpr std::uintptr_t ItemGetStack = 0x0730EB6C;

// Vanilla machine-data/input path. The historical alias name is retained so
// older notes remain readable; 0x0192B040 is Object.GetMachineData.
inline constexpr std::uintptr_t ObjectGetMachineDataId = 0x0192B040;
inline constexpr std::uintptr_t ObjectPlaceInMachine = 0x0192BE40;
// Full vanilla player drop-in path. Static body ABI from main+0x019A06E0:
// bool (Object*, Item*, bool probe, Farmer*, bool returnFalseIfConsumed).
// It calls GetMachineDataId/PlaceInMachine and then performs the original
// last-input clone/NetRef and post-processing steps.
inline constexpr std::uintptr_t ObjectPerformObjectDropInActionBody = 0x019A06E0;
// Verified from the five-argument managed invoker at main+0x074BCD40.  It
// unboxes the two bool arguments, then dispatches the concrete object through
// method-table +0x5E0 with the this-adjustment byte at +0x5E8.
inline constexpr std::uintptr_t ObjectPerformDropInVirtualSlot = 0x5E0;
inline constexpr std::uintptr_t ObjectPerformDropInThisAdjustment = 0x5E8;

// Object.AttemptAutoLoad(IInventory, Farmer) is the two-argument overload in
// Object's method metadata.  Its invoker at main+0x074C0234 unpacks the two
// managed references and dispatches through +0x810/+0x818.  This is the API
// used by PC Automate 2.6.1's DataBasedObjectMachine.SetInput.
inline constexpr std::uintptr_t ObjectAttemptAutoLoadInvoker = 0x074C0234;
inline constexpr std::uintptr_t ObjectAttemptAutoLoadVirtualSlot = 0x810;
inline constexpr std::uintptr_t ObjectAttemptAutoLoadThisAdjustment = 0x818;

// Chest read-only getter.  The body at this offset is the generated wrapper
// for Chest.GetItemsForPlayer(long playerId): it loads the current player's
// UniqueMultiplayerID in vanilla, then tail-calls the implementation with
// (Chest*, playerId).  Passing only X0 is invalid on this Build ID.
inline constexpr std::uintptr_t ChestGetItemsForPlayerBody = 0x01960210;

// Brute/AOT interface dispatch used by Chest's own IList<Item> callers on
// this exact Build ID. At main+0x0196055C the game resolves
// ICollection<Item> slot 0 and calls Count; at main+0x01960598 it resolves
// IList<Item> slot 0 and calls get_Item(index). These replace guessed
// List/NetList memory layouts.
inline constexpr std::uintptr_t ResolveInterfaceMethod = 0x00000E30;
inline constexpr std::uintptr_t ICollectionItemMetadataFactory = 0x0610890C;
inline constexpr std::uintptr_t IListItemMetadataFactory = 0x06B3143C;

// Object.GetMachineData's metadata invoker at main+0x074C24B8 branches to
// this body.  The older GetMachineDataId name is kept as an alias so prior
// analysis notes remain readable.
inline constexpr std::uintptr_t ObjectGetMachineDataBody = 0x0192B040;

// Item.ResetParentSheetIndex's metadata invoker at main+0x0730C0AC calls this
// body directly.  Vanilla's data-based output collection calls it after
// clearing heldObject/readyForHarvest/showNextIndex.
inline constexpr std::uintptr_t ItemResetParentSheetIndexBody = 0x0150F760;

// MachineDataUtility.TryGetMachineOutputRule's managed invoker at
// main+0x07383B68 calls this ten-argument body.  Vanilla Object collection at
// main+0x01937944 passes trigger 2 and uses the first out value as the rule for
// Object.OutputMachine.
inline constexpr std::uintptr_t TryGetMachineOutputRuleBody = 0x01657250;

// Object.OutputMachine's managed invoker and virtual ABI.  The same slots and
// argument order are visible in the vanilla collection path at
// main+0x01937964..0x01937998.
inline constexpr std::uintptr_t ObjectOutputMachineInvoker = 0x074BD91C;
inline constexpr std::uintptr_t ObjectOutputMachineVirtualSlot = 0x600;
inline constexpr std::uintptr_t ObjectOutputMachineThisAdjustment = 0x608;

// Tapper follow-up used by the same vanilla output path. Object.IsTapper is
// dispatched at main+0x0193799C, the location terrain-feature dictionary is
// queried at main+0x019379D4, and Tree.UpdateTapperProduct is called at
// main+0x01937A40.
inline constexpr std::uintptr_t ObjectIsTapperVirtualSlot = 0x9B0;
inline constexpr std::uintptr_t ObjectIsTapperThisAdjustment = 0x9B8;
inline constexpr std::uintptr_t GameLocationTerrainFeaturesRef = 0x140;
inline constexpr std::uintptr_t TerrainFeatureDictionaryValue = 0x48;
inline constexpr std::uintptr_t TerrainFeatureTryGetValueBody = 0x00DD5E80;
// The concrete dictionary stores NetRef<TerrainFeature>-style fields, not the
// TerrainFeature values exposed by NetVector2Dictionary.Pairs. Vanilla at
// main+0x019379EC..0x01937A04 passes the raw dictionary value through these
// virtual entries on the owning NetVector2Dictionary before any type test.
inline constexpr std::uintptr_t TerrainFeatureUnwrapVirtualSlot = 0x210;
inline constexpr std::uintptr_t TerrainFeatureUnwrapThisAdjustment = 0x218;
inline constexpr std::uintptr_t TreeUpdateTapperProductBody = 0x01A77FA0;

constexpr bool IsKnown(std::uintptr_t offset) noexcept {
    return offset != kUnknown;
}

} // namespace StardewValley::Offsets



