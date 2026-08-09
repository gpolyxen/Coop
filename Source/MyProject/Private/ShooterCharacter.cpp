#include "ShooterCharacter.h"
#include "WeaponBase.h"
#include "PickupActor.h"
#include "SaveBed.h"
#include "ShooterGameInstance.h"
#include "InventoryComponent.h"
#include "HealthArmorComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSequence.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/UnrealType.h"
#include "StarterRifle.h"
#include "KA47Rifle.h"
#include "SMG11Weapon.h"
#include "WeaponPickup.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "NavigationInvokerComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

AShooterCharacter::AShooterCharacter()
{
	bReplicates=true;
	bUseControllerRotationPitch=false;bUseControllerRotationYaw=true;bUseControllerRotationRoll=false;
	CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));CameraBoom->SetupAttachment(RootComponent);CameraBoom->TargetArmLength=0.f;CameraBoom->bDoCollisionTest=false;
	Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("CharacterCamera"));Camera->SetupAttachment(GetMesh(),TEXT("head"));Camera->SetRelativeLocation(FVector::ZeroVector);Camera->SetRelativeRotation(FRotator::ZeroRotator);Camera->bUsePawnControlRotation=true;Camera->FieldOfView=90.f;
	FirstPersonBody=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonBody"));FirstPersonBody->SetupAttachment(RootComponent);FirstPersonBody->SetRelativeLocation(FVector(0.f,0.f,-90.f));FirstPersonBody->SetRelativeRotation(FRotator(0.f,-90.f,0.f));FirstPersonBody->SetOnlyOwnerSee(true);FirstPersonBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);FirstPersonBody->CastShadow=false;
	FirstPersonArms=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));FirstPersonArms->SetupAttachment(Camera);FirstPersonArms->SetRelativeLocation(FVector(-20.f,12.f,-105.f));FirstPersonArms->SetRelativeRotation(FRotator(0.f,-10.f,0.f));FirstPersonArms->SetRelativeScale3D(FVector(.65f));FirstPersonArms->SetOnlyOwnerSee(true);FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);FirstPersonArms->CastShadow=false;
	FirstPersonArms->SetHiddenInGame(true);
	LeftFirstPersonArm=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFirstPersonArm"));LeftFirstPersonArm->SetupAttachment(Camera);LeftFirstPersonArm->SetOnlyOwnerSee(true);LeftFirstPersonArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);LeftFirstPersonArm->CastShadow=false;
	RightFirstPersonArm=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFirstPersonArm"));RightFirstPersonArm->SetupAttachment(Camera);RightFirstPersonArm->SetOnlyOwnerSee(true);RightFirstPersonArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);RightFirstPersonArm->CastShadow=false;
	LeftFirstPersonArm->SetHiddenInGame(true);RightFirstPersonArm->SetHiddenInGame(true);
	Inventory=CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));Health=CreateDefaultSubobject<UHealthArmorComponent>(TEXT("Health"));NavigationInvoker=CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavigationInvoker"));NavigationInvoker->SetGenerationRadii(12000.f,16000.f);StimuliSource=CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
	GetCharacterMovement()->MaxWalkSpeed=WalkSpeed;GetCharacterMovement()->bOrientRotationToMovement=false;GetCharacterMovement()->RotationRate=FRotator(0.f,540.f,0.f);GetCharacterMovement()->JumpZVelocity=600.f;GetCharacterMovement()->AirControl=.25f;GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch=true;
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/ThirdPersonBP/Player_0/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin"));if(MeshAsset.Succeeded()){GetMesh()->SetSkeletalMesh(MeshAsset.Object);GetMesh()->SetRelativeLocation(FVector(0.f,0.f,-100.f));GetMesh()->SetRelativeRotation(FRotator(0.f,-90.f,0.f));GetMesh()->SetOwnerNoSee(false);FirstPersonBody->SetSkeletalMesh(MeshAsset.Object);}
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>ArmsAsset(TEXT("/Game/FirstPerson/Character/Mesh/SK_Mannequin_Arms.SK_Mannequin_Arms"));if(ArmsAsset.Succeeded())FirstPersonArms->SetSkeletalMesh(ArmsAsset.Object);
	static ConstructorHelpers::FClassFinder<UAnimInstance> ArmedAnim(TEXT("/Game/ThirdPersonBP/Player_0/Anim/UE4ASP_HeroTPP_AnimBlueprint"));if(ArmedAnim.Succeeded())ArmedAnimClass=ArmedAnim.Class;
	static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAnim(TEXT("/Game/Mannequin/Animations/ThirdPerson_AnimBP"));if(UnarmedAnim.Succeeded())UnarmedAnimClass=UnarmedAnim.Class;
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);if(UnarmedAnimClass)GetMesh()->SetAnimInstanceClass(UnarmedAnimClass);
	static ConstructorHelpers::FObjectFinder<UBlendSpace>RifleBlend(TEXT("/Game/ThirdPersonBP/Player_0/Anim/BS_Jog.BS_Jog"));if(RifleBlend.Succeeded())RifleLocomotion=RifleBlend.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence>FPIdle(TEXT("/Game/FirstPerson/Animations/FirstPerson_Idle.FirstPerson_Idle"));if(FPIdle.Succeeded()){FirstPersonIdleAnimation=FPIdle.Object;FirstPersonArms->SetAnimationMode(EAnimationMode::AnimationSingleNode);FirstPersonArms->SetAnimation(FirstPersonIdleAnimation);FirstPersonArms->Play(true);}
	static ConstructorHelpers::FObjectFinder<UAnimSequence>FPFireAnim(TEXT("/Game/FirstPerson/Animations/FirstPerson_Fire.FirstPerson_Fire"));if(FPFireAnim.Succeeded())FirstPersonFireAnimation=FPFireAnim.Object;
	static ConstructorHelpers::FObjectFinder<UAnimMontage>FPFire(TEXT("/Game/FirstPerson/Animations/FirstPersonFire_Montage.FirstPersonFire_Montage"));if(FPFire.Succeeded())FirstPersonFireMontage=FPFire.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh>LeftArmAsset(TEXT("/Game/ThirdPerson/Meshes/LeftArm_StaticMesh.LeftArm_StaticMesh"));if(LeftArmAsset.Succeeded())LeftFirstPersonArm->SetStaticMesh(LeftArmAsset.Object);
	static ConstructorHelpers::FObjectFinder<UStaticMesh>RightArmAsset(TEXT("/Game/ThirdPerson/Meshes/RightArm_StaticMesh.RightArm_StaticMesh"));if(RightArmAsset.Succeeded())RightFirstPersonArm->SetStaticMesh(RightArmAsset.Object);
	LeftFirstPersonArm->SetRelativeLocation(FVector(42.f,0.f,-30.f));LeftFirstPersonArm->SetRelativeRotation(FRotator(0.f,-90.f,0.f));LeftFirstPersonArm->SetRelativeScale3D(FVector(.01f));
	RightFirstPersonArm->SetRelativeLocation(FVector(42.f,0.f,-30.f));RightFirstPersonArm->SetRelativeRotation(FRotator(0.f,-90.f,0.f));RightFirstPersonArm->SetRelativeScale3D(FVector(.01f));
}
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	if(StimuliSource)StimuliSource->RegisterWithPerceptionSystem();
	if(Health)Health->OnDeath.AddDynamic(this,&AShooterCharacter::HandleDeath);
	FirstPersonBody->SetHiddenInGame(true);
	FirstPersonArms->SetHiddenInGame(true);
	LeftFirstPersonArm->SetHiddenInGame(true);
	RightFirstPersonArm->SetHiddenInGame(true);
	GetMesh()->SetOwnerNoSee(false);
	RefreshAnimationState();
	if(GetMesh()->DoesSocketExist(CameraSocketName))
	{
		Camera->AttachToComponent(GetMesh(),FAttachmentTransformRules::SnapToTargetNotIncludingScale,CameraSocketName);
		Camera->SetRelativeLocation(CameraSocketOffset);
		Camera->SetRelativeRotation(CameraSocketRotation);
		Camera->SetRelativeScale3D(CameraSocketScale);
	}
	else UE_LOG(LogTemp,Error,TEXT("Camera socket %s is missing on %s"),*CameraSocketName.ToString(),*GetNameSafe(GetMesh()));
	if(IsLocallyControlled())
		if(APlayerController* PC=Cast<APlayerController>(Controller))
		{
			PC->PlayerCameraManager->ViewPitchMin=-80.f;
			PC->PlayerCameraManager->ViewPitchMax=80.f;
		}
	if(FParse::Param(FCommandLine::Get(),TEXT("CodexCapture"))&&IsLocallyControlled())
	{
		bIsAiming=true;
		if(HasAuthority())EquipWeapon(AKA47Rifle::StaticClass());
		FTimerHandle CaptureTimer;
		GetWorldTimerManager().SetTimer(CaptureTimer,this,&AShooterCharacter::CaptureDiagnosticScreenshot,3.f,false);
	}
}

bool AShooterCharacter::IsDead()const{return Health&&Health->IsDead();}

void AShooterCharacter::HandleDeath(){StopFire();bIsAiming=false;if(Camera)Camera->SetFieldOfView(HipFOV);if(UCharacterMovementComponent* Movement=GetCharacterMovement()){Movement->StopMovementImmediately();Movement->DisableMovement();}if(IsLocallyControlled())if(APlayerController* PC=Cast<APlayerController>(Controller))DisableInput(PC);UE_LOG(LogTemp,Display,TEXT("Player %s died"),*GetName());}
void AShooterCharacter::CaptureDiagnosticScreenshot(){FScreenshotRequest::RequestScreenshot(TEXT("CodexWeaponView.png"),false,false);}
void AShooterCharacter::Tick(float D)
{
	Super::Tick(D);
	const bool bWeaponAim=bIsAiming&&EquippedWeapon;
	const float DesiredFOV=bWeaponAim?EquippedWeapon->WeaponAimFOV:HipFOV;
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,DesiredFOV,D,12.f));
	FVector DesiredCameraLocation=CameraSocketOffset;
	if(bWeaponAim)
	{
		if(EquippedWeapon->bUseAimReferenceSocket&&EquippedWeapon->Mesh->DoesSocketExist(EquippedWeapon->AimReferenceSocket)&&GetMesh()->DoesSocketExist(CameraSocketName))
		{
			const FVector SightLocation=EquippedWeapon->Mesh->GetSocketLocation(EquippedWeapon->AimReferenceSocket);
			const FVector ViewDirection=Controller?Controller->GetControlRotation().Vector():Camera->GetForwardVector();
			const FVector DesiredWorldLocation=SightLocation-ViewDirection*EquippedWeapon->AimEyeRelief;
			DesiredCameraLocation=GetMesh()->GetSocketTransform(CameraSocketName,RTS_World).InverseTransformPosition(DesiredWorldLocation);
		}
		else DesiredCameraLocation+=EquippedWeapon->AimCameraOffset;
	}
	Camera->SetRelativeLocation(FMath::VInterpTo(Camera->GetRelativeLocation(),DesiredCameraLocation,D,14.f));
	const float Recovery=EquippedWeapon?EquippedWeapon->RecoilRecoverySpeed:12.f;
	RecoilPitchTarget=FMath::FInterpTo(RecoilPitchTarget,0.f,D,Recovery);RecoilYawTarget=FMath::FInterpTo(RecoilYawTarget,0.f,D,Recovery);RecoilKickTarget=FMath::FInterpTo(RecoilKickTarget,0.f,D,Recovery);
	const FVector LocalVelocity=GetActorTransform().InverseTransformVectorNoScale(GetVelocity());
	const float Direction=FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y,LocalVelocity.X));
	auto UpdateAnim=[this,Direction](UAnimInstance* Anim){if(!Anim)return;auto SetFloat=[Anim](const TCHAR* Name,float Value){if(FFloatProperty* P=FindFProperty<FFloatProperty>(Anim->GetClass(),Name))P->SetPropertyValue_InContainer(Anim,Value);};auto SetBool=[Anim](const TCHAR* Name,bool Value){if(FBoolProperty* P=FindFProperty<FBoolProperty>(Anim->GetClass(),Name))P->SetPropertyValue_InContainer(Anim,Value);};const bool bHasWeapon=EquippedWeapon!=nullptr;SetFloat(TEXT("Speed"),GetVelocity().Size2D());SetFloat(TEXT("Direction"),Direction);SetBool(TEXT("IsAiming"),bHasWeapon&&bIsAiming);SetBool(TEXT("IsAiming?"),bHasWeapon&&bIsAiming);SetBool(TEXT("Is Aiming?"),bHasWeapon&&bIsAiming);SetBool(TEXT("HasWeapon"),bHasWeapon);SetBool(TEXT("Has Weapon?"),bHasWeapon);SetBool(TEXT("Crouching"),bIsCrouched);SetBool(TEXT("Jumping"),GetCharacterMovement()->IsFalling());FObjectPropertyBase* PlayerProperty=FindFProperty<FObjectPropertyBase>(Anim->GetClass(),TEXT("As BP Player"));if(!PlayerProperty)PlayerProperty=FindFProperty<FObjectPropertyBase>(Anim->GetClass(),TEXT("AsBPPlayer"));if(PlayerProperty)PlayerProperty->SetObjectPropertyValue_InContainer(Anim,this);if(UFunction* CalculateRotation=Anim->FindFunction(TEXT("CalculateRotation")))Anim->ProcessEvent(CalculateRotation,nullptr);};UpdateAnim(GetMesh()->GetAnimInstance());
}
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* I){Super::SetupPlayerInputComponent(I);check(I);I->BindAxis("MoveForward",this,&AShooterCharacter::MoveForward);I->BindAxis("MoveRight",this,&AShooterCharacter::MoveRight);I->BindAxis("SwitchWeapon",this,&AShooterCharacter::SwitchWeapon);I->BindAxis("Turn",this,&APawn::AddControllerYawInput);I->BindAxis("LookUp",this,&APawn::AddControllerPitchInput);I->BindAxis("TurnRate",this,&AShooterCharacter::TurnAtRate);I->BindAxis("LookUpRate",this,&AShooterCharacter::LookUpAtRate);I->BindAction("Jump",IE_Pressed,this,&ACharacter::Jump);I->BindAction("Jump",IE_Released,this,&ACharacter::StopJumping);I->BindAction("Crouch",IE_Pressed,this,&AShooterCharacter::ToggleCrouch);I->BindAction("Aim",IE_Pressed,this,&AShooterCharacter::StartAim);I->BindAction("Aim",IE_Released,this,&AShooterCharacter::StopAim);I->BindAction("Fire",IE_Pressed,this,&AShooterCharacter::StartFire);I->BindAction("Fire",IE_Released,this,&AShooterCharacter::StopFire);I->BindAction("Reload",IE_Pressed,this,&AShooterCharacter::Reload);I->BindAction("Interact",IE_Pressed,this,&AShooterCharacter::Interact);I->BindAction("Sprint",IE_Pressed,this,&AShooterCharacter::SprintPressed);I->BindAction("Sprint",IE_Released,this,&AShooterCharacter::SprintReleased);}
void AShooterCharacter::MoveForward(float V){if(Controller&&V!=0)AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V);}void AShooterCharacter::MoveRight(float V){if(Controller&&V!=0)AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V);}
void AShooterCharacter::TurnAtRate(float V){AddControllerYawInput(V*BaseTurnRate*GetWorld()->GetDeltaSeconds());}void AShooterCharacter::LookUpAtRate(float V){AddControllerPitchInput(V*BaseTurnRate*GetWorld()->GetDeltaSeconds());}
void AShooterCharacter::ToggleCrouch(){bIsCrouched?UnCrouch():Crouch();}
void AShooterCharacter::StartAim(){if(!EquippedWeapon)return;bIsAiming=true;if(!HasAuthority())ServerSetAiming(true);}void AShooterCharacter::StopAim(){bIsAiming=false;if(!HasAuthority())ServerSetAiming(false);}bool AShooterCharacter::ServerSetAiming_Validate(bool){return true;}void AShooterCharacter::ServerSetAiming_Implementation(bool bNewAiming){bIsAiming=bNewAiming&&EquippedWeapon;}
void AShooterCharacter::StartFire(){if(IsDead()||!EquippedWeapon)return;FireOnce();const float Interval=60.f/FMath::Max(1.f,EquippedWeapon->Stats.RoundsPerMinute);GetWorldTimerManager().SetTimer(FireTimer,this,&AShooterCharacter::FireOnce,Interval,true,Interval);}void AShooterCharacter::StopFire(){GetWorldTimerManager().ClearTimer(FireTimer);}void AShooterCharacter::FireOnce(){if(IsDead()||!EquippedWeapon||!EquippedWeapon->CanFire())return;const FVector ViewStart=Camera->GetComponentLocation(),ViewEnd=ViewStart+Camera->GetForwardVector()*100000.f;FHitResult Hit;FCollisionQueryParams Params(TEXT("WeaponAim"),true,this);Params.AddIgnoredActor(EquippedWeapon);const bool bHit=GetWorld()->LineTraceSingleByChannel(Hit,ViewStart,ViewEnd,ECC_Visibility,Params);const FVector AimPoint=bHit?Hit.ImpactPoint:ViewEnd;if(!EquippedWeapon->Fire((AimPoint-EquippedWeapon->GetMuzzleLocation()).GetSafeNormal()))return;RecoilPitchTarget=FMath::Clamp(RecoilPitchTarget+EquippedWeapon->RecoilPitch,0.f,4.f);RecoilYawTarget=FMath::Clamp(RecoilYawTarget+FMath::FRandRange(-EquippedWeapon->RecoilYaw,EquippedWeapon->RecoilYaw),-1.5f,1.5f);RecoilKickTarget=FMath::Clamp(RecoilKickTarget+EquippedWeapon->RecoilKickback,0.f,5.f);if(Controller){FRotator ControlRotation=Controller->GetControlRotation();ControlRotation.Pitch=FMath::ClampAngle(ControlRotation.Pitch-EquippedWeapon->RecoilPitch,-80.f,80.f);ControlRotation.Yaw+=FMath::FRandRange(-EquippedWeapon->RecoilYaw,EquippedWeapon->RecoilYaw);Controller->SetControlRotation(ControlRotation);}}void AShooterCharacter::ResetFirstPersonArmsAnimation(){if(FirstPersonIdleAnimation)FirstPersonArms->PlayAnimation(FirstPersonIdleAnimation,true);}void AShooterCharacter::Reload(){if(IsDead())return;StopFire();if(EquippedWeapon)EquippedWeapon->Reload();}void AShooterCharacter::SprintPressed(){if(!IsDead())GetCharacterMovement()->MaxWalkSpeed=SprintSpeed;}void AShooterCharacter::SprintReleased(){if(!IsDead())GetCharacterMovement()->MaxWalkSpeed=WalkSpeed;}
void AShooterCharacter::Interact(){FHitResult H;FCollisionQueryParams P;P.AddIgnoredActor(this);const FVector A=Camera->GetComponentLocation(),B=A+Camera->GetForwardVector()*InteractionDistance;if(GetWorld()->LineTraceSingleByChannel(H,A,B,ECC_Visibility,P)){if(APickupActor* Pickup=Cast<APickupActor>(H.GetActor()))ServerInteract(Pickup);else if(ASaveBed* Bed=Cast<ASaveBed>(H.GetActor()))ServerUseBed(Bed);}}
bool AShooterCharacter::ServerInteract_Validate(APickupActor* P){return P&&FVector::DistSquared(P->GetActorLocation(),GetActorLocation())<=FMath::Square(InteractionDistance+100.f);}void AShooterCharacter::ServerInteract_Implementation(APickupActor* P){P->TryPickup(this);}
bool AShooterCharacter::ServerUseBed_Validate(ASaveBed* Bed){return Bed&&FVector::DistSquared(Bed->GetActorLocation(),GetActorLocation())<=FMath::Square(InteractionDistance+150.f);}void AShooterCharacter::ServerUseBed_Implementation(ASaveBed* Bed){if(Bed)ClientSaveAtBed();}
void AShooterCharacter::ClientSaveAtBed_Implementation(){if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())ShowLocalNotification(GI->SavePlayerAtBed(this)?TEXT("ИГРА СОХРАНЕНА У КРОВАТИ"):TEXT("НЕ УДАЛОСЬ СОХРАНИТЬ ИГРУ"));}
void AShooterCharacter::ShowLocalNotification(const FString& Message,float Duration){LocalNotification=Message;LocalNotificationEndTime=GetWorld()?GetWorld()->GetTimeSeconds()+FMath::Max(.1f,Duration):0.f;}
void AShooterCharacter::EquipWeapon(TSubclassOf<AWeaponBase>C){if(HasAuthority())ServerEquipWeapon_Implementation(C);else ServerEquipWeapon(C);}bool AShooterCharacter::ServerEquipWeapon_Validate(TSubclassOf<AWeaponBase>C){return C!=nullptr;}void AShooterCharacter::ServerEquipWeapon_Implementation(TSubclassOf<AWeaponBase>C){FActorSpawnParameters P;P.Owner=this;P.Instigator=this;AWeaponBase* W=GetWorld()->SpawnActor<AWeaponBase>(C,FTransform::Identity,P);if(!W)return;if(WeaponSlots.Num()<2){WeaponSlots.Add(W);ActiveWeaponSlot=WeaponSlots.Num()-1;}else{AWeaponBase* Old=WeaponSlots[ActiveWeaponSlot];if(Old){const FVector DropLocation=GetActorLocation()+GetActorForwardVector()*100.f+FVector(0.f,0.f,45.f);AWeaponPickup* Drop=GetWorld()->SpawnActor<AWeaponPickup>(AWeaponPickup::StaticClass(),DropLocation,GetActorRotation());if(Drop){Drop->ConfigureWeaponClass(Old->GetClass());Drop->Mesh->AddImpulse((GetActorForwardVector()*350.f+FVector(0.f,0.f,180.f))*Drop->Mesh->GetMass());Drop->Mesh->AddAngularImpulseInDegrees(FVector(0.f,180.f,360.f),NAME_None,true);}Old->Destroy();}WeaponSlots[ActiveWeaponSlot]=W;}EquippedWeapon=W;OnRep_Weapon();}
void AShooterCharacter::SwitchWeapon(float V){if(FMath::Abs(V)>.1f)ServerSwitchWeapon(V>0?1:-1);}bool AShooterCharacter::ServerSwitchWeapon_Validate(int32 D){return D==1||D==-1;}void AShooterCharacter::ServerSwitchWeapon_Implementation(int32 D){if(WeaponSlots.Num()<2)return;ActiveWeaponSlot=(ActiveWeaponSlot+D+WeaponSlots.Num())%WeaponSlots.Num();EquippedWeapon=WeaponSlots[ActiveWeaponSlot];OnRep_Weapon();}
void AShooterCharacter::RefreshAnimationState()
{
	const TSubclassOf<UAnimInstance> DesiredAnimClass=EquippedWeapon?ArmedAnimClass:UnarmedAnimClass;
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	if(DesiredAnimClass&&GetMesh()->GetAnimClass()!=DesiredAnimClass)GetMesh()->SetAnimInstanceClass(DesiredAnimClass);
}

void AShooterCharacter::SetActiveWeaponSlotForLoad(int32 SlotIndex)
{
	if(!HasAuthority()||!WeaponSlots.IsValidIndex(SlotIndex))return;
	ActiveWeaponSlot=SlotIndex;
	EquippedWeapon=WeaponSlots[ActiveWeaponSlot];
	OnRep_Weapon();
}

int32 AShooterCharacter::AddAmmunition(int32 Amount)
{
	if(!HasAuthority()||Amount<=0)return 0;
	int32 Remaining=Amount;
	if(EquippedWeapon)Remaining-=EquippedWeapon->AddReserveAmmo(Remaining);
	for(AWeaponBase* Weapon:WeaponSlots)
	{
		if(Remaining<=0)break;
		if(Weapon&&Weapon!=EquippedWeapon)Remaining-=Weapon->AddReserveAmmo(Remaining);
	}
	return Amount-Remaining;
}

void AShooterCharacter::AddExperience(int32 Amount)
{
	if(!HasAuthority()||Amount<=0)return;
	Experience+=Amount;
	TotalExperience+=Amount;
	while(Experience>=GetExperienceForNextLevel())
	{
		Experience-=GetExperienceForNextLevel();
		++CharacterLevel;
		UE_LOG(LogTemp,Display,TEXT("Player %s reached level %d"),*GetName(),CharacterLevel);
	}
}

void AShooterCharacter::OnRep_Weapon()
{
	RefreshAnimationState();
	FirstPersonBody->SetHiddenInGame(true);
	FirstPersonArms->SetHiddenInGame(true);

	USkeletalMeshComponent* CharacterMesh=GetMesh();
	if(!CharacterMesh||!CharacterMesh->DoesSocketExist(EquippedWeaponSocketName)||!CharacterMesh->DoesSocketExist(StowedWeaponSocketName))
	{
		UE_LOG(LogTemp,Error,TEXT("Weapon sockets are missing on %s (required: %s and %s)"),
			*GetNameSafe(CharacterMesh),*EquippedWeaponSocketName.ToString(),*StowedWeaponSocketName.ToString());
		return;
	}

	for(AWeaponBase* Weapon:WeaponSlots)
	{
		if(!Weapon)continue;

		const bool bIsEquipped=Weapon==EquippedWeapon;
		const FName TargetSocket=bIsEquipped?EquippedWeaponSocketName:StowedWeaponSocketName;
		Weapon->SetOwner(this);
		Weapon->AttachToComponent(CharacterMesh,FAttachmentTransformRules::SnapToTargetIncludingScale,TargetSocket);
		Weapon->SetActorRelativeTransform(FTransform::Identity);

		// The same editor-authored _X weapon mesh is used in first and third person.
		Weapon->Mesh->SetOwnerNoSee(false);
		Weapon->Mesh->SetVisibility(true,true);
		Weapon->FirstPersonMesh->SetVisibility(false,true);
	}
}
void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AShooterCharacter,EquippedWeapon);DOREPLIFETIME(AShooterCharacter,WeaponSlots);DOREPLIFETIME(AShooterCharacter,ActiveWeaponSlot);DOREPLIFETIME(AShooterCharacter,bIsAiming);DOREPLIFETIME(AShooterCharacter,CharacterLevel);DOREPLIFETIME(AShooterCharacter,Experience);DOREPLIFETIME(AShooterCharacter,TotalExperience);}
