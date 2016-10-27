#include "../PixelShaderPrivatePCH.h"
#include "SnowPixelShaderDeclaration.h"
#include "RHIStaticStates.h"
#include "SnowPixelShader.h"

#define WRITE_SNOW_MAP false

//It seems to be the convention to expose all vertex declarations as globals, and then reference them as externs in the headers where they are needed.
//It kind of makes sense since they do not contain any parameters that change and are purely used as their names suggest, as declarations :)
TGlobalResource<FTextureVertexDeclaration> GSnowTextureVertexDeclaration;

FSnowPixelShader::FSnowPixelShader(ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	FeatureLevel = ShaderFeatureLevel;

	ConstantParameters = FSnowPixelShaderConstantParameters();
	
	VariableParameters = FSnowPixelShaderVariableParameters();
	
	bMustRegenerateSRV = false;
	bIsPixelShaderExecuting = false;
	bIsUnloading = false;
	bSave = false;

	CurrentTexture = NULL;
	CurrentRenderTarget = NULL;
}

void FSnowPixelShader::Initialize(FRWStructuredBuffer* SnowBuffer, FRWStructuredBuffer* MaxSnowBuffer, int32 CellsDimensionX, int32 CellsDimensionY)
{
	this->SnowInputBuffer = SnowBuffer;
	this->MaxSnowInputBuffer = MaxSnowBuffer;

	ConstantParameters.CellsDimensionX = CellsDimensionX;
	ConstantParameters.CellsDimensionY = CellsDimensionY;
}

FSnowPixelShader::~FSnowPixelShader()
{
	bIsUnloading = true;
}

void FSnowPixelShader::ExecutePixelShader(UTextureRenderTarget2D* RenderTarget, bool SaveSnowMap)
{
	if (bIsUnloading || bIsPixelShaderExecuting) //Skip this execution round if we are already executing
	{
		return;
	}

	bIsPixelShaderExecuting = true;

	CurrentRenderTarget = RenderTarget;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FPixelShaderRunner,
		FSnowPixelShader*, PixelShader, this,
		bool, SaveSnowMap, SaveSnowMap,
		{
			PixelShader->ExecutePixelShaderInternal(SaveSnowMap);
		}
	);
}

void FSnowPixelShader::ExecutePixelShaderInternal(bool SaveSnowMap)
{
	check(IsInRenderingThread());

	// Only cleanup
	if (bIsUnloading) 
	{
		return;
	}

	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	CurrentTexture = CurrentRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
	SetRenderTarget(RHICmdList, CurrentTexture, FTextureRHIRef());
	RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
	
	static FGlobalBoundShaderState BoundShaderState;
	TShaderMapRef<FSnowVertexShader> VertexShader(GetGlobalShaderMap(FeatureLevel));
	TShaderMapRef<FSnowPixelShaderDeclaration> PixelShader(GetGlobalShaderMap(FeatureLevel));

	SetGlobalBoundShaderState(RHICmdList, FeatureLevel, BoundShaderState, GSnowTextureVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetParameters(RHICmdList, SnowInputBuffer->SRV, MaxSnowInputBuffer->SRV);
	PixelShader->SetUniformBuffers(RHICmdList, ConstantParameters, VariableParameters);

	// Draw a fullscreen quad that we can run our pixel shader on
	FTextureVertex Vertices[4];
	Vertices[0].Position = FVector4(-1.0f, 1.0f, 0, 1.0f);
	Vertices[1].Position = FVector4(1.0f, 1.0f, 0, 1.0f);
	Vertices[2].Position = FVector4(-1.0f, -1.0f, 0, 1.0f);
	Vertices[3].Position = FVector4(1.0f, -1.0f, 0, 1.0f);
	Vertices[0].UV = FVector2D(0, 0);
	Vertices[1].UV = FVector2D(ConstantParameters.CellsDimensionX, 0);
	Vertices[2].UV = FVector2D(0, ConstantParameters.CellsDimensionY);
	Vertices[3].UV = FVector2D(ConstantParameters.CellsDimensionX, ConstantParameters.CellsDimensionY);

	DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, Vertices, sizeof(Vertices[0]));
	 
	bIsPixelShaderExecuting = false;
	
	if (SaveSnowMap) 
	{
		TArray<FColor> Bitmap;

		FReadSurfaceDataFlags ReadDataFlags;
		ReadDataFlags.SetLinearToGamma(false);
		ReadDataFlags.SetOutputStencil(false);
		ReadDataFlags.SetMip(0);

		//This is pretty straight forward. Since we are using a standard format, we can use this convenience function instead of having to lock rect.
		RHICmdList.ReadSurfaceData(CurrentTexture, FIntRect(0, 0, CurrentTexture->GetSizeX(), CurrentTexture->GetSizeY()), Bitmap, ReadDataFlags);

		// if the format and texture type is supported
		if (Bitmap.Num())
		{
			// Create screenshot folder if not already present.
			IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);

			const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("SnowMap"));

			uint32 ExtendXWithMSAA = Bitmap.Num() / CurrentTexture->GetSizeY();

			// Save the contents of the array to a bitmap file. (24bit only so alpha channel is dropped)
			FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, CurrentTexture->GetSizeY(), Bitmap.GetData());

		}
		else
		{
			UE_LOG(LogConsoleResponse, Error, TEXT("Failed to save BMP, format or texture type is not supported"));
		}
	}
}



