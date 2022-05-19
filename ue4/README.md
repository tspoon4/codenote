# UE4

## Pipeline

* DDC shared folder
	* Setup on Windows Server, high connection limit
	* Setup on SSD / NVMe, high throughtput
	* Verify everybody has access rights
	* Verify DDC statistics in the UE editor
* UGS
* Cooking
* Lightmass
	* Setup Swarm lightmass baking farm
	* Fast baking quality "preview" mode
	* Reduce lightmap and volumetric lightmap resolution

## Console
* Graphics
    * stat unit
    * stat gpu
    * r.SetRes
    * r.ScreenPercentage
    * r.Shadow.MaxResolution
    * show TYPE
	* ShowFlags
    
## Command line

* -basedir: run with PAK $(SolutionDir)PROJECT\Saved\StagedBuilds\WindowsNoEditor\PROJECT\Content
* -d3ddebug: Enable D3D debug layer
* -ddc=noshared: Prevent use of network DDC

## Optimization

### CPU

* Profiling tools
	* stat startfile/stopfile
	* Insight -statnamedevent -trace=cpu
	* PIX/Razor with -statnamedevent
* GC
	* Asset clustering
	* Actor clustering
	* Blueprint clustering
	* Incremental GC
	* Disable ForceGCAfterLevelStreamedOut
	* Avoid inherit from UObject if not necessary
* Actor components
	* bAnyThread
	* TickFrequency
	* Overlap events
* Niagara systems
	* Fixed bounds, to avoid update bounds in the render thread
* Blocking loading
	* BP Construction graphs

### GPU

* Profiling tools
	* stat gpu
	* profilegpu
	* PIX/Razor/RenderDoc with "Development" configuration that has markers
	* Insights -trace=default,gpu
* Scalability
	* Global settings
	* NET for Niagara systems
	* Texture groups
* Bottlenecks
	* PrePass (Depth only): tweak r.EarlyZPass, adjust mesh LODs, remove masked materials
	* BasePass (GBuffer): tweak r.EarlyZPass, reduce material complexity, set simpler materials on LODs

### Memory

* Profiling tools
	* -LLM or -LLMCSV
	* stat LLM or LLMFULL
	* memreport -full
	* obj list Class=TYPE
	* Insights -trace=memory
* References
	* From persistent levels
	* From sublevels
	* From gameplay actors (characters, smart objects)
	* UE Editor tool: Reference Viewer to check data dependencies
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
	* Insights -statnamedevent -trace=cpu,loadtime

