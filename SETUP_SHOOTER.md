# Shooter C++ setup (UE 4.27)

The runtime module builds successfully as `MyProjectEditor Win64 Development`.

## One-time editor setup

1. Create Blueprint children of `ShooterCharacter`, `WeaponBase`, `BallisticProjectile`,
   `PickupActor`, `ZombieCharacter`, and `WindField`.
2. Set the player Blueprint as Default Pawn Class in the active GameMode.
3. Add a `WeaponSocket` socket to the player skeleton's hand. Add a `Muzzle` socket to
   every weapon skeletal mesh. Assign a ballistic projectile class to each weapon.
4. Create a Data Table whose row struct is `ItemDefinition`. Assign it to the player's
   Inventory component. The Data Table row name and `ItemId` on pickups must match.
5. Place a `NavMeshBoundsVolume` over all playable ground and press P to verify that
   the navigable area is green. Use `NavLinkProxy` actors for jumps, vaults, gaps, and
   other special traversal links.
6. Place one WindField in the level. Its vector uses Unreal centimetres per second.
7. Give zombie and player Blueprint children skeletal meshes, animation Blueprints,
   sounds, montages, and collision presets. The C++ layer intentionally exposes these
   presentation choices to Blueprint.
8. Add HUD widgets by binding to Inventory `OnInventoryChanged` and Health
   `OnHealthChanged`; ammo is available on `WeaponBase`.

## Input

- WASD: movement
- Mouse: camera
- Left mouse: fire
- R: reload
- E: interact/pick up
- Left Shift: sprint
- Space: jump

## Multiplayer contract

Damage, pickup, inventory mutation, weapon spawning, ammo use, and projectile spawning
are server-authoritative. Replicated state is already present, but production online
play still requires lag compensation, relevancy/dormancy tuning, anti-cheat validation,
session/lobby code, persistence, UI, animation, audio, content balancing, and network QA.

## Build command

```powershell
& 'C:\Program Files\Epic Games\UE_4.27\Engine\Build\BatchFiles\Build.bat' `
  MyProjectEditor Win64 Development `
  'C:\Program Files\Epic Games\UE_4.27\Coop\MyProject.uproject' `
  -WaitMutex -NoHotReload -NoXGE -MaxParallelActions=4
```
