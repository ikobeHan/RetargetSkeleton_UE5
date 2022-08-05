#include "SkeletonRetargetCommands.h"

#define LOCTEXT_NAMESPACE "SkeletonRetargetCommands"

void FSkeletonRetargetCommands::RegisterCommands()
{
	UI_COMMAND(RetargetSkeleton, "Retarget_RigSet", "Retarget_RigSet", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
