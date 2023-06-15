#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

class FSkeletonRetargetCommands : public TCommands<FSkeletonRetargetCommands>
{
public:
	FSkeletonRetargetCommands()
		: TCommands<FSkeletonRetargetCommands>(TEXT("SkeletonRetarget.Common"), NSLOCTEXT("Contexts", "Common", "Common"), NAME_None, FName(TEXT("SkeletonRetargetStyle")))
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> RetargetSkeleton;
};
