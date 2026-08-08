#include "ShooterHUD.h"
#include "ShooterCharacter.h"
#include "WeaponBase.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AShooterHUD::DrawHUD(){Super::DrawHUD();if(!Canvas)return;const float X=Canvas->ClipX*.5f,Y=Canvas->ClipY*.5f;DrawLine(X-8,Y,X+8,Y,FLinearColor::White,1.5f);DrawLine(X,Y-8,X,Y+8,FLinearColor::White,1.5f);AShooterCharacter*C=Cast<AShooterCharacter>(GetOwningPawn());if(!C)return;FString Text=TEXT("E - pick up weapon");if(C->EquippedWeapon)Text=FString::Printf(TEXT("%s  %d / %d%s"),*C->EquippedWeapon->WeaponName,C->EquippedWeapon->AmmoInMagazine,C->EquippedWeapon->ReserveAmmo,C->EquippedWeapon->bIsReloading?TEXT("  RELOADING"):TEXT(""));DrawText(Text,FLinearColor::White,Canvas->ClipX-260.f,Canvas->ClipY-80.f,GEngine->GetMediumFont(),1.2f,false);}
