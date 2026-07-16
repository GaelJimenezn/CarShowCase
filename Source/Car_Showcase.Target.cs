using UnrealBuildTool;
using System.Collections.Generic;

public class Car_ShowcaseTarget : TargetRules
{
	public Car_ShowcaseTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		ExtraModuleNames.Add("Car_Showcase");
	}
}
