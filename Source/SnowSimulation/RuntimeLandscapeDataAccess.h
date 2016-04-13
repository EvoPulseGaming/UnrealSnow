// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
LandscapeDataAccess.h: Classes for the editor to access to Landscape data
=============================================================================*/

#ifndef _LANDSCAPEDATAACCESS_H
#define _LANDSCAPEDATAACCESS_H

#include "Engine/Texture2D.h"

#define LANDSCAPE_VALIDATE_DATA_ACCESS 1
#define LANDSCAPE_ZSCALE		(1.0f/128.0f)
#define LANDSCAPE_INV_ZSCALE	128.0f

#define LANDSCAPE_XYOFFSET_SCALE	(1.0f/256.f)
#define LANDSCAPE_INV_XYOFFSET_SCALE	256.f

namespace RuntimeLandscapeDataAccess
{
	const int32 MaxValue = 65535;
	const float MidValue = 32768.f;
	// Reserved 2 bits for other purpose
	// Most significant bit - Visibility, 0 is visible(default), 1 is invisible
	// 2nd significant bit - Triangle flip, not implemented yet
	FORCEINLINE float GetLocalHeight(uint16 Height)
	{
		return ((float)Height - MidValue) * LANDSCAPE_ZSCALE;
	}

	FORCEINLINE uint16 GetTexHeight(float Height)
	{
		return FMath::Clamp<float>(Height * LANDSCAPE_INV_ZSCALE + MidValue, 0.f, MaxValue);
	}
};


class ULandscapeComponent;
class ULandscapeLayerInfoObject;

/*
struct FTextureAccessor
{
private:
	UTexture2D* Texture;
	void* TextureData;

#if WITH_EDITOR
	TArray<uint8> MipData;
#endif

public:

	FTextureAccessor(UTexture2D* Texture) : Texture(Texture)
	{
#if WITH_EDITOR
		Texture->Source.GetMipData(MipData, 0);
		TextureData = MipData.GetData();
#else
		TextureData = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_ONLY);
#endif
	}

	~FTextureAccessor()
	{
#if !WITH_EDITOR
		Texture->PlatformData->Mips[0].BulkData.Unlock();
#endif
	}

	FColor* GetTextureData()
	{
		return TextureData;
	}
};
*/
struct FRuntimeMipAccessor
{
private:

	struct FLockedMipDataInfo
	{
		FLockedMipDataInfo()
			: LockCount(0)
		{}

#if WITH_EDITOR
		TArray<uint8> MipData;
#else 
		void* MipData;
#endif
		int32 LockCount;
	};

public:
	FRuntimeMipAccessor()
	{}

	void* LockMip(UTexture2D* Texture, int32 MipLevel)
	{
		check(MipLevel < Texture->GetNumMips());

		TArray<FLockedMipDataInfo>* MipInfo = LockedMipInfoMap.Find(Texture);
		if (MipInfo == NULL)
		{
			MipInfo = &LockedMipInfoMap.Add(Texture, TArray<FLockedMipDataInfo>());
			for (int32 i = 0; i < Texture->GetNumMips(); ++i)
			{
				MipInfo->Add(FLockedMipDataInfo());
			}
		}

		auto LockedMipData = (*MipInfo)[MipLevel];
#if WITH_EDITOR
		if (LockedMipData.MipData.Num() == 0)
		{
			Texture->Source.GetMipData(LockedMipData.MipData, MipLevel);
		}
		LockedMipData.LockCount++;

		return LockedMipData.MipData.GetData();
#else 
		if (LockedMipData.MipData == nullptr)
		{
			LockedMipData.MipData = Texture->PlatformData->Mips[MipLevel].BulkData.Lock(LOCK_READ_ONLY);
		}
		LockedMipData.LockCount++;

		return LockedMipData.MipData;
#endif
	
	}

	void UnlockMip(UTexture2D* Texture, int32 MipLevel)
	{
		TArray<FLockedMipDataInfo>* MipInfo = LockedMipInfoMap.Find(Texture);
		check(MipInfo);

		auto LockedMipData = (*MipInfo)[MipLevel];
		if (LockedMipData.LockCount <= 0)
			return;
		LockedMipData.LockCount--;
		if (LockedMipData.LockCount == 0)
		{

#if WITH_EDITOR
			check(LockedMipData.MipData.Num() != 0);
			LockedMipData.MipData.Empty();
#else
			check(LockedMipData.MipData != NULL);
			Texture->PlatformData->Mips[MipLevel].BulkData.Unlock();
			LockedMipData.MipData = NULL;
#endif
			
		}
	}

private:
	TMap<UTexture2D*, TArray<FLockedMipDataInfo> > LockedMipInfoMap;
};

//
// FLandscapeComponentDataInterface
//
struct FRuntimeLandscapeComponentDataInterface
{
	friend struct FRuntimeLandscapeDataCache;

	// tors
	SNOWSIMULATION_API FRuntimeLandscapeComponentDataInterface(ULandscapeComponent* InComponent, FRuntimeMipAccessor* DataInterface, int32 MipLevel);
	SNOWSIMULATION_API ~FRuntimeLandscapeComponentDataInterface();

	// Accessors
	void VertexIndexToXY(int32 VertexIndex, int32& OutX, int32& OutY) const
	{
		//#if LANDSCAPE_VALIDATE_DATA_ACCESS
		//		check(MipLevel == 0);
		//#endif
		OutX = VertexIndex % ComponentSizeVerts;
		OutY = VertexIndex / ComponentSizeVerts;
	}

	// Accessors
	void QuadIndexToXY(int32 QuadIndex, int32& OutX, int32& OutY) const
	{
		//#if LANDSCAPE_VALIDATE_DATA_ACCESS
		//		check(MipLevel == 0);
		//#endif
		OutX = QuadIndex % (ComponentSizeVerts - 1);
		OutY = QuadIndex / (ComponentSizeVerts - 1);
	}

	int32 VertexXYToIndex(int32 VertX, int32 VertY) const
	{
		return VertY * ComponentSizeVerts + VertX;
	}

	void ComponentXYToSubsectionXY(int32 CompX, int32 CompY, int32& SubNumX, int32& SubNumY, int32& SubX, int32& SubY) const
	{
		// We do the calculation as if we're looking for the previous vertex.
		// This allows us to pick up the last shared vertex of every subsection correctly.
		SubNumX = (CompX - 1) / (SubsectionSizeVerts - 1);
		SubNumY = (CompY - 1) / (SubsectionSizeVerts - 1);
		SubX = (CompX - 1) % (SubsectionSizeVerts - 1) + 1;
		SubY = (CompY - 1) % (SubsectionSizeVerts - 1) + 1;

		// If we're asking for the first vertex, the calculation above will lead
		// to a negative SubNumX/Y, so we need to fix that case up.
		if (SubNumX < 0)
		{
			SubNumX = 0;
			SubX = 0;
		}

		if (SubNumY < 0)
		{
			SubNumY = 0;
			SubY = 0;
		}
	}

	void VertexXYToTexelXY(int32 VertX, int32 VertY, int32& OutX, int32& OutY) const
	{
		int32 SubNumX, SubNumY, SubX, SubY;
		ComponentXYToSubsectionXY(VertX, VertY, SubNumX, SubNumY, SubX, SubY);

		OutX = SubNumX * SubsectionSizeVerts + SubX;
		OutY = SubNumY * SubsectionSizeVerts + SubY;
	}

	int32 VertexIndexToTexel(int32 VertexIndex) const
	{
		int32 VertX, VertY;
		VertexIndexToXY(VertexIndex, VertX, VertY);
		int32 TexelX, TexelY;
		VertexXYToTexelXY(VertX, VertY, TexelX, TexelY);
		return TexelXYToIndex(TexelX, TexelY);
	}

	int32 TexelXYToIndex(int32 TexelX, int32 TexelY) const
	{
		return TexelY * ComponentNumSubsections * SubsectionSizeVerts + TexelX;
	}

	FColor* GetRawHeightData() const
	{
		return HeightMipData;
	}

	FColor* GetRawXYOffsetData() const
	{
		return XYOffsetMipData;
	}

	void SetRawHeightData(FColor* NewHeightData)
	{
		HeightMipData = NewHeightData;
	}

	void SetRawXYOffsetData(FColor* NewXYOffsetData)
	{
		XYOffsetMipData = NewXYOffsetData;
	}

	/* Return the raw heightmap data exactly same size for Heightmap texture which belong to only this component */
	SNOWSIMULATION_API void GetHeightmapTextureData(TArray<FColor>& OutData, bool bOkToFail = false);

	FColor* GetHeightData(int32 LocalX, int32 LocalY) const
	{
#if LANDSCAPE_VALIDATE_DATA_ACCESS
		check(Component);
		check(HeightMipData);
		check(LocalX >= 0 && LocalY >= 0 && LocalX < ComponentSizeVerts && LocalY < ComponentSizeVerts);
#endif

		int32 TexelX, TexelY;
		VertexXYToTexelXY(LocalX, LocalY, TexelX, TexelY);

		return &HeightMipData[TexelX + HeightmapComponentOffsetX + (TexelY + HeightmapComponentOffsetY) * HeightmapStride];
	}

	SNOWSIMULATION_API FColor* GetXYOffsetData(int32 LocalX, int32 LocalY) const;

	uint16 GetHeight(int32 LocalX, int32 LocalY) const
	{
		FColor* Texel = GetHeightData(LocalX, LocalY);
		return (Texel->R << 8) + Texel->G;
	}

	uint16 GetHeight(int32 VertexIndex) const
	{
		int32 X, Y;
		VertexIndexToXY(VertexIndex, X, Y);
		return GetHeight(X, Y);
	}

	void GetXYOffset(int32 X, int32 Y, float& XOffset, float& YOffset) const
	{
		if (XYOffsetMipData)
		{
			FColor* Texel = GetXYOffsetData(X, Y);
			XOffset = ((float)((Texel->R << 8) + Texel->G) - 32768.f) * LANDSCAPE_XYOFFSET_SCALE;
			YOffset = ((float)((Texel->B << 8) + Texel->A) - 32768.f) * LANDSCAPE_XYOFFSET_SCALE;
		}
		else
		{
			XOffset = YOffset = 0.f;
		}
	}

	void GetXYOffset(int32 VertexIndex, float& XOffset, float& YOffset) const
	{
		int32 X, Y;
		VertexIndexToXY(VertexIndex, X, Y);
		GetXYOffset(X, Y, XOffset, YOffset);
	}

	SNOWSIMULATION_API FVector GetLocalVertex(int32 LocalX, int32 LocalY) const;

	void GetLocalTangentVectors(int32 LocalX, int32 LocalY, FVector& LocalTangentX, FVector& LocalTangentY, FVector& LocalTangentZ) const
	{
		// Note: these are still pre-scaled, just not rotated

		FColor* Data = GetHeightData(LocalX, LocalY);
		LocalTangentZ.X = 2.f * (float)Data->B / 255.f - 1.f;
		LocalTangentZ.Y = 2.f * (float)Data->A / 255.f - 1.f;
		LocalTangentZ.Z = FMath::Sqrt(1.f - (FMath::Square(LocalTangentZ.X) + FMath::Square(LocalTangentZ.Y)));
		LocalTangentX = FVector(-LocalTangentZ.Z, 0.f, LocalTangentZ.X);
		LocalTangentY = FVector(0.f, LocalTangentZ.Z, -LocalTangentZ.Y);
	}

	FVector GetLocalVertex(int32 VertexIndex) const
	{
		int32 X, Y;
		VertexIndexToXY(VertexIndex, X, Y);
		return GetLocalVertex(X, Y);
	}

	void GetLocalTangentVectors(int32 VertexIndex, FVector& LocalTangentX, FVector& LocalTangentY, FVector& LocalTangentZ) const
	{
		int32 X, Y;
		VertexIndexToXY(VertexIndex, X, Y);
		GetLocalTangentVectors(X, Y, LocalTangentX, LocalTangentY, LocalTangentZ);
	}

	SNOWSIMULATION_API FVector GetWorldVertex(int32 LocalX, int32 LocalY) const;

	SNOWSIMULATION_API void GetWorldTangentVectors(int32 LocalX, int32 LocalY, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ) const;

	SNOWSIMULATION_API void GetWorldPositionTangents(int32 LocalX, int32 LocalY, FVector& WorldPos, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ) const;

	FVector GetWorldVertex(int32 VertexIndex) const
	{
		int32 X, Y;
		VertexIndexToXY(VertexIndex, X, Y);
		return GetWorldVertex(X, Y);
	}

	void GetWorldTangentVectors(int32 VertexIndex, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ) const
	{
		int32 X, Y;
		VertexIndexToXY(VertexIndex, X, Y);
		GetWorldTangentVectors(X, Y, WorldTangentX, WorldTangentY, WorldTangentZ);
	}

	void GetWorldPositionTangents(int32 VertexIndex, FVector& WorldPos, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ) const
	{
		int32 X, Y;
		VertexIndexToXY(VertexIndex, X, Y);
		GetWorldPositionTangents(X, Y, WorldPos, WorldTangentX, WorldTangentY, WorldTangentZ);
	}

private:
	struct FRuntimeLandscapeDataCache* DataInterface;
	ULandscapeComponent* Component;

public:
	// offset of this component's data into heightmap texture
	int32 HeightmapStride;
	int32 HeightmapComponentOffsetX;
	int32 HeightmapComponentOffsetY;
	int32 HeightmapSubsectionOffset;

private:
	FColor* HeightMipData;
	FColor* XYOffsetMipData;

	int32 ComponentSizeVerts;
	int32 SubsectionSizeVerts;
	int32 ComponentNumSubsections;

	bool bNeedToDeleteDataInterface;
public:
	const int32 MipLevel;
};

// Helper functions
template<typename T>
void FillCornerValues(uint8& CornerSet, T* CornerValues)
{
	uint8 OriginalSet = CornerSet;

	if (CornerSet)
	{
		// Fill unset values
		while (CornerSet != 15)
		{
			if (CornerSet != 15 && (OriginalSet & 1))
			{
				if (!(CornerSet & 1 << 1))
				{
					CornerValues[1] = CornerValues[0];
					CornerSet |= 1 << 1;
				}
				if (!(CornerSet & 1 << 2))
				{
					CornerValues[2] = CornerValues[0];
					CornerSet |= 1 << 2;
				}
			}
			if (CornerSet != 15 && (OriginalSet & 1 << 1))
			{
				if (!(CornerSet & 1))
				{
					CornerValues[0] = CornerValues[1];
					CornerSet |= 1;
				}
				if (!(CornerSet & 1 << 3))
				{
					CornerValues[3] = CornerValues[1];
					CornerSet |= 1 << 3;
				}
			}
			if (CornerSet != 15 && (OriginalSet & 1 << 2))
			{
				if (!(CornerSet & 1))
				{
					CornerValues[0] = CornerValues[2];
					CornerSet |= 1;
				}
				if (!(CornerSet & 1 << 3))
				{
					CornerValues[3] = CornerValues[2];
					CornerSet |= 1 << 3;
				}
			}
			if (CornerSet != 15 && (OriginalSet & 1 << 3))
			{
				if (!(CornerSet & 1 << 1))
				{
					CornerValues[1] = CornerValues[3];
					CornerSet |= 1 << 1;
				}
				if (!(CornerSet & 1 << 2))
				{
					CornerValues[2] = CornerValues[3];
					CornerSet |= 1 << 2;
				}
			}

			OriginalSet = CornerSet;
		}
	}
}



#endif // _LANDSCAPEDATAACCESS_H
