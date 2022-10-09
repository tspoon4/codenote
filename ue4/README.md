# UE4

## Pipeline

* Compilation setup
	* UnrealBuildTool
	* Project.Target.cs
	* Engine\Source\Runtime\Core\Public\Misc\Build.h
	* INI files (specific variables)
* DDC shared folder
	* Setup on Windows Server, high connection limit
	* Setup on SSD / NVMe, high throughtput
	* Verify everybody has access rights
	* Verify DDC statistics in the UE editor
	* Environment variable override: UE-SharedDataCachePath \\10.0.18.36\ddc
* UGS
* Cooking
* Lightmass
	* Setup Swarm lightmass baking farm
	* Fast baking quality "preview" mode
	* Minimize lightmap and volumetric lightmap resolution

## Console
* Engine
	* stat levels
* Graphics
	* stat unit
	* stat gpu
	* stat streaming
	* r.SetRes
	* r.ScreenPercentage
	* r.DynRes.MinScreenPercentage
	* sg.XXXquality
	* show TYPE
	* ShowFlags
	* ToggleDebugCamera
    
## Command line
* UE4Editor-Cmd.exe
* -unattended
* -ExecCmds="stat unit, stat gpu"
* -run=pythonscript -script="c:\\script.py"
* -basedir $(SolutionDir)PROJECT\Saved\StagedBuilds\WindowsNoEditor\PROJECT\Content: run with Paks
* -d3ddebug: Enable D3D debug layer
* -ddc=noshared: Prevent use of network DDC

## Tools
* TAutoConsoleVariable
* UCommanlet
* UBlueprintFunctionLibrary (for python)

## TRC
* Streaming installation: Chunks
* Achievements
* SaveGame

## Optimization

### CPU

* Profiling tools
	* stat startfile/stopfile
	* Insight -statnamedevents -trace=cpu,frames,log,bookmarks
	* PIX/Razor with -statnamedevents
* Garbage collection
	* Activate asset clustering
	* Activate actor clustering
	* Activate blueprint clustering
	* Activate incremental GC
	* Disable ForceGCAfterLevelStreamedOut
	* Avoid inherit from UObject if not necessary
* Actor components
	* FTickFunction strategy
	* bAnyThread at the beginning of the project
	* Avoid overlap events
	* Verify FParallel tasks are running on the TaskGraph/WorkerThreads
* Niagara systems
	* Fixed bounds, to avoid update bounds in the render thread
* Blocking loading
	* Seamless vs. non-seamless travel
	* BP Construction graphs LoadObject
	* Chase FlushAsyncLoading and remove them

### GPU

* Profiling tools
	* stat gpu
	* profilegpu
	* PIX/Razor/RenderDoc with "Development" configuration that has markers
	* Insights -trace=default,gpu
	* r.DumpShaderDebugInfo=1
* Scalability
	* Global settings
	* NET for Niagara systems
	* Texture groups
* Bottlenecks
	* PrePass (Depth only): tweak r.EarlyZPass, adjust mesh LODs, remove masked materials
	* BasePass (GBuffer): tweak r.EarlyZPass, reduce material complexity, set simpler materials on LODs
* Crash
	* regedit for TdrDelay, TdrDdiDelay
	* -gpucrashdebugging

### Memory

* Profiling tools
	* -LLM or -LLMCSV
	* stat LLM or LLMFULL
	* memreport -full
	* obj list Class=TYPE
	* Insights -trace=memory
	* -stompmemalloc
* References
	* From persistent levels
	* From sublevels
	* From gameplay actors (characters, smart objects)
	* UE Editor Reference Viewer: check data dependencies
* Meshes
	* Drop highest LODs
	* Activate mesh streaming
	* Strip geometry copy on CPU side
* Misc
	* Simple vs. Complex collision
	* Volumetric lightmaps resolution
	* Minimize offscreen 2D RenderTarget assets
	* Minimize volumetric fog quality that impacts 3D RenderTarget

### IO

* Profiling tools
	* Insights -statnamedevents -trace=cpu,loadtime
* Optimizations
	* Pak compression / Hardware compression
	* IOStore packaging settings
	* File ordering for HDD: -fileopenlog


