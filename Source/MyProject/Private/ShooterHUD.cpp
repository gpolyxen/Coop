#include "ShooterHUD.h"
#include "ShooterCharacter.h"
#include "HealthArmorComponent.h"
#include "WeaponBase.h"
#include "ZombieSpawnManager.h"
#include "OpenWorldStreamingManager.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AShooterHUD::DrawHUD()
{
	Super::DrawHUD();
	if(!Canvas)return;
	AShooterCharacter* C=Cast<AShooterCharacter>(GetOwningPawn());
	if(!C)return;

	const float CenterX=Canvas->ClipX*.5f,CenterY=Canvas->ClipY*.5f;
	for(TActorIterator<AOpenWorldStreamingManager> It(GetWorld());It;++It)
	{
		const float Hours=It->GetTimeHours();const int32 Hour=FMath::FloorToInt(Hours)%24;const int32 Minute=FMath::FloorToInt((Hours-Hour)*60.f)%60;
		const FLinearColor TimeColor=It->IsNightTime()?FLinearColor(.45f,.62f,1.f):FLinearColor(1.f,.82f,.3f);
		DrawText(FString::Printf(TEXT("%02d:%02d  %s"),Hour,Minute,*It->GetTimePeriodName().ToString()),TimeColor,Canvas->ClipX-255.f,25.f,GEngine->GetMediumFont(),1.05f,false);
		if(It->IsNightTime())DrawText(TEXT("НОЧЬ: ЗОМБИ УСИЛЕНЫ"),FLinearColor(1.f,.18f,.12f),Canvas->ClipX-285.f,53.f,GEngine->GetSmallFont(),.95f,false);
		break;
	}
	for(TActorIterator<AZombieSpawnManager> It(GetWorld());It;++It)
	{
		const float Remaining=It->GetPreparationRemaining();
		if(Remaining>0.f){DrawText(FString::Printf(TEXT("ПОДГОТОВКА К ВОЛНЕ: %02d"),FMath::CeilToInt(Remaining)),FLinearColor(1.f,.78f,.12f),CenterX-155.f,24.f,GEngine->GetLargeFont(),1.15f,false);break;}
	}
	if(C->IsBuilding())
	{
		const FLinearColor BuildColor=C->IsBuildPreviewValid()?FLinearColor(.2f,1.f,.3f):FLinearColor(1.f,.18f,.08f);
		DrawText(FString::Printf(TEXT("СТРОИТЕЛЬСТВО: %s"),*C->GetSelectedBuildName()),BuildColor,CenterX-175.f,70.f,GEngine->GetMediumFont(),1.15f,false);
		DrawText(C->IsBuildPreviewValid()?TEXT("ЛКМ: ПОСТАВИТЬ   R: ПОВЕРНУТЬ 90°   ПКМ: ОТМЕНА"):TEXT("МЕСТО ЗАНЯТО ИЛИ ПОВЕРХНОСТЬ НЕРОВНАЯ"),BuildColor,CenterX-245.f,100.f,GEngine->GetSmallFont(),1.f,false);
	}
	const bool bScoped=C->EquippedWeapon&&C->IsAiming()&&C->EquippedWeapon->bUseScopeOverlay;
	if(bScoped)
	{
		const float Radius=FMath::Min(Canvas->ClipX,Canvas->ClipY)*.38f;
		DrawRect(FLinearColor::Black,0.f,0.f,CenterX-Radius,Canvas->ClipY);
		DrawRect(FLinearColor::Black,CenterX+Radius,0.f,Canvas->ClipX-(CenterX+Radius),Canvas->ClipY);
		DrawRect(FLinearColor::Black,CenterX-Radius,0.f,Radius*2.f,CenterY-Radius);
		DrawRect(FLinearColor::Black,CenterX-Radius,CenterY+Radius,Radius*2.f,Canvas->ClipY-(CenterY+Radius));
		const int32 Segments=64;
		for(int32 Index=0;Index<Segments;++Index)
		{
			const float A0=2.f*PI*Index/Segments,A1=2.f*PI*(Index+1)/Segments;
			DrawLine(CenterX+FMath::Cos(A0)*Radius,CenterY+FMath::Sin(A0)*Radius,CenterX+FMath::Cos(A1)*Radius,CenterY+FMath::Sin(A1)*Radius,FLinearColor::Black,4.f);
		}
		DrawLine(CenterX-Radius,CenterY,CenterX+Radius,CenterY,FLinearColor::Black,1.f);
		DrawLine(CenterX,CenterY-Radius,CenterX,CenterY+Radius,FLinearColor::Black,1.f);
	}
	else if(!C->IsDead())
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
	const float StaminaX=36.f,StaminaY=Canvas->ClipY-42.f,StaminaWidth=260.f,StaminaHeight=10.f;
	DrawRect(FLinearColor(0.f,0.f,0.f,.7f),StaminaX-2.f,StaminaY-2.f,StaminaWidth+4.f,StaminaHeight+4.f);
	DrawRect(C->HasSkill(EShooterSkill::Marathon)?FLinearColor(.2f,.8f,1.f):FLinearColor(1.f,.72f,.12f),StaminaX,StaminaY,StaminaWidth*C->GetStaminaRatio(),StaminaHeight);
	DrawText(C->HasSkill(EShooterSkill::Marathon)?TEXT("STAMINA: MARATHON"):FString::Printf(TEXT("STAMINA %.0f / %.0f"),C->Stamina,C->MaxStamina),FLinearColor::White,StaminaX,StaminaY-20.f,GEngine->GetSmallFont(),.8f,false);
	const int32 ExperienceNeeded=FMath::Max(1,C->GetExperienceForNextLevel());
	const float ExperienceRatio=FMath::Clamp(static_cast<float>(C->Experience)/ExperienceNeeded,0.f,1.f);
	const float ExperienceX=36.f,ExperienceY=Canvas->ClipY-122.f,ExperienceWidth=260.f;
	DrawRect(FLinearColor(0.f,0.f,0.f,.7f),ExperienceX-2.f,ExperienceY-2.f,ExperienceWidth+4.f,12.f);
	DrawRect(FLinearColor(.2f,.55f,1.f,1.f),ExperienceX,ExperienceY,ExperienceWidth*ExperienceRatio,8.f);
	DrawText(FString::Printf(TEXT("LEVEL %d   XP %d / %d"),C->CharacterLevel,C->Experience,ExperienceNeeded),FLinearColor::White,ExperienceX,ExperienceY-25.f,GEngine->GetSmallFont(),1.f,false);
	const FLinearColor SkillColor=C->SkillPoints>0?FLinearColor(1.f,.78f,.15f):FLinearColor(.65f,.7f,.72f);
	DrawText(FString::Printf(TEXT("SKILL POINTS: %d  [K]     INVENTORY [I]"),C->SkillPoints),SkillColor,ExperienceX,ExperienceY-48.f,GEngine->GetSmallFont(),.9f,false);
	if(C->IsLastLifeInvulnerable())
	{
		const float Remaining=FMath::Max(0.f,C->LastLifeInvulnerableUntil-GetWorld()->GetTimeSeconds());
		DrawText(FString::Printf(TEXT("LAST LIFE  %.1f"),Remaining),FLinearColor(1.f,.2f,.1f),CenterX-80.f,CenterY-95.f,GEngine->GetMediumFont(),1.2f,false);
	}

	FString WeaponText=TEXT("E - pick up item");
	if(C->EquippedWeapon)WeaponText=FString::Printf(TEXT("%s  %d / %d%s"),*C->EquippedWeapon->WeaponName,C->EquippedWeapon->AmmoInMagazine,C->EquippedWeapon->ReserveAmmo,C->EquippedWeapon->bIsReloading?TEXT("  RELOADING"):TEXT(""));
	DrawText(WeaponText,FLinearColor::White,Canvas->ClipX-260.f,Canvas->ClipY-80.f,GEngine->GetMediumFont(),1.2f,false);
	if(!C->LocalNotification.IsEmpty()&&GetWorld()&&GetWorld()->GetTimeSeconds()<C->LocalNotificationEndTime)
		DrawText(C->LocalNotification,FLinearColor(.25f,.8f,1.f),CenterX-190.f,CenterY+150.f,GEngine->GetMediumFont(),1.1f,false);
	if(C->CanBeRevived())
	{
		DrawText(TEXT("ВЫ РАНЕНЫ"),FLinearColor(1.f,.15f,.08f),CenterX-110.f,CenterY-55.f,GEngine->GetLargeFont(),1.5f,false);
		DrawText(FString::Printf(TEXT("СОЮЗНИК МОЖЕТ ВОСКРЕСИТЬ ВАС: %.0f"),C->GetReviveSecondsRemaining()),FLinearColor::White,CenterX-210.f,CenterY-12.f,GEngine->GetMediumFont(),1.f,false);
	}
	else if(C->IsDead())DrawText(TEXT("YOU DIED"),FLinearColor::Red,CenterX-95.f,CenterY-35.f,GEngine->GetLargeFont(),1.5f,false);
	else
	{
		for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)
		{
			if(*It==C||!It->CanBeRevived()||FVector::DistSquared(C->GetActorLocation(),It->GetActorLocation())>FMath::Square(375.f))continue;
			DrawText(FString::Printf(TEXT("E — ВОСКРЕСИТЬ СОЮЗНИКА (%.0f)"),It->GetReviveSecondsRemaining()),FLinearColor(.2f,1.f,.35f),CenterX-175.f,CenterY+95.f,GEngine->GetMediumFont(),1.1f,false);
			break;
		}
	}
}
