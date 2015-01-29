// implement this interface to allow Actors to affect the path node graph UT builds on top of the navmesh
// simply implementing the interface makes it a seed point for building and you can optionally
// add functionality to generate special paths with unique properties or requirements (e.g. teleporters)
// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPathBuilderInterface.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UUTPathBuilderInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class UNREALTOURNAMENT_API IUTPathBuilderInterface
{
	GENERATED_IINTERFACE_BODY()

	/** return whether this Actor should be stored in its PathNode's list for efficient access during path searches
	 * return false for objects that only need to run path building code and not game time code
	 */
	virtual bool IsPOI() const
	{
		return true;
	}

	/** add special paths needed to interact with this Actor
	 * NOTE: if IsPOI() is false, MyNode will be NULL since the Actor won't be put in any node's POI list
	 */
	virtual void AddSpecialPaths(class UUTPathNode* MyNode, class AUTRecastNavMesh* NavData)
	{}
};
