#include "Misc/EngineGlobals.h"
#include "RHI/BaseRHI.h"
#include "Logger/BaseLogger.h"
#include "Logger/LoggerMacros.h"
#include "Render/RenderingThread.h"

/* Whether the renderer is currently running in a separate thread */
bool			GIsThreadedRendering = false;

/* ID of rendering thread */
uint32			GRenderingThreadId = 0;

bool FRenderingThread::Init()
{
	// Acquire rendering context ownership on the current thread
	GRHI->AcquireThreadOwnership();

	return true;
}

uint32 FRenderingThread::Run()
{
	while ( GIsThreadedRendering )
	{
	}

	return 0;
}

void FRenderingThread::Stop()
{}

void FRenderingThread::Exit()
{
	// Release rendering context ownership on the current thread
	GRHI->ReleaseThreadOwnership();
}

/** Thread used for rendering */
FRunnableThread*	GRenderingThread = nullptr;
FRunnable*			GRenderingThreadRunnable = nullptr;

void StartRenderingThread()
{
	if ( !GIsThreadedRendering )
	{
		// Turn on the threaded rendering flag.
		GIsThreadedRendering = true;

		// Create the rendering thread.
		GRenderingThreadRunnable = new FRenderingThread();

		// Release rendering context ownership on the current thread
		GRHI->ReleaseThreadOwnership();

		const uint32		stackSize = 0;
		GRenderingThread = GThreadFactory->CreateThread( GRenderingThreadRunnable, TEXT( "RenderingThread" ), 0, 0, stackSize, TP_Realtime );
		check( GRenderingThread );
	}
}

void StopRenderingThread()
{
	// This function is not thread-safe. Ensure it is only called by the main game thread.
	check( IsInGameThread() );

	static bool			GIsRenderThreadStopping = false;
	if ( GIsThreadedRendering && !GIsRenderThreadStopping )
	{
		GIsRenderThreadStopping = true;

		// The rendering thread may have already been stopped
		if ( GIsThreadedRendering )
		{
			check( GRenderingThread );

			// Turn off the threaded rendering flag.
			GIsThreadedRendering = false;

			//Reset the rendering thread id
			GRenderingThreadId = 0;

			// Wait for the rendering thread to return.
			GRenderingThread->WaitForCompletion();

			// We must kill the thread here, so that it correctly frees up the rendering thread handle
			// without this we get thread leaks when the device is lost TTP 14738, TTP 22274
			GRenderingThread->Kill();

			// Destroy the rendering thread objects.
			GThreadFactory->Destroy( GRenderingThread );
			delete GRenderingThreadRunnable;

			GRenderingThread = nullptr;
			GRenderingThreadRunnable = nullptr;

			// Acquire rendering context ownership on the current thread
			GRHI->AcquireThreadOwnership();
		}
	}

	GIsRenderThreadStopping = false;
}