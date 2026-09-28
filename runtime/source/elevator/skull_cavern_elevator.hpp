#pragma once

#include <cstdint>

#include "game/runtime.hpp"

namespace AutomateLite::Elevator {

class SkullCavernElevator final {
public:
    static SkullCavernElevator& Instance() noexcept;

    // Called from the already verified MonoGame Tick hook. Work is throttled
    // and only the live Skull Cavern lobby/current generated mine is touched.
    void Update() noexcept;

    bool IsElevatorContext() const noexcept;
    void* GetSkullCaveString() const noexcept;
    std::int32_t GetDeepestMineLevel() const noexcept;

    static constexpr std::int32_t BaseMineLevel = 120;
    static constexpr std::int32_t ElevatorStep = 5;

private:
    SkullCavernElevator() = default;

    void* ResolveManagedString(std::uintptr_t rootSlot) const noexcept;
    Game::GameLocation* GetSkullCaveLobby() const noexcept;
    std::int32_t GetCurrentMineLevel() const noexcept;
    void* FindMineArtworkTileSheetId(
        Game::GameLocation* location) const noexcept;
    bool PlaceElevatorTile(
        Game::GameLocation* location, std::int32_t x,
        std::int32_t y) const noexcept;
    bool InstallLobbyElevator(Game::GameLocation* lobby) noexcept;
    void InstallMineElevator(
        Game::GameLocation* mine, std::int32_t mineLevel) noexcept;
    bool IsElevatorFloor(std::int32_t mineLevel) const noexcept;

    // Start half a period out of phase with Automate's 30-tick live-location
    // scan so both main-thread jobs do not land on the same frame.
    std::uint32_t m_Throttle{15};
    Game::GameLocation* m_LastLobby{};
    bool m_LobbyInstalled{};
    Game::GameLocation* m_LastMine{};
    std::int32_t m_LastMineLevel{};
};

} // namespace AutomateLite::Elevator
