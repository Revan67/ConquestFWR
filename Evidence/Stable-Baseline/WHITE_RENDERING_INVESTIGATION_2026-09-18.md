# White placement and construction investigation

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Scope: user authorized prioritizing white ghosts, construction effects, and the
untextured planet. HQ crash/CRT assertion remain flagged but deferred.

## Confirmed source defects

1. UnbornMeshList::Render requests green (0,255,0,128) or red
   (255,0,0,128). RenderMeshAt passes these into per-face-group diffuse/emissive
   colors and alpha. The placement caller supplies the expected distinction.
2. FaceGroupRender::Render has a disabled CPU-colored fallback (`false && ...`).
   Its non-CQ2 material branch binds colorTex but selects D3DTA_TFACTOR as the
   stage-zero color and alpha sources, disables alpha blending, and draws a VB
   without the per-instance diffuse color. This branch cannot implement the
   requested tint/texture/alpha contract as written.
3. This branch sets some states through BATCH and immediately draws through
   PIPE. RenderBatcher::set_texture_stage_state only caches changes while
   RPR_BATCH is true. The requested state therefore need not reach the immediate
   draw. Actual outcome depends on incoming state.
4. SetCommonStates returns immediately after updating cam_pos. Its legacy
   texture-times-diffuse and alpha setup is unreachable.
5. FaceGroupRender::cleanupRenderStates writes depth disabled through PIPE,
   among other states, without a corresponding batch-cache update. Audit this
   boundary before claiming the next planet draw is safe.

## Repair constraints / next implementation work

The existing MMaterial<RPVertex,D3DFVF_RPVERTEX>::Render computes per-vertex
lighting, tint, alpha and visibility and submits through CQBATCH. Investigate
restoring this route for legacy materials, including material-state caching,
modelview, flush boundaries and state identity. Do not simply enable texture
modulation on the current VB: it lacks per-instance diffuse tint.

RenderPortion/RenderPortionZAlign retain separate direct shader-era paths and
need their own geometry-preserving review. TerranBuild's historical Render is
compiled out; its current Update/ReBuild/composeGirders chain must be followed
before attributing all animated white geometry to ordinary FaceGroupRender.

The source defects explain a broken contract but are not GPU capture proof of
every symptom. Planet causality remains open: determine whether it is already
white before any ghost/effect and inspect its texture/resource state independently.

No production source changes or new deployment in this investigation pass.
