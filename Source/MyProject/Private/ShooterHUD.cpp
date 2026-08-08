#include "ShooterHUD.h"
#include "ShooterCharacter.h"
#include "HealthArmorComponent.h"
#include "WeaponBase.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AShooterHUD::DrawHUD()
{
	Super::DrawHUD();
	if(!Canvas)return;
	AShooterCharacter* C=Cast<AShooterCharacter>(GetOwningPawn());
	if(!C)return;

	const float CenterX=Canvas->ClipX*.5f,CenterY=Canvas->ClipY*.5f;
	if(!C->IsDead())
	{
		DrawLine(CenterX-8.f,CenterY,CenterX+8.f,CenterY,FLinearColor::White,1.5f);
		DrawLine(CenterX,CenterY-8.f,CenterX,CenterY+8.f,FLinearColor::White,1.5f);
	}

	if(C->Health)
	{
		const float MaxHealth=FMath::Max(1.f,C->Health->MaxHealth);
		const float HealthRatio=FMath::Clamp(C->Health->Health/MaxHealth,0.f,1.f);
		const float BarX=36.f,BarY=Canvas->ClipY-78.f,BarWidth=260.f,BarHeight=18.f;
		DrawRect(FLinearColor(0.f,0.f,0.f,.7f),BarX-3.f,BarY-3.f,BarWidth+6.f,BarHeight+6.f);
		DrawRect(FLinearColor::LerpUsingHSV(FLinearColor::Red,FLinearColor::Green,HealthRatio),BarX,BarY,BarWidth*HealthRatio,BarHeight);
		DrawText(FString::Printf(TEXT("HP  %.0f / %.0f"),C->Health->Health,MaxHealth),FLinearColor::White,BarX,BarY-28.f,GEngine->GetMediumFont(),1.f,false);
	}

	FString WeaponText=TEXT("E - pick up weapon");
	if(C->EquippedWeapon)WeaponText=FString::Printf(TEXT("%s  %d / %d%s"),*C->EquippedWeapon->WeaponName,C->EquippedWeapon->AmmoInMagazine,C->EquippedWeapon->ReserveAmmo,C->EquippedWeapon->bIsReloading?TEXT("  RELOADING"):TEXT(""));
	DrawText(WeaponText,FLinearColor::White,Canvas->ClipX-260.f,Canvas->ClipY-80.f,GEngine->GetMediumFont(),1.2f,false);
	if(C->IsDead())DrawText(TEXT("YOU DIED"),FLinearColor::Red,CenterX-95.f,CenterY-35.f,GEngine->GetLargeFont(),1.5f,false);
}
