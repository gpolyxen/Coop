#include "ShooterCharacter.h"
#include "WeaponBase.h"
#include "PickupActor.h"
#include "SaveBed.h"
#include "BuildableStructure.h"
#include "StorageChest.h"
#include "ShooterGameInstance.h"
#include "ShooterPlayerController.h"
#include "InventoryComponent.h"
#include "HealthArmorComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/LightComponent.h"
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
#include "P9Weapon.h"
#include "AK74UWeapon.h"
#include "WeaponPickup.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "NavigationInvokerComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "EngineUtils.h"

namespace
{
	struct FBuildSnapCandidate
	{
		FBuildSnapCandidate():Location(FVector::ZeroVector),Rotation(FRotator::ZeroRotator),Parent(nullptr){}
		FBuildSnapCandidate(const FVector& InLocation,const FRotator& InRotation,ABuildableStructure* InParent):Location(InLocation),Rotation(InRotation),Parent(InParent){}
		FVector Location;FRotator Rotation;ABuildableStructure* Parent;
	};
	void AddBuildSnapCandidates(EBuildPieceType Piece,ABuildableStructure* Target,int32 QuarterTurns,TArray<FBuildSnapCandidate>& Out)
	{
		if(!Target||Target->IsCollapsing())return;
		const FVector T=Target->GetActorLocation();
		const FVector Right=Target->GetActorRotation().RotateVector(FVector::RightVector).GetSafeNormal2D();
		const FVector Forward=Target->GetActorRotation().RotateVector(FVector::ForwardVector).GetSafeNormal2D();
		if(Piece==EBuildPieceType::WoodPillar)
		{
			if(AWoodFloor* Floor=Cast<AWoodFloor>(Target))
			{
				Out.Add({T,Floor->GetActorRotation(),Target});
				Out.Add({T-FVector(0.f,0.f,220.f),Floor->GetActorRotation(),Target});
			}
			return;
		}
		if(Piece==EBuildPieceType::WoodFloor)
		{
			if(Target->IsA<AWoodFloor>())
			{
				for(const FVector& Offset:{Right*300.f,-Right*300.f,Forward*300.f,-Forward*300.f})Out.Add({T+Offset,Target->GetActorRotation(),Target});
			}
			else if(Target->IsA<AWoodWall>()||Target->IsA<AWoodGate>())
			{
				for(float Side:{-1.f,1.f})Out.Add({T+Forward*(150.f*Side)+FVector(0,0,220.f),Target->GetActorRotation(),Target});
			}
			else if(Target->IsA<AWoodStairs>())
			{
				Out.Add({T+Right*300.f+FVector(0,0,220.f),Target->GetActorRotation(),Target});
			}
			return;
		}
		if(Piece==EBuildPieceType::WoodStairs)
		{
			if(!(Target->IsA<AWoodWall>()||Target->IsA<AWoodGate>()))return;
			for(float Side:{-1.f,1.f})
			{
				const float RotatedSide=(QuarterTurns&1)?-Side:Side;const FVector Along=-Forward*RotatedSide;
				FRotator R=Along.Rotation();R.Pitch=0.f;R.Roll=0.f;R.Yaw-=90.f;
				Out.Add({T+Forward*(150.f*RotatedSide),R,Target});
			}
			return;
		}
		if(Piece==EBuildPieceType::WoodWall||Piece==EBuildPieceType::WoodGate)
		{
			if(Target->IsA<AWoodFloor>())
			{
				Out.Add({T+Forward*150.f,Target->GetActorRotation(),Target});Out.Add({T-Forward*150.f,Target->GetActorRotation(),Target});
				FRotator Across=Target->GetActorRotation();Across.Yaw+=90.f;
				Out.Add({T+Right*150.f,Across,Target});Out.Add({T-Right*150.f,Across,Target});return;
			}
			if(Target->IsA<AWoodWall>()||Target->IsA<AWoodGate>())
			{
				FRotator R=Target->GetActorRotation();R.Yaw+=QuarterTurns*90.f;const FVector Along=R.RotateVector(FVector::RightVector).GetSafeNormal2D();
				for(const FVector& End:{T+Right*150.f,T-Right*150.f})for(float Side:{-1.f,1.f})
				{
					const FVector Candidate=End+Along*(150.f*Side);if(FVector::Dist2D(Candidate,T)>200.f)Out.Add({Candidate,R,Target});
				}
			}
		}
	}
	bool IsBuildLocationBlocked(UWorld* World,EBuildPieceType Piece,const FVector& Location,const ABuildableStructure* Parent)
	{
		if(!World)return true;
		// Stairs deliberately occupy the same modular bay as their supporting wall
		// and nearby landing. A centre-distance test cannot distinguish that valid
		// arrangement from an obstruction, so stair clearance is handled by the
		// authored snap points and component collision instead.
		if(Piece==EBuildPieceType::WoodStairs)return false;
		for(TActorIterator<ABuildableStructure> It(World);It;++It)
		{
			if(*It==Parent||It->IsCollapsing())continue;const FVector Other=It->GetActorLocation();
			if(Piece==EBuildPieceType::WoodPillar&&It->IsA<AWoodPillar>()&&FMath::Abs(Location.Z-Other.Z)<80.f&&FVector::Dist2D(Location,Other)<80.f)return true;
			if(Piece==EBuildPieceType::WoodFloor&&It->IsA<AWoodStairs>()&&FMath::Abs(Location.Z-(Other.Z+220.f))<90.f&&FVector::Dist2D(Location,Other)<210.f)return true;
			if(Piece==EBuildPieceType::WoodStairs&&FMath::Abs(Location.Z-Other.Z)<180.f&&FVector::Dist2D(Location,Other)<125.f)return true;
		}
		return false;
	}
}

AShooterCharacter::AShooterCharacter()
{
	bReplicates=true;
	bUseControllerRotationPitch=false;bUseControllerRotationYaw=true;bUseControllerRotationRoll=false;
	CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));CameraBoom->SetupAttachment(RootComponent);CameraBoom->TargetArmLength=0.f;CameraBoom->bDoCollisionTest=false;
	Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("CharacterCamera"));Camera->SetupAttachment(GetMesh(),TEXT("head"));Camera->SetRelativeLocation(FVector::ZeroVector);Camera->SetRelativeRotation(FRotator::ZeroRotator);Camera->bUsePawnControlRotation=true;Camera->FieldOfView=90.f;Camera->PostProcessSettings.bOverride_MotionBlurAmount=true;Camera->PostProcessSettings.MotionBlurAmount=0.f;
	FirstPersonBody=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonBody"));FirstPersonBody->SetupAttachment(RootComponent);FirstPersonBody->SetRelativeLocation(FVector(0.f,0.f,-90.f));FirstPersonBody->SetRelativeRotation(FRotator(0.f,-90.f,0.f));FirstPersonBody->SetOnlyOwnerSee(true);FirstPersonBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);FirstPersonBody->CastShadow=false;
	// The legacy camera carries a non-unit scale copied from BP_Player. Keep a
	// scale-neutral root so dedicated FPS rigs retain their authored dimensions.
	FirstPersonRigRoot=CreateDefaultSubobject<USceneComponent>(TEXT("FirstPersonRigRoot"));FirstPersonRigRoot->SetupAttachment(Camera);FirstPersonRigRoot->SetAbsolute(false,false,true);
	FirstPersonArms=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));FirstPersonArms->SetupAttachment(FirstPersonRigRoot);FirstPersonArms->SetRelativeLocation(FVector(-20.f,12.f,-105.f));FirstPersonArms->SetRelativeRotation(FRotator(0.f,-10.f,0.f));FirstPersonArms->SetRelativeScale3D(FVector(.65f));FirstPersonArms->SetOnlyOwnerSee(true);FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);FirstPersonArms->CastShadow=false;
	FirstPersonArms->SetIsReplicated(false);
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
	ApplyUnlockedSkillEffects();
	if(HasAuthority()&&Inventory&&Inventory->Items.Num()==0){Inventory->MaxWeight=FMath::Max(Inventory->MaxWeight,500.f);Inventory->AddItem(TEXT("Wood"),250);Inventory->AddItem(TEXT("Rope"),60);}
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
	const bool bCaptureDefaultWeapon=FParse::Param(FCommandLine::Get(),TEXT("CodexCapture"));
	const bool bCaptureP9=FParse::Param(FCommandLine::Get(),TEXT("CodexCaptureP9"));
	const bool bCaptureAK74U=FParse::Param(FCommandLine::Get(),TEXT("CodexCaptureAK74U"));
	const bool bCaptureAK74UHip=FParse::Param(FCommandLine::Get(),TEXT("CodexCaptureAK74UHip"));
	if((bCaptureDefaultWeapon||bCaptureP9||bCaptureAK74U||bCaptureAK74UHip)&&IsLocallyControlled())
	{
		bIsAiming=!bCaptureAK74UHip;
		if(HasAuthority())EquipWeapon((bCaptureAK74U||bCaptureAK74UHip)?AAK74UWeapon::StaticClass():(bCaptureP9?AP9Weapon::StaticClass():AKA47Rifle::StaticClass()));
		FTimerHandle CaptureTimer;
		GetWorldTimerManager().SetTimer(CaptureTimer,this,&AShooterCharacter::CaptureDiagnosticScreenshot,3.f,false);
	}
	RestoreLocalGameplayInput();
}

void AShooterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	RestoreLocalGameplayInput();
}

void AShooterCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	RestoreLocalGameplayInput();
}

void AShooterCharacter::RestoreLocalGameplayInput()
{
	if(!IsLocallyControlled())return;
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())if(GI->IsPauseMenuOpen())return;
	if(APlayerController* PC=Cast<APlayerController>(Controller))
	{
		PC->bShowMouseCursor=false;
		PC->bEnableClickEvents=false;
		PC->bEnableMouseOverEvents=false;
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

void AShooterCharacter::StopGameplayActionsForMenu()
{
	StopFire();
	StopAim();
	SprintReleased();
}

bool AShooterCharacter::IsDead()const{return Health&&Health->IsDead();}

void AShooterCharacter::HandleDeath(){StopFire();bIsAiming=false;if(Camera)Camera->SetFieldOfView(HipFOV);if(UCharacterMovementComponent* Movement=GetCharacterMovement()){Movement->StopMovementImmediately();Movement->DisableMovement();}if(IsLocallyControlled())if(APlayerController* PC=Cast<APlayerController>(Controller))DisableInput(PC);UE_LOG(LogTemp,Display,TEXT("Player %s died"),*GetName());}
void AShooterCharacter::CaptureDiagnosticScreenshot()
{
	FScreenshotRequest::RequestScreenshot(TEXT("CodexWeaponView.png"),false,false);
}
void AShooterCharacter::Tick(float D)
{
	Super::Tick(D);
	if(HasAuthority()&&!IsDead())
	{
		if(HasSkill(EShooterSkill::Marathon))Stamina=MaxStamina;
		else
		{
			const bool bDraining=bWantsToSprint&&GetVelocity().Size2D()>10.f&&GetCharacterMovement()->IsMovingOnGround();
			if(bDraining){Stamina=FMath::Max(0.f,Stamina-SprintStaminaPerSecond*D);LastStaminaUseTime=GetWorld()->GetTimeSeconds();}
			else if(GetWorld()->GetTimeSeconds()-LastStaminaUseTime>=StaminaRecoveryDelay)Stamina=FMath::Min(MaxStamina,Stamina+StaminaRecoveryPerSecond*D);
			if(Stamina<=0.f)bWantsToSprint=false;
		}
		GetCharacterMovement()->MaxWalkSpeed=bWantsToSprint&&(HasSkill(EShooterSkill::Marathon)||Stamina>0.f)?SprintSpeed*(HasSkill(EShooterSkill::Marathon)?1.15f:1.f):WalkSpeed;
	}
	else if(!HasAuthority()&&Stamina<=0.f)GetCharacterMovement()->MaxWalkSpeed=WalkSpeed;
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
	UpdateDedicatedFirstPersonRigTransform(D);
	const float Recovery=EquippedWeapon?EquippedWeapon->RecoilRecoverySpeed:12.f;
	RecoilPitchTarget=FMath::FInterpTo(RecoilPitchTarget,0.f,D,Recovery);RecoilYawTarget=FMath::FInterpTo(RecoilYawTarget,0.f,D,Recovery);RecoilKickTarget=FMath::FInterpTo(RecoilKickTarget,0.f,D,Recovery);
	const FVector LocalVelocity=GetActorTransform().InverseTransformVectorNoScale(GetVelocity());
	const float Direction=FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y,LocalVelocity.X));
	auto UpdateAnim=[this,Direction](UAnimInstance* Anim){if(!Anim)return;auto SetFloat=[Anim](const TCHAR* Name,float Value){if(FFloatProperty* P=FindFProperty<FFloatProperty>(Anim->GetClass(),Name))P->SetPropertyValue_InContainer(Anim,Value);};auto SetBool=[Anim](const TCHAR* Name,bool Value){if(FBoolProperty* P=FindFProperty<FBoolProperty>(Anim->GetClass(),Name))P->SetPropertyValue_InContainer(Anim,Value);};const bool bHasWeapon=EquippedWeapon!=nullptr;SetFloat(TEXT("Speed"),GetVelocity().Size2D());SetFloat(TEXT("Direction"),Direction);SetBool(TEXT("IsAiming"),bHasWeapon&&bIsAiming);SetBool(TEXT("IsAiming?"),bHasWeapon&&bIsAiming);SetBool(TEXT("Is Aiming?"),bHasWeapon&&bIsAiming);SetBool(TEXT("HasWeapon"),bHasWeapon);SetBool(TEXT("Has Weapon?"),bHasWeapon);SetBool(TEXT("Crouching"),bIsCrouched);SetBool(TEXT("Jumping"),GetCharacterMovement()->IsFalling());FObjectPropertyBase* PlayerProperty=FindFProperty<FObjectPropertyBase>(Anim->GetClass(),TEXT("As BP Player"));if(!PlayerProperty)PlayerProperty=FindFProperty<FObjectPropertyBase>(Anim->GetClass(),TEXT("AsBPPlayer"));if(PlayerProperty)PlayerProperty->SetObjectPropertyValue_InContainer(Anim,this);if(UFunction* CalculateRotation=Anim->FindFunction(TEXT("CalculateRotation")))Anim->ProcessEvent(CalculateRotation,nullptr);};UpdateAnim(GetMesh()->GetAnimInstance());
	UpdateDedicatedFirstPersonRigAnimation();
	if(IsBuilding())UpdateBuildPreview();
}
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* I){Super::SetupPlayerInputComponent(I);check(I);I->BindAxis("MoveForward",this,&AShooterCharacter::MoveForward);I->BindAxis("MoveRight",this,&AShooterCharacter::MoveRight);I->BindAxis("SwitchWeapon",this,&AShooterCharacter::SwitchWeapon);I->BindAxis("Turn",this,&APawn::AddControllerYawInput);I->BindAxis("LookUp",this,&APawn::AddControllerPitchInput);I->BindAxis("TurnRate",this,&AShooterCharacter::TurnAtRate);I->BindAxis("LookUpRate",this,&AShooterCharacter::LookUpAtRate);I->BindAction("Jump",IE_Pressed,this,&AShooterCharacter::JumpPressed);I->BindAction("Jump",IE_Released,this,&AShooterCharacter::JumpReleased);I->BindAction("Crouch",IE_Pressed,this,&AShooterCharacter::ToggleCrouch);I->BindAction("Aim",IE_Pressed,this,&AShooterCharacter::StartAim);I->BindAction("Aim",IE_Released,this,&AShooterCharacter::StopAim);I->BindAction("Fire",IE_Pressed,this,&AShooterCharacter::StartFire);I->BindAction("Fire",IE_Released,this,&AShooterCharacter::StopFire);I->BindAction("Reload",IE_Pressed,this,&AShooterCharacter::Reload);I->BindAction("Interact",IE_Pressed,this,&AShooterCharacter::Interact);I->BindAction("Sprint",IE_Pressed,this,&AShooterCharacter::SprintPressed);I->BindAction("Sprint",IE_Released,this,&AShooterCharacter::SprintReleased);I->BindAction("CancelBuild",IE_Pressed,this,&AShooterCharacter::CancelBuildMode);}
void AShooterCharacter::MoveForward(float V){if(Controller&&V!=0)AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V);}void AShooterCharacter::MoveRight(float V){if(Controller&&V!=0)AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V);}
void AShooterCharacter::TurnAtRate(float V){AddControllerYawInput(V*BaseTurnRate*GetWorld()->GetDeltaSeconds());}void AShooterCharacter::LookUpAtRate(float V){AddControllerPitchInput(V*BaseTurnRate*GetWorld()->GetDeltaSeconds());}
void AShooterCharacter::ToggleCrouch(){bIsCrouched?UnCrouch():Crouch();}
void AShooterCharacter::StartAim(){if(IsBuilding()||!EquippedWeapon)return;bIsAiming=true;if(!HasAuthority())ServerSetAiming(true);}void AShooterCharacter::StopAim(){bIsAiming=false;if(!HasAuthority())ServerSetAiming(false);}bool AShooterCharacter::ServerSetAiming_Validate(bool){return true;}void AShooterCharacter::ServerSetAiming_Implementation(bool bNewAiming){bIsAiming=bNewAiming&&EquippedWeapon;}
void AShooterCharacter::StartFire()
{
	if(IsBuilding()){ConfirmBuildPlacement();return;}
	if(IsDead()||!EquippedWeapon)return;
	FireOnce();
	if(!EquippedWeapon->bAutomatic)return;
	const float Interval=60.f/FMath::Max(1.f,EquippedWeapon->Stats.RoundsPerMinute);
	GetWorldTimerManager().SetTimer(FireTimer,this,&AShooterCharacter::FireOnce,Interval,true,Interval);
}

void AShooterCharacter::StopFire(){GetWorldTimerManager().ClearTimer(FireTimer);}

bool AShooterCharacter::CanCraftBed()const{return Inventory&&Inventory->HasItems(TEXT("Wood"),10,TEXT("Rope"),10);}
bool AShooterCharacter::CanCraftMedkit()const{return Inventory&&Inventory->HasItems(TEXT("Medicine"),1,TEXT("Bandage"),2);}
void AShooterCharacter::CraftMedkit()
{
	if(!HasAuthority()){ServerCraftMedkit();return;}
	if(!CanCraftMedkit())return;
	if(!Inventory->ConsumeItems(TEXT("Medicine"),1,TEXT("Bandage"),2))return;
	if(!Inventory->AddItem(TEXT("Medkit"),1)){Inventory->AddItem(TEXT("Medicine"),1);Inventory->AddItem(TEXT("Bandage"),2);return;}
	ShowLocalNotification(TEXT("СОЗДАНА АПТЕЧКА"),2.f);
}
void AShooterCharacter::UseMedkit()
{
	if(!HasAuthority()){ServerUseMedkit();return;}
	if(!Inventory||!Health||Health->Health>=Health->MaxHealth||Inventory->GetQuantity(TEXT("Medkit"))<1)return;
	if(Inventory->RemoveItem(TEXT("Medkit"),1))Health->Heal(50.f*GetHealingMultiplier());
}
bool AShooterCharacter::ServerCraftMedkit_Validate(){return true;}void AShooterCharacter::ServerCraftMedkit_Implementation(){CraftMedkit();}
bool AShooterCharacter::ServerUseMedkit_Validate(){return true;}void AShooterCharacter::ServerUseMedkit_Implementation(){UseMedkit();}
FString AShooterCharacter::GetSelectedBuildName()const
{
	switch(SelectedBuildPiece){case EBuildPieceType::Bed:return TEXT("КРОВАТЬ");case EBuildPieceType::WoodChest:return TEXT("СУНДУК");case EBuildPieceType::WoodWall:return TEXT("ДЕРЕВЯННАЯ СТЕНА");case EBuildPieceType::WoodGate:return TEXT("ДЕРЕВЯННЫЕ ВОРОТА");case EBuildPieceType::WoodFloor:return TEXT("ДЕРЕВЯННЫЙ ПОЛ");case EBuildPieceType::WoodStairs:return TEXT("ДЕРЕВЯННАЯ ЛЕСТНИЦА");case EBuildPieceType::WoodPillar:return TEXT("ДЕРЕВЯННАЯ КОЛОННА");default:return FString();}
}
void AShooterCharacter::BeginBuildPlacement(EBuildPieceType PieceType)
{
	CancelBuildMode();if(!Inventory||PieceType==EBuildPieceType::None)return;
	const int32 RequiredWood=PieceType==EBuildPieceType::Bed?10:PieceType==EBuildPieceType::WoodChest?20:PieceType==EBuildPieceType::WoodWall?6:PieceType==EBuildPieceType::WoodGate?12:PieceType==EBuildPieceType::WoodFloor?5:PieceType==EBuildPieceType::WoodPillar?4:8;
	const int32 RequiredRope=PieceType==EBuildPieceType::Bed?10:PieceType==EBuildPieceType::WoodGate?4:0;
	const int32 RequiredLeather=PieceType==EBuildPieceType::WoodChest?5:0;
	const bool bAffordable=Inventory->GetQuantity(TEXT("Wood"))>=RequiredWood&&Inventory->GetQuantity(TEXT("Rope"))>=RequiredRope&&Inventory->GetQuantity(TEXT("Leather"))>=RequiredLeather;
	if(!bAffordable){ShowLocalNotification(TEXT("НЕДОСТАТОЧНО МАТЕРИАЛОВ"));return;}
	SelectedBuildPiece=PieceType;StopFire();StopAim();
	FActorSpawnParameters PreviewParameters;PreviewParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* PreviewClass=PieceType==EBuildPieceType::Bed?ASaveBed::StaticClass():PieceType==EBuildPieceType::WoodChest?AStorageChest::StaticClass():PieceType==EBuildPieceType::WoodWall?AWoodWall::StaticClass():PieceType==EBuildPieceType::WoodGate?AWoodGate::StaticClass():PieceType==EBuildPieceType::WoodFloor?AWoodFloor::StaticClass():PieceType==EBuildPieceType::WoodPillar?AWoodPillar::StaticClass():AWoodStairs::StaticClass();
	BuildPreview=GetWorld()->SpawnActor<AActor>(PreviewClass,GetActorLocation(),GetActorRotation(),PreviewParameters);
	if(BuildPreview){BuildPreview->SetReplicates(false);BuildPreview->SetActorEnableCollision(false);if(ABuildableStructure* Structure=Cast<ABuildableStructure>(BuildPreview))Structure->SetConstructionPreview(true);TArray<ULightComponent*> Lights;BuildPreview->GetComponents<ULightComponent>(Lights);for(ULightComponent* Light:Lights)if(Light)Light->SetVisibility(false);}
	BuildRotationQuarterTurns=0;UpdateBuildPreview();ShowLocalNotification(TEXT("ЛКМ — ПОСТАВИТЬ    R — ПОВЕРНУТЬ 90°    ПКМ — ОТМЕНА"),8.f);
}
void AShooterCharacter::UpdateBuildPreview()
{
	if(!IsBuilding()||!Camera||!BuildPreview)return;
	FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(BuildPreview),false,this);Query.AddIgnoredActor(BuildPreview);
	const FVector Start=Camera->GetComponentLocation(),End=Start+Camera->GetForwardVector()*850.f;
	const bool bHasHit=GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Query);
	const FVector AimLocation=bHasHit?Hit.ImpactPoint:End;
	FVector Location=AimLocation;FRotator Rotation(0.f,FMath::GridSnap(Controller?Controller->GetControlRotation().Yaw:0.f,90.f)+BuildRotationQuarterTurns*90.f,0.f);
	FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(BuildPreviewGround),false,this);GroundQuery.AddIgnoredActor(BuildPreview);for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)GroundQuery.AddIgnoredActor(*It);
	FCollisionObjectQueryParams GroundObjects;GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	FHitResult GroundHit;const bool bGroundHit=GetWorld()->LineTraceSingleByObjectType(GroundHit,Location+FVector(0,0,1500.f),Location-FVector(0,0,3000.f),GroundObjects,GroundQuery)&&GroundHit.ImpactNormal.Z>=.35f;
	if(bGroundHit)Location.Z=GroundHit.ImpactPoint.Z;
	bBuildPreviewValid=bGroundHit;
	if(SelectedBuildPiece==EBuildPieceType::Bed||SelectedBuildPiece==EBuildPieceType::WoodChest)
	{
		AWoodFloor* Floor=bHasHit?Cast<AWoodFloor>(Hit.GetActor()):nullptr;
		if(!Floor)for(TActorIterator<AWoodFloor> It(GetWorld());It;++It)if(FMath::Abs(Location.Z-It->GetActorLocation().Z)<300.f&&FVector::Dist2D(Location,It->GetActorLocation())<205.f){Floor=*It;break;}
		if(Floor){Location.Z=Floor->GetActorLocation().Z+12.f;bBuildPreviewValid=true;}
	}
	else
	{
		ABuildableStructure* DirectStructure=bHasHit?Cast<ABuildableStructure>(Hit.GetActor()):nullptr;
		float BestScore=FLT_MAX;
		FBuildSnapCandidate Best;
		bool bFoundSnap=false;
		for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)
		{
			if(*It==BuildPreview)continue;TArray<FBuildSnapCandidate> Candidates;AddBuildSnapCandidates(SelectedBuildPiece,*It,BuildRotationQuarterTurns,Candidates);
			for(const FBuildSnapCandidate& Candidate:Candidates)
			{
				if(SelectedBuildPiece==EBuildPieceType::WoodFloor&&(Candidate.Parent->IsA<AWoodWall>()||Candidate.Parent->IsA<AWoodGate>())&&AimLocation.Z<Candidate.Parent->GetActorLocation().Z+115.f)continue;
				const float CandidateDistanceSq=FVector::DistSquared(Location,Candidate.Location);const float Score=CandidateDistanceSq+(DirectStructure==*It?-FMath::Square(500.f):0.f);
				const float SnapSearchRadius=SelectedBuildPiece==EBuildPieceType::WoodStairs?800.f:550.f;
				if((DirectStructure==*It||CandidateDistanceSq<=FMath::Square(SnapSearchRadius))&&Score<BestScore&&!IsBuildLocationBlocked(GetWorld(),SelectedBuildPiece,Candidate.Location,Candidate.Parent)){BestScore=Score;Best=Candidate;bFoundSnap=true;}
			}
		}
		if(bFoundSnap){Location=Best.Location;Rotation=Best.Rotation;bBuildPreviewValid=true;}
	}
	if(IsBuildLocationBlocked(GetWorld(),SelectedBuildPiece,Location,nullptr))bBuildPreviewValid=false;
	BuildPreview->SetActorLocationAndRotation(Location,Rotation);
}
void AShooterCharacter::RotateBuildPreview(){if(IsBuilding()){BuildRotationQuarterTurns=(BuildRotationQuarterTurns+1)%4;UpdateBuildPreview();ShowLocalNotification(TEXT("ПОВОРОТ: 90°"),1.f);}}
void AShooterCharacter::CancelBuildMode(){if(IsBuilding()){SelectedBuildPiece=EBuildPieceType::None;bBuildPreviewValid=false;BuildRotationQuarterTurns=0;if(BuildPreview){BuildPreview->Destroy();BuildPreview=nullptr;}ShowLocalNotification(TEXT("СТРОИТЕЛЬСТВО ОТМЕНЕНО"));}}
void AShooterCharacter::ConfirmBuildPlacement()
{
	if(!IsBuilding()||!BuildPreview)return;
	const EBuildPieceType Piece=SelectedBuildPiece;const FVector Location=BuildPreview->GetActorLocation();const FRotator Rotation=BuildPreview->GetActorRotation();
	ServerPlaceBuildPiece(Piece,Location,Rotation);
}
bool AShooterCharacter::ServerPlaceBuildPiece_Validate(EBuildPieceType Piece,FVector_NetQuantize Location,FRotator){return Piece!=EBuildPieceType::None&&FVector::DistSquared(Location,GetActorLocation())<=FMath::Square(1600.f);}
void AShooterCharacter::ServerPlaceBuildPiece_Implementation(EBuildPieceType Piece,FVector_NetQuantize Location,FRotator Rotation)
{
	if(!Inventory)return;const int32 WoodCost=Piece==EBuildPieceType::Bed?10:Piece==EBuildPieceType::WoodChest?20:Piece==EBuildPieceType::WoodWall?6:Piece==EBuildPieceType::WoodGate?12:Piece==EBuildPieceType::WoodFloor?5:Piece==EBuildPieceType::WoodPillar?4:8;const int32 RopeCost=Piece==EBuildPieceType::Bed?10:(Piece==EBuildPieceType::WoodGate?4:0);const int32 LeatherCost=Piece==EBuildPieceType::WoodChest?5:0;
	if(Inventory->GetQuantity(TEXT("Wood"))<WoodCost||Inventory->GetQuantity(TEXT("Rope"))<RopeCost||Inventory->GetQuantity(TEXT("Leather"))<LeatherCost){UE_LOG(LogTemp,Warning,TEXT("Build rejected: insufficient materials for piece %d"),static_cast<int32>(Piece));ClientBuildPlacementResult(Piece,false,false);return;}
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ServerBuildPlacement),false,this);
	FVector FinalLocation=Location;FRotator FinalRotation=Rotation;ABuildableStructure* SnapParent=nullptr;
	if(Piece==EBuildPieceType::Bed||Piece==EBuildPieceType::WoodChest)
	{
		float BestDistanceSq=FMath::Square(230.f);
		for(TActorIterator<AWoodFloor> It(GetWorld());It;++It)
		{
			const float DistanceSq=FVector::DistSquared2D(FinalLocation,It->GetActorLocation());
			if(DistanceSq<BestDistanceSq&&FMath::Abs(FinalLocation.Z-It->GetActorLocation().Z)<80.f){BestDistanceSq=DistanceSq;FinalLocation.Z=It->GetActorLocation().Z+12.f;SnapParent=*It;}
		}
	}
	else
	{
		float BestDistanceSq=FMath::Square(95.f);
		for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)
		{
			TArray<FBuildSnapCandidate> Candidates;for(int32 Quarter=0;Quarter<4;++Quarter)AddBuildSnapCandidates(Piece,*It,Quarter,Candidates);
			for(const FBuildSnapCandidate& Candidate:Candidates)
			{
				const float DistanceSq=FVector::DistSquared(FinalLocation,Candidate.Location);if(DistanceSq<BestDistanceSq){BestDistanceSq=DistanceSq;FinalLocation=Candidate.Location;FinalRotation=Candidate.Rotation;SnapParent=Candidate.Parent;}
			}
		}
	}
	if(IsBuildLocationBlocked(GetWorld(),Piece,FinalLocation,SnapParent)){UE_LOG(LogTemp,Warning,TEXT("Build rejected: modular location blocked for piece %d at %s"),static_cast<int32>(Piece),*FinalLocation.ToCompactString());ClientBuildPlacementResult(Piece,false,true);return;}
	if(SnapParent)Query.AddIgnoredActor(SnapParent);
	else
	{
		FHitResult Ground;FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(ServerBuildGround),false,this);for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)GroundQuery.AddIgnoredActor(*It);FCollisionObjectQueryParams GroundObjects;GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
		if(!GetWorld()->LineTraceSingleByObjectType(Ground,FinalLocation+FVector(0,0,1500),FinalLocation-FVector(0,0,3000),GroundObjects,GroundQuery)||Ground.ImpactNormal.Z<.35f){UE_LOG(LogTemp,Warning,TEXT("Build rejected: no ground for piece %d at %s"),static_cast<int32>(Piece),*FinalLocation.ToCompactString());ClientBuildPlacementResult(Piece,false,true);return;}
		FinalLocation=Ground.ImpactPoint;
	}
	FActorSpawnParameters Parameters;Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* BuildClass=Piece==EBuildPieceType::Bed?ASaveBed::StaticClass():Piece==EBuildPieceType::WoodChest?AStorageChest::StaticClass():Piece==EBuildPieceType::WoodWall?AWoodWall::StaticClass():Piece==EBuildPieceType::WoodGate?AWoodGate::StaticClass():Piece==EBuildPieceType::WoodFloor?AWoodFloor::StaticClass():Piece==EBuildPieceType::WoodPillar?AWoodPillar::StaticClass():AWoodStairs::StaticClass();
	if(GetWorld()->SpawnActor<AActor>(BuildClass,FinalLocation,FinalRotation,Parameters))
	{
		UE_LOG(LogTemp,Display,TEXT("Build placed: piece %d at %s rotation %s snapped=%s"),static_cast<int32>(Piece),*FinalLocation.ToCompactString(),*FinalRotation.ToCompactString(),SnapParent?TEXT("true"):TEXT("false"));
		Inventory->RemoveItem(TEXT("Wood"),WoodCost);if(RopeCost>0)Inventory->RemoveItem(TEXT("Rope"),RopeCost);if(LeatherCost>0)Inventory->RemoveItem(TEXT("Leather"),LeatherCost);
		const bool bCanContinue=Inventory->GetQuantity(TEXT("Wood"))>=WoodCost&&Inventory->GetQuantity(TEXT("Rope"))>=RopeCost&&Inventory->GetQuantity(TEXT("Leather"))>=LeatherCost;
		ClientBuildPlacementResult(Piece,true,bCanContinue);
	}
	else ClientBuildPlacementResult(Piece,false,true);
}
void AShooterCharacter::ClientBuildPlacementResult_Implementation(EBuildPieceType Piece,bool bPlaced,bool bCanContinue)
{
	if(!bPlaced)
	{
		if(!bCanContinue){if(BuildPreview){BuildPreview->Destroy();BuildPreview=nullptr;}SelectedBuildPiece=EBuildPieceType::None;bBuildPreviewValid=false;ShowLocalNotification(TEXT("МАТЕРИАЛЫ ЗАКОНЧИЛИСЬ"),3.f);}
		else ShowLocalNotification(TEXT("ЗДЕСЬ НЕЛЬЗЯ ПОСТАВИТЬ"),2.f);
		return;
	}
	if(SelectedBuildPiece!=Piece)return;
	if(!bCanContinue)
	{
		if(BuildPreview){BuildPreview->Destroy();BuildPreview=nullptr;}SelectedBuildPiece=EBuildPieceType::None;bBuildPreviewValid=false;ShowLocalNotification(TEXT("ПОСТРОЕНО. МАТЕРИАЛЫ ЗАКОНЧИЛИСЬ"),3.f);return;
	}
	UpdateBuildPreview();ShowLocalNotification(TEXT("ПОСТРОЕНО — МОЖНО СТАВИТЬ СЛЕДУЮЩИЙ ЭЛЕМЕНТ"),1.5f);
}

void AShooterCharacter::FireOnce()
{
	if(IsDead()||!EquippedWeapon||!EquippedWeapon->CanFire())return;
	const FVector ViewStart=Camera->GetComponentLocation(),ViewEnd=ViewStart+Camera->GetForwardVector()*100000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("WeaponAim"),true,this);
	Params.AddIgnoredActor(EquippedWeapon);
	const bool bHit=GetWorld()->LineTraceSingleByChannel(Hit,ViewStart,ViewEnd,ECC_Visibility,Params);
	const FVector AimPoint=bHit?Hit.ImpactPoint:ViewEnd;
	if(!EquippedWeapon->Fire(AimPoint))return;
	PlayDedicatedFirstPersonRigAction(EquippedWeapon->FirstPersonRigFireAnimation);
	const float RecoilMultiplier=GetRecoilMultiplier();
	RecoilPitchTarget=FMath::Clamp(RecoilPitchTarget+EquippedWeapon->RecoilPitch*RecoilMultiplier,0.f,4.f);
	RecoilYawTarget=FMath::Clamp(RecoilYawTarget+FMath::FRandRange(-EquippedWeapon->RecoilYaw,EquippedWeapon->RecoilYaw)*RecoilMultiplier,-1.5f,1.5f);
	RecoilKickTarget=FMath::Clamp(RecoilKickTarget+EquippedWeapon->RecoilKickback,0.f,5.f);
	if(Controller)
	{
		FRotator ControlRotation=Controller->GetControlRotation();
		ControlRotation.Pitch=FMath::ClampAngle(ControlRotation.Pitch-EquippedWeapon->RecoilPitch*RecoilMultiplier,-80.f,80.f);
		ControlRotation.Yaw+=FMath::FRandRange(-EquippedWeapon->RecoilYaw,EquippedWeapon->RecoilYaw)*RecoilMultiplier;
		Controller->SetControlRotation(ControlRotation);
	}
}

void AShooterCharacter::ResetFirstPersonArmsAnimation()
{
	if(IsDedicatedFirstPersonRigActive())UpdateDedicatedFirstPersonRigAnimation();
	else if(FirstPersonIdleAnimation)FirstPersonArms->PlayAnimation(FirstPersonIdleAnimation,true);
}

void AShooterCharacter::Reload()
{
	if(IsBuilding()){RotateBuildPreview();return;}
	if(IsDead())return;
	StopFire();
	if(!EquippedWeapon)return;
	const bool bCanStartReload=!EquippedWeapon->bIsReloading
		&&EquippedWeapon->AmmoInMagazine<EquippedWeapon->Stats.MagazineSize
		&&EquippedWeapon->ReserveAmmo>0;
	EquippedWeapon->Reload();
	if(bCanStartReload)PlayDedicatedFirstPersonRigAction(EquippedWeapon->FirstPersonRigReloadAnimation);
}

void AShooterCharacter::SprintPressed(){if(IsDead())return;bWantsToSprint=HasSkill(EShooterSkill::Marathon)||Stamina>0.f;if(bWantsToSprint)GetCharacterMovement()->MaxWalkSpeed=SprintSpeed*(HasSkill(EShooterSkill::Marathon)?1.15f:1.f);if(!HasAuthority())ServerSetSprinting(bWantsToSprint);}
void AShooterCharacter::SprintReleased(){bWantsToSprint=false;GetCharacterMovement()->MaxWalkSpeed=WalkSpeed;if(!HasAuthority())ServerSetSprinting(false);}
bool AShooterCharacter::ServerSetSprinting_Validate(bool){return true;}void AShooterCharacter::ServerSetSprinting_Implementation(bool bNewSprinting){bWantsToSprint=bNewSprinting&&(HasSkill(EShooterSkill::Marathon)||Stamina>0.f);}
void AShooterCharacter::JumpPressed(){if(IsDead()||GetCharacterMovement()->IsFalling())return;if(HasAuthority())ServerTryJump_Implementation();else ServerTryJump();}
void AShooterCharacter::JumpReleased(){StopJumping();}
bool AShooterCharacter::ServerTryJump_Validate(){return true;}void AShooterCharacter::ServerTryJump_Implementation(){if(IsDead()||GetCharacterMovement()->IsFalling())return;if(!HasSkill(EShooterSkill::Marathon)){if(Stamina<JumpStaminaCost)return;Stamina-=JumpStaminaCost;LastStaminaUseTime=GetWorld()->GetTimeSeconds();}Jump();}

bool AShooterCharacter::IsDedicatedFirstPersonRigActive()const
{
	return IsLocallyControlled()&&EquippedWeapon&&EquippedWeapon->bUseDedicatedFirstPersonRig
		&&EquippedWeapon->FirstPersonRigMesh;
}

void AShooterCharacter::ConfigureDedicatedFirstPersonRig()
{
	const bool bWasSameWeapon=FirstPersonRigWeapon==EquippedWeapon;
	GetWorldTimerManager().ClearTimer(FirstPersonRigAnimationTimer);
	bFirstPersonRigActionPlaying=false;
	bFirstPersonRigLooping=false;
	CurrentFirstPersonRigAnimation=nullptr;

	const bool bActive=IsDedicatedFirstPersonRigActive();
	FirstPersonArms->SetHiddenInGame(!bActive);
	FirstPersonArms->SetVisibility(bActive,true);
	GetMesh()->SetOwnerNoSee(bActive&&EquippedWeapon->bHideOwnerCharacterMeshWhenRigActive);
	if(!bActive)
	{
		FirstPersonRigWeapon=nullptr;
		return;
	}

	FirstPersonRigWeapon=EquippedWeapon;
	FirstPersonArms->SetSkeletalMesh(EquippedWeapon->FirstPersonRigMesh);
	FirstPersonArms->ShowAllMaterialSections(0);
	for(const int32 MaterialSlot:EquippedWeapon->HiddenFirstPersonRigMaterialSlots)
		FirstPersonArms->ShowMaterialSection(MaterialSlot,MaterialSlot,false,0);
	FirstPersonArms->SetRelativeLocation(EquippedWeapon->FirstPersonRigLocation);
	FirstPersonArms->SetRelativeRotation(EquippedWeapon->FirstPersonRigRotation);
	FirstPersonArms->SetRelativeScale3D(EquippedWeapon->FirstPersonRigScale);
	FirstPersonArms->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	if(!bWasSameWeapon&&EquippedWeapon->FirstPersonRigDrawAnimation)
		PlayDedicatedFirstPersonRigAction(EquippedWeapon->FirstPersonRigDrawAnimation);
	else UpdateDedicatedFirstPersonRigAnimation();
}

void AShooterCharacter::PlayDedicatedFirstPersonRigAction(UAnimSequence* Animation)
{
	if(!IsDedicatedFirstPersonRigActive()||!Animation)return;
	GetWorldTimerManager().ClearTimer(FirstPersonRigAnimationTimer);
	bFirstPersonRigActionPlaying=true;
	bFirstPersonRigLooping=false;
	CurrentFirstPersonRigAnimation=Animation;
	FirstPersonArms->PlayAnimation(Animation,false);
	GetWorldTimerManager().SetTimer(FirstPersonRigAnimationTimer,this,
		&AShooterCharacter::FinishDedicatedFirstPersonRigAction,
		FMath::Max(.05f,Animation->GetPlayLength()),false);
}

void AShooterCharacter::UpdateDedicatedFirstPersonRigTransform(float DeltaSeconds)
{
	if(!IsDedicatedFirstPersonRigActive())return;
	const bool bUseAimTransform=bIsAiming&&EquippedWeapon->bUseFirstPersonRigAimTransform;
	const FVector DesiredLocation=bUseAimTransform
		?EquippedWeapon->FirstPersonRigAimLocation:EquippedWeapon->FirstPersonRigLocation;
	const FRotator DesiredRotation=bUseAimTransform
		?EquippedWeapon->FirstPersonRigAimRotation:EquippedWeapon->FirstPersonRigRotation;
	const FVector DesiredScale=bUseAimTransform
		?EquippedWeapon->FirstPersonRigAimScale:EquippedWeapon->FirstPersonRigScale;
	const float Speed=EquippedWeapon->FirstPersonRigAimInterpSpeed;
	FirstPersonArms->SetRelativeLocation(FMath::VInterpTo(
		FirstPersonArms->GetRelativeLocation(),DesiredLocation,DeltaSeconds,Speed));
	FirstPersonArms->SetRelativeRotation(FMath::RInterpTo(
		FirstPersonArms->GetRelativeRotation(),DesiredRotation,DeltaSeconds,Speed));
	FirstPersonArms->SetRelativeScale3D(FMath::VInterpTo(
		FirstPersonArms->GetRelativeScale3D(),DesiredScale,DeltaSeconds,Speed));
}

void AShooterCharacter::FinishDedicatedFirstPersonRigAction()
{
	bFirstPersonRigActionPlaying=false;
	bFirstPersonRigLooping=false;
	CurrentFirstPersonRigAnimation=nullptr;
	UpdateDedicatedFirstPersonRigAnimation();
}

void AShooterCharacter::UpdateDedicatedFirstPersonRigAnimation()
{
	if(!IsDedicatedFirstPersonRigActive()||bFirstPersonRigActionPlaying)return;
	UAnimSequence* DesiredAnimation=GetVelocity().Size2D()>10.f
		?EquippedWeapon->FirstPersonRigWalkAnimation
		:EquippedWeapon->FirstPersonRigIdleAnimation;
	if(!DesiredAnimation)DesiredAnimation=EquippedWeapon->FirstPersonRigIdleAnimation;
	if(!DesiredAnimation||CurrentFirstPersonRigAnimation==DesiredAnimation&&bFirstPersonRigLooping)return;
	CurrentFirstPersonRigAnimation=DesiredAnimation;
	bFirstPersonRigLooping=true;
	FirstPersonArms->PlayAnimation(DesiredAnimation,true);
}
void AShooterCharacter::Interact()
{
	FHitResult H;FCollisionQueryParams P;P.AddIgnoredActor(this);const FVector A=Camera->GetComponentLocation(),B=A+Camera->GetForwardVector()*InteractionDistance;
	if(GetWorld()->LineTraceSingleByChannel(H,A,B,ECC_Visibility,P))
	{
		if(ASaveBed* Bed=Cast<ASaveBed>(H.GetActor())){ServerUseBed(Bed);return;}
		if(AStorageChest* Chest=Cast<AStorageChest>(H.GetActor())){if(AShooterPlayerController* PC=Cast<AShooterPlayerController>(Controller))PC->OpenStorageChest(Chest);return;}
		if(AWoodGate* Gate=Cast<AWoodGate>(H.GetActor())){ServerToggleGate(Gate);return;}
		if(APickupActor* Pickup=Cast<APickupActor>(H.GetActor())){ServerInteract(Pickup);return;}
	}
	APickupActor* NearestPickup=nullptr;float NearestDistanceSq=FMath::Square(InteractionDistance+75.f);
	for(TActorIterator<APickupActor> It(GetWorld());It;++It)
	{
		const float DistanceSq=FVector::DistSquared(GetActorLocation(),It->GetActorLocation());
		if(DistanceSq<NearestDistanceSq){NearestDistanceSq=DistanceSq;NearestPickup=*It;}
	}
	if(NearestPickup)ServerInteract(NearestPickup);
}
bool AShooterCharacter::ServerInteract_Validate(APickupActor* P){return P&&FVector::DistSquared(P->GetActorLocation(),GetActorLocation())<=FMath::Square(InteractionDistance+100.f);}void AShooterCharacter::ServerInteract_Implementation(APickupActor* P){P->TryPickup(this);}
bool AShooterCharacter::ServerUseBed_Validate(ASaveBed* Bed){return Bed&&FVector::DistSquared(Bed->GetActorLocation(),GetActorLocation())<=FMath::Square(InteractionDistance+150.f);}void AShooterCharacter::ServerUseBed_Implementation(ASaveBed* Bed){if(Bed)ClientSaveAtBed();}
bool AShooterCharacter::ServerToggleGate_Validate(AWoodGate* Gate){return Gate&&FVector::DistSquared(Gate->GetActorLocation(),GetActorLocation())<=FMath::Square(InteractionDistance+100.f);}void AShooterCharacter::ServerToggleGate_Implementation(AWoodGate* Gate){if(Gate)Gate->TryToggle(this);}
void AShooterCharacter::TransferItemToChest(AStorageChest* Chest,FName ItemId,int32 Quantity){if(HasAuthority())ServerTransferItemToChest_Implementation(Chest,ItemId,Quantity);else ServerTransferItemToChest(Chest,ItemId,Quantity);}
void AShooterCharacter::TransferItemFromChest(AStorageChest* Chest,FName ItemId,int32 Quantity){if(HasAuthority())ServerTransferItemFromChest_Implementation(Chest,ItemId,Quantity);else ServerTransferItemFromChest(Chest,ItemId,Quantity);}
void AShooterCharacter::StoreEquippedWeaponInChest(AStorageChest* Chest){if(HasAuthority())ServerStoreEquippedWeaponInChest_Implementation(Chest);else ServerStoreEquippedWeaponInChest(Chest);}
bool AShooterCharacter::ServerTransferItemToChest_Validate(AStorageChest* Chest,FName ItemId,int32 Quantity){return Chest&&!ItemId.IsNone()&&Quantity>0&&Quantity<=999&&FVector::DistSquared(Chest->GetActorLocation(),GetActorLocation())<=FMath::Square(500.f);}
void AShooterCharacter::ServerTransferItemToChest_Implementation(AStorageChest* Chest,FName ItemId,int32 Quantity){if(!Chest||!Chest->Storage||!Inventory)return;Chest->Storage->MaxSlots=FMath::Max(0,20-Chest->StoredWeapons.Num());if(Inventory->GetQuantity(ItemId)<Quantity||!Chest->Storage->CanAddItems(ItemId,Quantity,NAME_None,0))return;if(Inventory->RemoveItem(ItemId,Quantity)){if(!Chest->Storage->AddItem(ItemId,Quantity))Inventory->AddItem(ItemId,Quantity);}}
bool AShooterCharacter::ServerTransferItemFromChest_Validate(AStorageChest* Chest,FName ItemId,int32 Quantity){return Chest&&!ItemId.IsNone()&&Quantity>0&&Quantity<=999&&FVector::DistSquared(Chest->GetActorLocation(),GetActorLocation())<=FMath::Square(500.f);}
void AShooterCharacter::ServerTransferItemFromChest_Implementation(AStorageChest* Chest,FName ItemId,int32 Quantity)
{
	if(!Chest||!Chest->Storage||!Inventory)return;
	if(ItemId.ToString().StartsWith(TEXT("Weapon_")))
	{
		AWeaponBase* Weapon=Chest->FindStoredWeapon(ItemId);if(!Weapon)return;
		if(WeaponSlots.Num()<2){WeaponSlots.Add(Weapon);ActiveWeaponSlot=WeaponSlots.Num()-1;}
		else{AWeaponBase* Old=WeaponSlots[ActiveWeaponSlot];if(Old){const FVector DropLocation=GetActorLocation()+GetActorForwardVector()*100.f+FVector(0,0,45.f);AWeaponPickup* Drop=GetWorld()->SpawnActor<AWeaponPickup>(AWeaponPickup::StaticClass(),DropLocation,GetActorRotation());if(Drop)Drop->ConfigureWeaponClass(Old->GetClass());Old->Destroy();}WeaponSlots[ActiveWeaponSlot]=Weapon;}
		Chest->StoredWeapons.Remove(Weapon);Chest->Storage->MaxSlots=FMath::Max(0,20-Chest->StoredWeapons.Num());Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);Weapon->SetActorHiddenInGame(false);Weapon->SetOwner(this);EquippedWeapon=Weapon;OnRep_Weapon();return;
	}
	if(Chest->Storage->GetQuantity(ItemId)<Quantity||!Inventory->CanAddItems(ItemId,Quantity,NAME_None,0))return;if(Chest->Storage->RemoveItem(ItemId,Quantity)){if(!Inventory->AddItem(ItemId,Quantity))Chest->Storage->AddItem(ItemId,Quantity);}
}
bool AShooterCharacter::ServerStoreEquippedWeaponInChest_Validate(AStorageChest* Chest){return Chest&&FVector::DistSquared(Chest->GetActorLocation(),GetActorLocation())<=FMath::Square(500.f);}
void AShooterCharacter::ServerStoreEquippedWeaponInChest_Implementation(AStorageChest* Chest)
{
	if(!Chest||!EquippedWeapon||Chest->Storage->Items.Num()+Chest->StoredWeapons.Num()>=20)return;AWeaponBase* Weapon=EquippedWeapon;Chest->StoredWeapons.Add(Weapon);Chest->Storage->MaxSlots=FMath::Max(0,20-Chest->StoredWeapons.Num());WeaponSlots.Remove(Weapon);Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);Weapon->SetOwner(Chest);Weapon->AttachToActor(Chest,FAttachmentTransformRules::KeepRelativeTransform);Weapon->SetActorHiddenInGame(true);
	if(WeaponSlots.Num()>0){ActiveWeaponSlot=FMath::Clamp(ActiveWeaponSlot,0,WeaponSlots.Num()-1);EquippedWeapon=WeaponSlots[ActiveWeaponSlot];}else{ActiveWeaponSlot=INDEX_NONE;EquippedWeapon=nullptr;}OnRep_Weapon();
}
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
	Amount=FMath::Max(1,FMath::RoundToInt(Amount*GetPickupMultiplier()));
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
		++SkillPoints;
		ShowLocalNotification(FString::Printf(TEXT("LEVEL %d: +1 SKILL POINT"),CharacterLevel),5.f);
		UE_LOG(LogTemp,Display,TEXT("Player %s reached level %d"),*GetName(),CharacterLevel);
	}
}

int32 AShooterCharacter::GetSkillCost(EShooterSkill Skill)const
{
	switch(Skill)
	{
	case EShooterSkill::Marathon:return 5;
	case EShooterSkill::CombatMedic:case EShooterSkill::DeepPockets:return 2;
	case EShooterSkill::LastLife:return 3;
	default:return 1;
	}
}

FText AShooterCharacter::GetSkillName(EShooterSkill Skill)const
{
	switch(Skill)
	{
	case EShooterSkill::QuickReload:return FText::FromString(TEXT("Быстрая перезарядка"));
	case EShooterSkill::Marathon:return FText::FromString(TEXT("Марафонец"));
	case EShooterSkill::Vitality:return FText::FromString(TEXT("Живучесть"));
	case EShooterSkill::SteadyAim:return FText::FromString(TEXT("Твёрдая рука"));
	case EShooterSkill::Scavenger:return FText::FromString(TEXT("Сборщик"));
	case EShooterSkill::CombatMedic:return FText::FromString(TEXT("Полевой медик"));
	case EShooterSkill::DeepPockets:return FText::FromString(TEXT("Глубокие карманы"));
	case EShooterSkill::LastLife:return FText::FromString(TEXT("Последняя жизнь"));
	default:return FText::FromString(TEXT("Навык"));
	}
}

FText AShooterCharacter::GetSkillDescription(EShooterSkill Skill)const
{
	switch(Skill)
	{
	case EShooterSkill::QuickReload:return FText::FromString(TEXT("Перезарядка оружия быстрее на 28%."));
	case EShooterSkill::Marathon:return FText::FromString(TEXT("Бег и прыжки больше не расходуют стамину; скорость спринта выше на 15%."));
	case EShooterSkill::Vitality:return FText::FromString(TEXT("Максимальное здоровье увеличено на 25."));
	case EShooterSkill::SteadyAim:return FText::FromString(TEXT("Вертикальная и горизонтальная отдача меньше на 22%."));
	case EShooterSkill::Scavenger:return FText::FromString(TEXT("Подбираемые боеприпасы дают на 25% больше патронов."));
	case EShooterSkill::CombatMedic:return FText::FromString(TEXT("Аптечки восстанавливают на 40% больше здоровья."));
	case EShooterSkill::DeepPockets:return FText::FromString(TEXT("Максимальный переносимый вес увеличен на 15 кг."));
	case EShooterSkill::LastLife:return FText::FromString(TEXT("Один раз оставляет 1 HP и даёт бессмертие на 5 секунд."));
	default:return FText::GetEmpty();
	}
}

FText AShooterCharacter::GetSkillRequirementText(EShooterSkill Skill)const
{
	if(Skill==EShooterSkill::CombatMedic)return FText::FromString(TEXT("Требуется: Живучесть"));
	if(Skill==EShooterSkill::DeepPockets)return FText::FromString(TEXT("Требуется: Сборщик"));
	if(Skill==EShooterSkill::LastLife)return FText::FromString(TEXT("Требуются: Живучесть и Полевой медик"));
	return FText::GetEmpty();
}

bool AShooterCharacter::CanPurchaseSkill(EShooterSkill Skill)const
{
	if(HasSkill(Skill)||SkillPoints<GetSkillCost(Skill))return false;
	if(Skill==EShooterSkill::CombatMedic&&!HasSkill(EShooterSkill::Vitality))return false;
	if(Skill==EShooterSkill::DeepPockets&&!HasSkill(EShooterSkill::Scavenger))return false;
	if(Skill==EShooterSkill::LastLife&&(!HasSkill(EShooterSkill::Vitality)||!HasSkill(EShooterSkill::CombatMedic)))return false;
	return true;
}

bool AShooterCharacter::PurchaseSkill(EShooterSkill Skill)
{
	if(!HasAuthority()){ServerPurchaseSkill(Skill);return CanPurchaseSkill(Skill);}
	if(!CanPurchaseSkill(Skill))return false;
	SkillPoints-=GetSkillCost(Skill);
	UnlockedSkills.AddUnique(Skill);
	ApplyUnlockedSkillEffects();
	ShowLocalNotification(FString::Printf(TEXT("UNLOCKED: %s"),*GetSkillName(Skill).ToString()),4.f);
	return true;
}

bool AShooterCharacter::ServerPurchaseSkill_Validate(EShooterSkill Skill){return static_cast<uint8>(Skill)<=static_cast<uint8>(EShooterSkill::LastLife);}
void AShooterCharacter::ServerPurchaseSkill_Implementation(EShooterSkill Skill){PurchaseSkill(Skill);}

void AShooterCharacter::ApplyUnlockedSkillEffects()
{
	if(Health)
	{
		const float OldMax=Health->MaxHealth;
		Health->MaxHealth=HasSkill(EShooterSkill::Vitality)?125.f:100.f;
		if(HasAuthority()&&Health->Health>0.f&&Health->MaxHealth>OldMax)Health->Health=FMath::Min(Health->MaxHealth,Health->Health+Health->MaxHealth-OldMax);
	}
	if(Inventory)Inventory->MaxWeight=HasSkill(EShooterSkill::DeepPockets)?650.f:500.f;
	if(HasSkill(EShooterSkill::Marathon))Stamina=MaxStamina;
}

bool AShooterCharacter::IsLastLifeInvulnerable()const
{
	return GetWorld()&&GetWorld()->GetTimeSeconds()<LastLifeInvulnerableUntil;
}

bool AShooterCharacter::TryActivateLastLife()
{
	if(!HasAuthority()||!HasSkill(EShooterSkill::LastLife)||bLastLifeConsumed)return false;
	bLastLifeConsumed=true;
	LastLifeInvulnerableUntil=GetWorld()->GetTimeSeconds()+5.f;
	ShowLocalNotification(TEXT("LAST LIFE: 5 SECONDS OF INVULNERABILITY"),5.f);
	return true;
}

void AShooterCharacter::OnRep_Weapon()
{
	RefreshAnimationState();
	FirstPersonBody->SetHiddenInGame(true);
	ConfigureDedicatedFirstPersonRig();

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
void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AShooterCharacter,EquippedWeapon);DOREPLIFETIME(AShooterCharacter,WeaponSlots);DOREPLIFETIME(AShooterCharacter,ActiveWeaponSlot);DOREPLIFETIME(AShooterCharacter,bIsAiming);DOREPLIFETIME(AShooterCharacter,bWantsToSprint);DOREPLIFETIME(AShooterCharacter,Stamina);DOREPLIFETIME(AShooterCharacter,CharacterLevel);DOREPLIFETIME(AShooterCharacter,Experience);DOREPLIFETIME(AShooterCharacter,TotalExperience);DOREPLIFETIME(AShooterCharacter,SkillPoints);DOREPLIFETIME(AShooterCharacter,UnlockedSkills);DOREPLIFETIME(AShooterCharacter,bLastLifeConsumed);DOREPLIFETIME(AShooterCharacter,LastLifeInvulnerableUntil);}
